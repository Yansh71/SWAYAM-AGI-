#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include <iostream>

using namespace clang;
using namespace clang::tooling;

// Sovereign AST Function Visitor: Scans and identifies logical segments (functions)
class SovereignFunctionVisitor : public RecursiveASTVisitor<SovereignFunctionVisitor> {
public:
    explicit SovereignFunctionVisitor(ASTContext *Context) : Context(Context) {}

    bool VisitFunctionDecl(FunctionDecl *FD) {
        // Focus purely on our own source files, filtering out system/library headers
        if (!Context->getSourceManager().isInMainFile(FD->getLocation()))
            return true;

        if (FD->hasBody()) {
            SourceRange range = FD->getSourceRange();
            llvm::outs() << "[SWAYAM-CORE] Discovered Logical Segment -> Function: " 
                         << FD->getNameAsString()
                         << " | Range: [" << range.getBegin().printToString(Context->getSourceManager())
                         << " -> " << range.getEnd().printToString(Context->getSourceManager())
                         << "]\n";
        }
        return true; 
    }

private:
    ASTContext *Context;
};

class SovereignASTConsumer : public ASTConsumer {
public:
    explicit SovereignASTConsumer(ASTContext *Context) : Visitor(Context) {}
    void HandleTranslationUnit(ASTContext &Context) override {
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }

private:
    SovereignFunctionVisitor Visitor;
};

class SovereignFrontendAction : public ASTFrontendAction {
public:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(
        CompilerInstance &Compiler, llvm::StringRef InFile) override {
        return std::make_unique<SovereignASTConsumer>(&Compiler.getASTContext());
    }
};

static llvm::cl::OptionCategory SovereignToolCategory("swayam-ast-scanner options");

int main(int argc, const char **argv) {
    std::cout << "========================================\n";
    std::cout << "  SWAYAM-AGI: SOVEREIGN AST SCANNER v1.0\n";
    std::cout << "========================================\n";

    auto ExpectedParser = CommonOptionsParser::create(argc, argv, SovereignToolCategory);
    if (!ExpectedParser) {
        llvm::errs() << "Error parsing tool options.\n";
        return 1;
    }

    CommonOptionsParser &OptionsParser = ExpectedParser.get();
    ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());

    // Execute the AST traversal and print proof-of-life verification
    return Tool.run(newFrontendActionFactory<SovereignFrontendAction>().get());
}
