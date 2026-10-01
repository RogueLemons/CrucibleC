#include "rule_global_variable.hpp"

#include <cctype>

bool GlobalVariableRule::isThirdParty(const std::string &path) const {
    for (const auto &p : config.thirdPartyIncludes) {
        if (!p.empty() && path.find(p) != std::string::npos)
            return true;
    }

    return false;
}

bool GlobalVariableRule::shouldIgnore(
    const SourceManager &sm,
    SourceLocation loc) const
{
    if (loc.isInvalid())
        return true;

    const SourceLocation expansion = sm.getExpansionLoc(loc);

    if (suppressions.isSuppressed(sm, expansion))
        return true;

    if (sm.isInSystemHeader(expansion))
        return true;

    const std::string path = sm.getFilename(expansion).str();

    return !path.empty() && isThirdParty(path);
}

bool GlobalVariableRule::isDeepConst(
    QualType type,
    const ASTContext &context) const
{
    // An array is const when its elements are
    if (const ArrayType *array = context.getAsArrayType(type))
        return isDeepConst(array->getElementType(), context);

    // What a function pointer points to can not be const
    if (type->isFunctionType())
        return true;

    if (!type.isConstQualified())
        return false;

    if (const auto *pointer = type->getAs<PointerType>())
        return isDeepConst(pointer->getPointeeType(), context);

    return true;
}

void GlobalVariableRule::report(
    DiagCode code,
    const SourceManager &sm,
    const VarDecl *var,
    const std::string &kind,
    const std::string &message)
{
    diagnostics.report(
        config.globalVariableRule.level,
        code,
        sm,
        sm.getExpansionLoc(var->getLocation()),
        kind + " '" + var->getNameAsString() + "' " + message
    );
}

GlobalVariableRule::GlobalVariableRule(const Config &cfg,
                                       SuppressionManager &sup,
                                       Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag) {}

void GlobalVariableRule::bindFinder(MatchFinder &finder) {
    finder.addMatcher(
        varDecl(
            hasGlobalStorage(),
            unless(isExpansionInSystemHeader())
        ).bind("var"),
        this
    );
}

void GlobalVariableRule::run(const MatchFinder::MatchResult &result) {
    const auto &cfg = config.globalVariableRule;

    if (cfg.level == RuleLevel::Off)
        return;

    const auto *var = result.Nodes.getNodeAs<VarDecl>("var");

    if (!var)
        return;

    const bool isStaticLocal = var->isStaticLocal();

    // Variables at file scope and function-local statics are both checked.
    if (!isStaticLocal && !var->isFileVarDecl()) {
        return;
    }

    // Check each variable once: at its definition, at its tentative
    // definition ('int x;'), or otherwise at its first declaration.
    const VarDecl *representative = var->getDefinition();

    if (!representative)
        representative = var->getActingDefinition();

    if (!representative)
        representative = var->getCanonicalDecl();

    if (var != representative)
        return;

    const SourceManager &sm = *result.SourceManager;

    if (shouldIgnore(sm, var->getLocation()))
        return;

    const std::string kind =
        isStaticLocal ? "static local variable" : "global variable";

    const bool isConst = isDeepConst(var->getType(), *result.Context);
    const bool prefixRequired = isStaticLocal
        ? (isConst
            ? cfg.constLocalStaticRequirePrefix
            : cfg.mutableLocalStaticRequirePrefix)
        : (isConst ? cfg.constRequirePrefix : cfg.mutableRequirePrefix);
    const std::string &prefix = isStaticLocal
        ? (isConst ? cfg.constLocalStaticPrefix : cfg.mutableLocalStaticPrefix)
        : (isConst ? cfg.constPrefix : cfg.mutablePrefix);
    const bool mustBeCaps = isStaticLocal
        ? (isConst
            ? cfg.constLocalStaticMustBeCaps
            : cfg.mutableLocalStaticMustBeCaps)
        : (isConst ? cfg.constMustBeCaps : cfg.mutableMustBeCaps);

    const std::string name = var->getNameAsString();

    std::string nameAfterPrefix = name;
    bool hasPrefix = false;

    if (prefixRequired) {
        if (name.rfind(prefix, 0) != 0) {
            report(DiagCode::GlobalMissingPrefix, sm, var, kind,
                "must start with the prefix '" + prefix + "'");
        }
        else {
            nameAfterPrefix = name.substr(prefix.size());
            hasPrefix = true;
        }
    }

    if (mustBeCaps) {
        bool hasLowercase = false;

        for (unsigned char c : nameAfterPrefix) {
            if (std::islower(c))
                hasLowercase = true;
        }

        if (hasLowercase) {
            report(DiagCode::GlobalNotCapitals, sm, var, kind,
                hasPrefix
                    ? "must be written in capital letters after the prefix '" +
                      prefix + "'"
                    : std::string("must be written in capital letters"));
        }
    }

    // A static local is always static
    if (!isStaticLocal &&
        cfg.mustBeStatic &&
        var->getStorageClass() != SC_Static)
    {
        report(DiagCode::GlobalNotStatic, sm, var, kind, "must be static");
    }

    if (!isStaticLocal && cfg.mustBeConst &&
        !isDeepConst(var->getType(), *result.Context))
    {
        const bool involvesPointer =
            var->getType()->isPointerType() ||
            result.Context->getBaseElementType(var->getType())->isPointerType();

        report(DiagCode::GlobalNotConst, sm, var, kind,
            involvesPointer
                ? "must be const, and so must everything it points to "
                  "(e.g. 'const int* const')"
                : "must be const");
    }

    // A static global in a header gives every file that includes the
    // header its own separate copy of the variable. The same goes for a
    // static local in a function defined in a header, so it is checked
    // too when static locals are treated as globals.
    if (cfg.forbidStaticInHeader &&
        var->getStorageClass() == SC_Static &&
        !sm.isInMainFile(sm.getExpansionLoc(var->getLocation())))
    {
        report(DiagCode::GlobalStaticInHeader, sm, var, kind,
            "may not be static in a header, every file that includes "
            "the header would get its own copy of it");
    }
}
