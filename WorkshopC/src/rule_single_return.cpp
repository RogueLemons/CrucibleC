#include "rule_single_return.hpp"

bool SingleReturnRule::isThirdParty(const std::string &path) const {
    for (const auto &p : config.thirdPartyIncludes) {
        if (!p.empty() && path.find(p) != std::string::npos)
            return true;
    }

    return false;
}

bool SingleReturnRule::shouldIgnore(
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

void SingleReturnRule::collectReturns(
    const Stmt *stmt,
    std::vector<const ReturnStmt *> &returns) const
{
    if (!stmt)
        return;

    if (const auto *ret = dyn_cast<ReturnStmt>(stmt))
        returns.push_back(ret);

    for (const Stmt *child : stmt->children())
        collectReturns(child, returns);
}

void SingleReturnRule::collectEarlyReturns(
    const Stmt *topLevel,
    std::unordered_set<const ReturnStmt *> &allowed) const
{
    if (!topLevel)
        return;

    // A macro used as a statement, e.g. RETURN_IF_NULL(p);
    if (topLevel->getBeginLoc().isMacroID()) {
        std::vector<const ReturnStmt *> returns;
        collectReturns(topLevel, returns);
        allowed.insert(returns.begin(), returns.end());
        return;
    }

    const auto *ifStmt = dyn_cast<IfStmt>(topLevel);

    if (!ifStmt || ifStmt->getElse())
        return;

    const Stmt *branch = ifStmt->getThen();

    // if (cond) return x;
    if (const auto *ret = dyn_cast_or_null<ReturnStmt>(branch)) {
        allowed.insert(ret);
        return;
    }

    // if (cond) { ...; return x; }
    if (const auto *block = dyn_cast_or_null<CompoundStmt>(branch)) {
        if (const auto *ret =
                dyn_cast_or_null<ReturnStmt>(block->body_back()))
            allowed.insert(ret);
    }
}

SingleReturnRule::SingleReturnRule(const Config &cfg,
                                   SuppressionManager &sup,
                                   Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag) {}

void SingleReturnRule::bindFinder(MatchFinder &finder) {
    finder.addMatcher(
        functionDecl(
            isDefinition(),
            unless(isExpansionInSystemHeader())
        ).bind("function"),
        this
    );
}

void SingleReturnRule::run(const MatchFinder::MatchResult &result) {
    const auto &cfg = config.singleReturnRule;

    if (cfg.level == RuleLevel::Off)
        return;

    const auto *fn = result.Nodes.getNodeAs<FunctionDecl>("function");

    if (!fn || !fn->doesThisDeclarationHaveABody())
        return;

    const auto *body = dyn_cast_or_null<CompoundStmt>(fn->getBody());

    if (!body)
        return;

    const SourceManager &sm = *result.SourceManager;

    if (shouldIgnore(sm, fn->getLocation()))
        return;

    const std::string name = fn->getNameAsString();
    const bool isVoid = fn->getReturnType()->isVoidType();

    std::vector<const ReturnStmt *> returns;
    collectReturns(body, returns);

    std::unordered_set<const ReturnStmt *> allowed;

    // The final return, the last statement of the function body
    const auto *finalReturn =
        body->body_empty()
            ? nullptr
            : dyn_cast<ReturnStmt>(body->body_back());

    if (finalReturn)
        allowed.insert(finalReturn);

    if (cfg.allowEarlyReturn) {
        for (const Stmt *topLevel : body->body()) {
            if (topLevel != finalReturn)
                collectEarlyReturns(topLevel, allowed);
        }
    }

    const std::string allowedText =
        cfg.allowEarlyReturn
            ? "only early returns directly in the function's top-level "
              "block and one final return are allowed"
            : "only one final return is allowed";

    for (const ReturnStmt *ret : returns) {
        if (allowed.count(ret))
            continue;

        if (shouldIgnore(sm, ret->getReturnLoc()))
            continue;

        diagnostics.report(
            cfg.level,
            DiagCode::MultipleReturns,
            sm,
            sm.getExpansionLoc(ret->getReturnLoc()),
            "function '" + name + "' has more than a single return, " +
            allowedText
        );
    }

    const bool finalReturnRequired = !isVoid || cfg.requireReturnForVoid;

    if (finalReturnRequired && !finalReturn &&
        !shouldIgnore(sm, body->getRBracLoc()))
    {
        diagnostics.report(
            cfg.level,
            DiagCode::MissingFinalReturn,
            sm,
            sm.getExpansionLoc(body->getRBracLoc()),
            "function '" + name + "' must end with a return statement"
        );
    }
}
