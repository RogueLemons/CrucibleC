#include "rule_arg_ptr_move.hpp"

#include "tag_placement.hpp"

#include <sstream>

bool ArgumentPointerMovementRule::isNonConstPointer(QualType qt) const {
    const auto *ptr = qt->getAs<PointerType>();
    if (!ptr) return false;

    // A function can not be const, and a function pointer does not
    // point to data that can be moved, written out or modified
    if (ptr->getPointeeType()->isFunctionType())
        return false;

    return !ptr->getPointeeType().isConstQualified();
}

bool ArgumentPointerMovementRule::isThirdParty(const std::string &path) const {
    for (const auto &p : config.thirdPartyIncludes) {
        if (path.find(p) != std::string::npos)
            return true;
    }
    return false;
}

std::string ArgumentPointerMovementRule::getTag(const ParmVarDecl *param) const {
    for (const auto *attr : param->attrs()) {
        if (const auto *A = dyn_cast<AnnotateAttr>(attr)) {
            StringRef t = A->getAnnotation();
            if (t == kMoveTag || t == kOutTag || t == kModTag)
                return t.str();
        }
    }
    return "";
}

std::string ArgumentPointerMovementRule::makeKey(const FunctionDecl *FD) const {
    std::ostringstream oss;

    oss << FD->getNameAsString() << "(";

    bool first = true;
    for (const ParmVarDecl *P : FD->parameters()) {
        if (!first) oss << ",";
        first = false;
        oss << P->getType().getAsString();
    }

    oss << ")";
    return oss.str();
}

void ArgumentPointerMovementRule::report(const std::string &msg,
            const ParmVarDecl *P,
            const SourceManager &sm,
            SourceLocation loc)
{
    diagnostics.report(
        config.argumentPointerMovementRule.level,
        sm,
        loc,
        msg
    );
}

bool ArgumentPointerMovementRule::isWorkshopCInternal(const FunctionDecl *FD) const {
    if (!FD) return false;

    std::string name = FD->getNameAsString();

    return name == "workshopc_move"
        || name == "workshopc_out"
        || name == "workshopc_modify";
}

ArgumentPointerMovementRule::ArgumentPointerMovementRule(
    const Config &cfg,
    SuppressionManager &sup,
    Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag)
{}

void ArgumentPointerMovementRule::checkTagPlacement(
    const Decl *D,
    const SourceManager &sm)
{
    if (!D || D->isImplicit())
        return;

    const auto *param = dyn_cast<ParmVarDecl>(D);

    // Tags belong on the parameters of function declarations
    if (param && isParameterOfFunctionDecl(param))
        return;

    const SourceLocation expansionLoc = sm.getExpansionLoc(D->getLocation());

    if (expansionLoc.isInvalid() ||
        suppressions.isSuppressed(sm, expansionLoc) ||
        sm.isInSystemHeader(expansionLoc))
        return;

    const std::string path = sm.getFilename(expansionLoc).str();

    if (!path.empty() && isThirdParty(path))
        return;

    for (const auto *attr : D->attrs()) {
        const auto *A = dyn_cast<AnnotateAttr>(attr);

        // Inherited attributes were already reported where written
        if (!A || A->isInherited())
            continue;

        const StringRef t = A->getAnnotation();

        if (t != kMoveTag && t != kOutTag && t != kModTag)
            continue;

        std::string name = "<unnamed>";

        if (const auto *ND = dyn_cast<NamedDecl>(D)) {
            if (!ND->getName().empty())
                name = ND->getNameAsString();
        }

        diagnostics.report(
            config.argumentPointerMovementRule.level,
            sm,
            expansionLoc,
            param
                ? "movement attribute (" + t.str() + ") on '" + name +
                  "' may not be used on a parameter of a function pointer "
                  "type, calls through function pointers are not checked"
                : "movement attribute (" + t.str() + ") on '" + name +
                  "' may only be used on function parameters"
        );
    }
}

void ArgumentPointerMovementRule::bindFinder(MatchFinder &finder) {
    finder.addMatcher(
        decl(
            hasAttr(clang::attr::Annotate),
            unless(isExpansionInSystemHeader())
        ).bind("taggedDecl"),
        this
    );

    finder.addMatcher(
        functionDecl(
            unless(isExpansionInSystemHeader())
        ).bind("func"),
        this
    );

    finder.addMatcher(
        callExpr(
            unless(isExpansionInSystemHeader())
        ).bind("call"),
        this
    );
}

void ArgumentPointerMovementRule::run(const MatchFinder::MatchResult &result) {

    if (const auto *tagged = result.Nodes.getNodeAs<Decl>("taggedDecl")) {
        checkTagPlacement(tagged, *result.SourceManager);
        return;
    }

    const auto *FD =
        result.Nodes.getNodeAs<FunctionDecl>("func");

    if (!FD || FD->isImplicit())
        return;

    // =====================================================
    // 🔥 IMPORTANT: SKIP INTERNAL HELPER FUNCTIONS
    // =====================================================
    if (isWorkshopCInternal(FD))
        return;

    auto &sm = *result.SourceManager;

    SourceLocation loc = FD->getLocation();
    SourceLocation expansionLoc = sm.getExpansionLoc(loc);

    // -------------------------
    // suppression
    // -------------------------
    if (suppressions.isSuppressed(sm, expansionLoc))
        return;

    // -------------------------
    // system header filter
    // -------------------------
    if (sm.isInSystemHeader(expansionLoc))
        return;

    // -------------------------
    // third-party filter
    // -------------------------
    std::string expansionPath =
        sm.getFilename(expansionLoc).str();

    if (!expansionPath.empty() &&
        isThirdParty(expansionPath))
    {
        return;
    }

    // -------------------------
    // process parameters
    // -------------------------
    std::vector<std::string> tags;

    for (const ParmVarDecl *P : FD->parameters()) {

        QualType qt = P->getType();
        std::string tag;

        if (qt->isPointerType() && isNonConstPointer(qt)) {

            for (const auto *attr : P->attrs()) {
                if (const auto *A = dyn_cast<AnnotateAttr>(attr)) {
                    StringRef t = A->getAnnotation();
                    if (t == kMoveTag || t == kOutTag || t == kModTag)
                        tag = t.str();
                }
            }

            if (tag.empty()) {
                report(
                    "function '" + FD->getNameAsString() +
                    "' parameter '" + P->getNameAsString() +
                    "' uses non-const pointer; movement attribute required "
                    "(workshopc_move, workshopc_out, workshopc_modify)",
                    P,
                    sm,
                    expansionLoc
                );
            }
        }

        tags.push_back(tag);
    }

    // -------------------------
    // store declaration state
    // -------------------------
    std::string key = makeKey(FD);
    auto &state = functions[key];

    if (!FD->isThisDeclarationADefinition()) {
        state.declTags = tags;
        state.hasDecl = true;
        return;
    }

    // -------------------------
    // definition check
    // -------------------------
    if (!state.hasDecl)
        return;

    if (state.declTags.size() != tags.size())
        return;

    for (size_t i = 0; i < tags.size(); i++) {

        if (state.declTags[i] != tags[i]) {

            const ParmVarDecl *P = FD->parameters()[i];

            report(
                "function '" + FD->getNameAsString() +
                "' parameter '" + P->getNameAsString() +
                "' movement attribute mismatch between declaration and definition",
                P,
                sm,
                expansionLoc
            );
        }
    }
}
