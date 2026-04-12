#include "types.h"
#include "utils.h"
#include "ast_parser.h"
#include "code_gen.h"
#include <iostream>

bool processAndWriteOutput(
    const std::string &outputFile, const std::string &sourceContent,
    const std::vector<FunctionLocation> &functionLocations,
    const std::string &injectionCode, const std::string &headerCode,
    InjectionMode mode) {
  if (functionLocations.empty()) {
    std::cerr << "No functions found in the source file." << std::endl;
    return true; // Not an error even if no functions found
  }

  std::cerr << "\nTotal functions found: " << functionLocations.size()
            << std::endl;

  // Insert code
  std::string modifiedContent =
      injectCode(sourceContent, functionLocations, injectionCode, mode);

  // Inject header code if not empty
  if (!headerCode.empty()) {
    std::string formattedHeader = headerCode;
    if (formattedHeader.back() != '\n') {
      formattedHeader += "\n";
    }
    formattedHeader += "\n"; // Add extra newline for separation
    modifiedContent.insert(0, formattedHeader);
  }

  // Write to file
  if (!writeFile(outputFile, modifiedContent)) {
    std::cerr << "Error: Failed to write output file: " << outputFile
              << std::endl;
    return false;
  }

  // Display output destination if not stdout
  if (outputFile != "/dev/stdout") {
    std::cerr << "\nModified source written to: " << outputFile << std::endl;
  }

  return true;
}

int main(int argc, char *argv[]) {
  // Parse command line arguments
  CommandLineArgs args;
  if (!parseArguments(argc, argv, args)) {
    return 1;
  }

  // Read input source file
  std::string sourceContent = readFile(args.inputSourceFile);
  if (sourceContent.empty()) {
    std::cerr << "Error: Could not read input file: " << args.inputSourceFile
              << std::endl;
    return 1;
  }

  // Read code to inject
  std::string injectionCode;  // Code to inject
  if (!args.hookContentFile.empty()) {
      injectionCode = readFile(args.hookContentFile);
      if (injectionCode.empty()) {
          std::cerr << "Warning: Hook content file is empty or could not be read: " << args.hookContentFile << std::endl;
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
      std::cerr << "Warning: Header content file is empty or could not be read: "
                << args.headerContentFile << std::endl;
    }
  }

  // Create libclang index
  // First argument: Exclude declarations from PCH (PreCompiledHeader)
  // Second argument: Don't display diagnostic (0)
  CXIndex index = clang_createIndex(1, 0);

  // Parse source file and build AST
  CXTranslationUnit translationUnit = parseSourceFile(index, args.inputSourceFile);
  if (translationUnit == nullptr) {
    clang_disposeIndex(index);
    return 1;
  }

  // Get handle for input file (to identify file during function visiting)
  CXFile inputFile = clang_getFile(translationUnit, args.inputSourceFile.c_str());
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

  // Process results and output to file
  bool success =
      processAndWriteOutput(args.outputFile, sourceContent, functionLocations,
                            injectionCode, headerCode, args.injectionMode);

  // Clean up resources
  clang_disposeTranslationUnit(translationUnit);
  clang_disposeIndex(index);

  return success ? 0 : 1;
}
