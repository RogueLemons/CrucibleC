#include "rule_function_pointer.hpp"

bool FunctionPointerRule::isThirdParty(const std::string &path) const {
    for (const auto &p : config.thirdPartyIncludes) {
        if (path.find(p) != std::string::npos)
            return true;
    }
    return false;
}

bool FunctionPointerRule::isFunctionPointer(QualType qt) const {
    if (qt.isNull())
        return false;

    const Type *t = qt.getTypePtrOrNull();

    // Through arrays and pointers, e.g. an array of function pointers
    // or a pointer to a function pointer
    while (t) {
        t = t->getUnqualifiedDesugaredType();

        if (const auto *array = dyn_cast<ArrayType>(t)) {
            t = array->getElementType().getTypePtrOrNull();
            continue;
        }

        if (const auto *ptr = dyn_cast<PointerType>(t)) {
            const Type *pt = ptr->getPointeeType().getTypePtrOrNull();

            if (!pt)
                return false;

            if (pt->getUnqualifiedDesugaredType()->isFunctionType())
                return true;

            t = pt;
            continue;
        }

        return false;
    }

    return false;
}

bool FunctionPointerRule::isTypedefSpelled(TypeLoc TL) const {
    // Walk through type locations to find typedef spelling
    while (!TL.isNull()) {
        if (TL.getTypeLocClass() == TypeLoc::Typedef)
            return true;

        // Past the function type the walk continues into its return
        // type, e.g. the 'size_t' of 'size_t (*f)(int)', which says
        // nothing about how the function pointer itself is written
        if (TL.getAs<FunctionTypeLoc>())
            return false;

        TL = TL.getNextTypeLoc();
    }

    return false;
}

std::string FunctionPointerRule::makeKey(const Decl *D, const SourceManager &sm) const {
    D = D->getCanonicalDecl();

    SourceLocation loc = D->getLocation();
    PresumedLoc ploc = sm.getPresumedLoc(loc);

    if (!ploc.isValid())
        return "";

    return std::string(ploc.getFilename()) + ":" +
           std::to_string(ploc.getLine()) + ":" +
           D->getDeclKindName();
}

FunctionPointerRule::FunctionPointerRule(const Config &cfg,
                    SuppressionManager &sup,
                    Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag) {}

void FunctionPointerRule::bindFinder(MatchFinder &finder) {
    // Variables, including parameters, and struct fields
    finder.addMatcher(
        declaratorDecl(
            anyOf(varDecl(), fieldDecl())
        ).bind("funcptr"),
        this
    );

    // Return types
    finder.addMatcher(
        functionDecl().bind("function"),
        this
    );
}

void FunctionPointerRule::run(const MatchFinder::MatchResult &result) {
    if (config.functionPointerRule.level == RuleLevel::Off)
        return;

    const auto *declarator =
        result.Nodes.getNodeAs<DeclaratorDecl>("funcptr");

    const auto *function =
        result.Nodes.getNodeAs<FunctionDecl>("function");

    const Decl *decl = nullptr;
    QualType qt;
    TypeLoc typeLoc;

    if (declarator) {
        decl = declarator;
        qt = declarator->getType();

        if (const TypeSourceInfo *TSI = declarator->getTypeSourceInfo())
            typeLoc = TSI->getTypeLoc();
    }
    else if (function) {
        decl = function;
        qt = function->getReturnType();

        if (const FunctionTypeLoc FTL = function->getFunctionTypeLoc())
            typeLoc = FTL.getReturnLoc();
    }
    else {
        return;
    }

    if (decl->isImplicit())
        return;

    auto &sm = *result.SourceManager;

    // -------------------------
    // Location
    // -------------------------
    SourceLocation loc = decl->getLocation();
    bool fromMacro = loc.isMacroID();

    SourceLocation spellingLoc = sm.getSpellingLoc(loc);
    SourceLocation expansionLoc = sm.getExpansionLoc(loc);

    // -------------------------
    // Suppression
    // -------------------------
    if (suppressions.isSuppressed(sm, expansionLoc))
        return;

    if (sm.isInSystemHeader(spellingLoc))
        return;

    std::string spellingPath = sm.getFilename(spellingLoc).str();
    std::string expansionPath = sm.getFilename(expansionLoc).str();

    if (!spellingPath.empty() && isThirdParty(spellingPath))
        return;

    if (!expansionPath.empty() && isThirdParty(expansionPath))
        return;

    // -------------------------
    // Must be function pointer
    // -------------------------
    if (!isFunctionPointer(qt))
        return;

    // -------------------------
    // Typedef exception
    // -------------------------
    if (typeLoc.isNull() || isTypedefSpelled(typeLoc))
        return;

    // -------------------------
    // Dedup
    // -------------------------
    std::string key = makeKey(decl, sm);

    if (!key.empty()) {
        if (reported.count(key))
            return;

        reported.insert(key);
    }

    // -------------------------
    // Message
    // -------------------------
    std::string name;

    if (const auto *named = dyn_cast<NamedDecl>(decl))
        name = named->getNameAsString();

    std::string msg;

    if (function)
        msg = "function '" + name + "' must return a function pointer "
              "declared using a typedef";
    else if (name.empty())
        msg = "function pointer must be declared using a typedef";
    else
        msg = "function pointer '" + name + "' must be declared using a typedef";

    if (fromMacro)
        msg += " (macro expansion)";

    diagnostics.report(
        config.functionPointerRule.level,
        DiagCode::FunctionPointerMissingTypedef,
        sm,
        expansionLoc,
        msg
    );
}
