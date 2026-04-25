#include "types.h"
#include "utils.h"
#include "ast_parser.h"
#include "code_gen.h"
#include <clang-c/Rewrite.h>
#include <iostream>
#include <cstdio>
#include <regex>

int main(int argc, char *argv[]) {
  // Parse command line arguments
  CommandLineArgs args;
  if (!parseArguments(argc, argv, args)) {
    return 1;
  }

  // Revert mode: strip previously injected blocks and exit
  if (args.revert) {
    std::string content = readFile(args.inputSourceFile);
    std::regex block(
        R"(\n?/\* FHI_INJECT_BEGIN \*/[\s\S]*?/\* FHI_INJECT_END \*/\n)");
    std::string reverted = std::regex_replace(content, block, "");
    if (args.outputFile == "/dev/stdout") {
      std::cout << reverted;
    } else if (!writeFile(args.outputFile, reverted)) {
      return 1;
    }
    return 0;
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
    std::string formattedHeader = "/* FHI_INJECT_BEGIN */\n";
    formattedHeader += headerCode;
    if (formattedHeader.back() != '\n') {
      formattedHeader += "\n";
    }
    formattedHeader += "/* FHI_INJECT_END */\n";

    CXSourceLocation fileStart =
        clang_getLocationForOffset(translationUnit, inputFile, 0);
    clang_CXRewriter_insertTextBefore(rewriter, fileStart,
                                      formattedHeader.c_str());
  }

  // Inject hook code into each function body
  for (const auto &func : functionLocations) {
    unsigned int funcLines = func.endLine - func.startLine + 1;
    if (args.minLines > 0 && funcLines < static_cast<unsigned>(args.minLines)) {
      std::cerr << "Skipping short function: " << func.functionName
                << " (" << funcLines << " lines)" << std::endl;
      continue;
    }

    std::string hookCode =
        generateHookCode(func, injectionCode, args.injectionMode);
    if (hookCode.empty())
      continue;

    std::string injection = "\n/* FHI_INJECT_BEGIN */\n";
    injection += hookCode;
    if (injection.back() != '\n') injection += "\n";
    injection += "/* FHI_INJECT_END */\n";

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
