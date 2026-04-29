#include "types.h"
#include "utils.h"
#include "ast_parser.h"
#include "code_gen.h"
#include <clang-c/Rewrite.h>
#include <iostream>
#include <cstdio>
#include <regex>

/**
 * Revert injected code from the source file
 */
int handleRevert(const CommandLineArgs& args) {
    std::string content = readFile(args.inputSourceFile);
    if (content.empty()) return 1;

    // Escape markers for regex
    std::string beginMarker = std::regex_replace(INJECT_BEGIN, std::regex(R"(\*)"), R"(\*)");
    std::string endMarker = std::regex_replace(INJECT_END, std::regex(R"(\*)"), R"(\*)");
    std::string pattern = R"(\n?)" + beginMarker + R"([\s\S]*?)" + endMarker + R"(\n)";
    
    std::regex block(pattern);
    std::string reverted = std::regex_replace(content, block, "");
    
    if (args.outputFile == "/dev/stdout") {
        std::cout << reverted;
    } else if (!writeFile(args.outputFile, reverted)) {
        return 1;
    }
    return 0;
}

/**
 * Inject common header code at the top of the file
 */
void injectHeader(CXRewriter rewriter, CXTranslationUnit tu, CXFile file, const std::string& headerCode) {
    std::string formattedHeader = INJECT_BEGIN + "\n";
    formattedHeader += headerCode;
    if (formattedHeader.back() != '\n') {
        formattedHeader += "\n";
    }
    formattedHeader += INJECT_END + "\n";

    CXSourceLocation fileStart = clang_getLocationForOffset(tu, file, 0);
    clang_CXRewriter_insertTextBefore(rewriter, fileStart, formattedHeader.c_str());
}

/**
 * Inject hook code into a specific function
 */
void injectFunctionHook(CXRewriter rewriter, CXTranslationUnit tu, CXFile file, 
                        const FunctionLocation& func, const std::string& injectionCode, 
                        const CommandLineArgs& args) {
    unsigned int funcLines = func.endLine - func.startBodyLine + 1;
    if (args.minLines > 0 && funcLines < static_cast<unsigned>(args.minLines)) {
        std::cerr << "Skipping short function: " << func.functionName
                  << " (" << funcLines << " lines)" << std::endl;
        return;
    }

    std::string hookCode = generateHookCode(func, injectionCode, args.injectionMode);
    if (hookCode.empty()) return;

    std::string injection = "\n" + INJECT_BEGIN + "\n";
    injection += hookCode;
    if (injection.back() != '\n') injection += "\n";
    injection += INJECT_END + "\n";

    CXSourceLocation loc = clang_getLocationForOffset(tu, file, func.bodyStartOffset);
    clang_CXRewriter_insertTextBefore(rewriter, loc, injection.c_str());
    std::cerr << "Injected code into function: " << func.functionName << std::endl;
}

int main(int argc, char *argv[]) {
    CommandLineArgs args;
    if (!parseArguments(argc, argv, args)) return 1;

    if (args.revert) return handleRevert(args);

    // Read necessary files
    std::string injectionCode = args.hookContentFile.empty() ? "" : readFile(args.hookContentFile);
    while (!injectionCode.empty() && injectionCode.back() == '\n') injectionCode.pop_back();

    std::string headerCode = args.headerContentFile.empty() ? "" : readFile(args.headerContentFile);

    // Initialize libclang
    CXIndex index = clang_createIndex(1, 0);
    CXTranslationUnit tu = parseSourceFile(index, args.inputSourceFile);
    if (!tu) {
        clang_disposeIndex(index);
        return 1;
    }

    CXFile inputFile = clang_getFile(tu, args.inputSourceFile.c_str());
    if (!inputFile) {
        std::cerr << "Error: Unable to get CXFile for " << args.inputSourceFile << std::endl;
        clang_disposeTranslationUnit(tu);
        clang_disposeIndex(index);
        return 1;
    }

    // Find functions
    std::vector<std::regex> excludePatterns = readExcludePatterns(args.excludePatternFile);
    std::vector<FunctionLocation> functionLocations;
    VisitorClientData visitorData = {&functionLocations, inputFile, &excludePatterns};

    CXCursor rootCursor = clang_getTranslationUnitCursor(tu);
    clang_visitChildren(rootCursor, functionVisitor, &visitorData);

    std::cerr << "\nTotal functions found: " << functionLocations.size() << std::endl;

    // Perform rewriting
    CXRewriter rewriter = clang_CXRewriter_create(tu);

    if (!headerCode.empty() || !functionLocations.empty()) {
        injectHeader(rewriter, tu, inputFile, headerCode);
    }

    for (const auto& func : functionLocations) {
        injectFunctionHook(rewriter, tu, inputFile, func, injectionCode, args);
    }

    // Output results
    if (args.outputFile != "/dev/stdout") {
        if (!std::freopen(args.outputFile.c_str(), "w", stdout)) {
            std::cerr << "Error: Failed to open output file: " << args.outputFile << std::endl;
            // Cleanup and exit
        }
    }

    clang_CXRewriter_writeMainFileToStdOut(rewriter);

    if (args.outputFile != "/dev/stdout") {
        std::cerr << "\nModified source written to: " << args.outputFile << std::endl;
    }

    // Cleanup
    clang_CXRewriter_dispose(rewriter);
    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);

    return 0;
}
