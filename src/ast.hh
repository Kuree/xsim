#ifndef XSIM_AST_HH
#define XSIM_AST_HH

#include <unordered_set>

#include "slang/compilation/Compilation.h"
#include "slang/symbols/ASTVisitor.h"

namespace xsim {

// visit slang AST nodes
class ModuleDefinitionVisitor : public slang::ASTVisitor<ModuleDefinitionVisitor, false, false> {
public:
    ModuleDefinitionVisitor() = default;

    // only visit modules
    [[maybe_unused]] void handle(const slang::InstanceSymbol &symbol);

    std::unordered_map<std::string_view, const slang::Definition *> modules;
};

// compute complexity for each module definition. small module will be inlined into its parent
// module
class ModuleComplexityVisitor : public slang::ASTVisitor<ModuleComplexityVisitor, true, true> {
    // see
    // https://clang.llvm.org/extra/clang-tidy/checks/readability-function-cognitive-complexity.html
};

class DependencyAnalysisVisitor : public slang::ASTVisitor<DependencyAnalysisVisitor, true, true> {
public:
    struct Node {
        explicit Node(const slang::Symbol &symbol) : symbol(symbol) {}
        // double linked graph
        std::unordered_set<const Node *> edges_to;
        std::unordered_set<const Node *> edges_from;

        const slang::Symbol &symbol;
    };

    struct Graph {
    public:
        std::vector<std::unique_ptr<Node>> nodes;
        std::unordered_map<std::string, Node *> node_mapping;

        Node *get_node(const slang::NamedValueExpression *name);
        Node *get_node(const std::string &name) const;
        Node *get_node(const slang::Symbol &symbol);

    private:
        uint64_t procedural_blk_count_ = 0;
        std::unordered_map<const slang::Symbol *, std::string> new_names_;
    };

    DependencyAnalysisVisitor();
    explicit DependencyAnalysisVisitor(Graph *graph) : graph(graph) {}

    [[maybe_unused]] void handle(const slang::ContinuousAssignSymbol &stmt);
    [[maybe_unused]] void handle(const slang::ProceduralBlockSymbol &stmt);

    Graph *graph;

    std::string error;

private:
    std::unique_ptr<Graph> graph_;
};

}  // namespace xsim

#endif  // XSIM_AST_HH
