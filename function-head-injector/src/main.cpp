#include "types.h"
#include "utils.h"
#include "ast_parser.h"
#include "code_gen.h"
#include <clang-c/Rewrite.h>
#include <iostream>
#include <cstdio>

int main(int argc, char *argv[]) {
  // Parse command line arguments
  CommandLineArgs args;
  if (!parseArguments(argc, argv, args)) {
    return 1;
  }

  // Read code to inject
  std::string injectionCode;
  if (!args.hookContentFile.empty()) {
    injectionCode = readFile(args.hookContentFile);
    if (injectionCode.empty()) {
      std::cerr << "Warning: Hook content file is empty or could not be read: "
                << args.hookContentFile << std::endl;
    }
  }

  // Remove trailing newlines from injection code (for cleaner formatting)
  while (!injectionCode.empty() && injectionCode.back() == '\n') {
    injectionCode.pop_back();
  }

  // Read header injection code
  std::string headerCode;
  if (!args.headerContentFile.empty()) {
    headerCode = readFile(args.headerContentFile);
    if (headerCode.empty()) {
      std::cerr
          << "Warning: Header content file is empty or could not be read: "
          << args.headerContentFile << std::endl;
    }
  }

  // Create libclang index
  // First argument: Exclude declarations from PCH (PreCompiledHeader)
  // Second argument: Don't display diagnostic (0)
  CXIndex index = clang_createIndex(1, 0);

  // Parse source file and build AST
  CXTranslationUnit translationUnit =
      parseSourceFile(index, args.inputSourceFile);
  if (translationUnit == nullptr) {
    clang_disposeIndex(index);
    return 1;
  }

  // Get handle for input file (to identify file during function visiting)
  CXFile inputFile =
      clang_getFile(translationUnit, args.inputSourceFile.c_str());
  if (!inputFile) {
    std::cerr << "Error: Unable to get CXFile for input file: "
              << args.inputSourceFile << std::endl;
    clang_disposeTranslationUnit(translationUnit);
    clang_disposeIndex(index);
    return 1;
  }

  // Read exclude patterns from file (empty if no file specified)
  std::vector<std::regex> excludePatterns =
      readExcludePatterns(args.excludePatternFile);

  // Prepare client data for visitor
  std::vector<FunctionLocation> functionLocations;
  VisitorClientData visitorData = {&functionLocations, inputFile,
                                   &excludePatterns};

  // Visit all nodes from AST root to find functions
  CXCursor rootCursor = clang_getTranslationUnitCursor(translationUnit);
  clang_visitChildren(rootCursor, functionVisitor, &visitorData);

  if (functionLocations.empty()) {
    std::cerr << "No functions found in the source file." << std::endl;
  } else {
    std::cerr << "\nTotal functions found: " << functionLocations.size()
              << std::endl;
  }

  // Create CXRewriter for source-to-source transformation
  CXRewriter rewriter = clang_CXRewriter_create(translationUnit);

  // Inject header code at the beginning of the file
  if (!headerCode.empty()) {
    std::string formattedHeader = headerCode;
    if (formattedHeader.back() != '\n') {
      formattedHeader += "\n";
    }
    formattedHeader += "\n"; // Add extra newline for separation

    CXSourceLocation fileStart =
        clang_getLocationForOffset(translationUnit, inputFile, 0);
    clang_CXRewriter_insertTextBefore(rewriter, fileStart,
                                      formattedHeader.c_str());
  }

  // Inject hook code into each function body
  for (const auto &func : functionLocations) {
    std::string hookCode =
        generateHookCode(func, injectionCode, args.injectionMode);
    if (hookCode.empty())
      continue;

    std::string injection = "\n" + hookCode;
    if (hookCode.back() != '\n') {
      injection += "\n";
    }

    CXSourceLocation loc = clang_getLocationForOffset(
        translationUnit, inputFile, func.bodyStartOffset);
    clang_CXRewriter_insertTextBefore(rewriter, loc, injection.c_str());
    std::cerr << "Injected code into function: " << func.functionName
              << std::endl;
  }

  // Redirect stdout to output file if specified
  if (args.outputFile != "/dev/stdout") {
    if (!std::freopen(args.outputFile.c_str(), "w", stdout)) {
      std::cerr << "Error: Failed to open output file: " << args.outputFile
                << std::endl;
      clang_CXRewriter_dispose(rewriter);
      clang_disposeTranslationUnit(translationUnit);
      clang_disposeIndex(index);
      return 1;
    }
  }

  // Write out the rewritten source
  clang_CXRewriter_writeMainFileToStdOut(rewriter);

  if (args.outputFile != "/dev/stdout") {
    std::cerr << "\nModified source written to: " << args.outputFile
              << std::endl;
  }

  // Clean up resources
  clang_CXRewriter_dispose(rewriter);
  clang_disposeTranslationUnit(translationUnit);
  clang_disposeIndex(index);

  return 0;
}
