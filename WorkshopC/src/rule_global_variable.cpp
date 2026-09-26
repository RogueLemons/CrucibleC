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
    const SourceManager &sm,
    const VarDecl *var,
    const std::string &message)
{
    diagnostics.report(
        config.globalVariableRule.level,
        sm,
        sm.getExpansionLoc(var->getLocation()),
        "global variable '" + var->getNameAsString() + "' " + message
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

    // Only variables at file scope, not static locals
    if (!var || !var->isFileVarDecl())
        return;

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

    const std::string name = var->getNameAsString();

    std::string nameAfterPrefix = name;

    if (cfg.requirePrefix) {
        if (name.rfind(cfg.prefix, 0) != 0) {
            report(sm, var,
                "must start with the prefix '" + cfg.prefix + "'");
        }
        else {
            nameAfterPrefix = name.substr(cfg.prefix.size());
        }
    }

    if (cfg.mustBeCaps) {
        bool hasLowercase = false;

        for (unsigned char c : nameAfterPrefix) {
            if (std::islower(c))
                hasLowercase = true;
        }

        if (hasLowercase) {
            report(sm, var,
                cfg.requirePrefix
                    ? "must be written in capital letters after the prefix '" +
                      cfg.prefix + "'"
                    : std::string("must be written in capital letters"));
        }
    }

    if (cfg.mustBeStatic && var->getStorageClass() != SC_Static) {
        report(sm, var, "must be static");
    }

    if (cfg.mustBeConst &&
        !isDeepConst(var->getType(), *result.Context))
    {
        const bool involvesPointer =
            var->getType()->isPointerType() ||
            result.Context->getBaseElementType(var->getType())->isPointerType();

        report(sm, var,
            involvesPointer
                ? "must be const, and so must everything it points to "
                  "(e.g. 'const int* const')"
                : "must be const");
    }
}
