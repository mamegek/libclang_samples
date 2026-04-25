#ifndef CORE_TYPES_H
#define CORE_TYPES_H

#include <string>
#include <vector>
#include <regex>
#include <clang-c/Index.h>

/**
 * Injection mode
 */
enum class InjectionMode {
    TEMPLATE,
    PRINTF,
    USDT
};

/**
 * Structure to manage command line arguments
 */
struct CommandLineArgs {
  std::string inputSourceFile;    // Input C/C++ source file
  std::string hookContentFile;    // File containing code to inject
  std::string outputFile;         // Output file (default is stdout)
  std::string excludePatternFile; // File containing regex patterns for functions to exclude
  std::string headerContentFile;  // File containing code to inject at the top of the file
  InjectionMode injectionMode;    // mode of injection: TEMPLATE, PRINTF, USDT
  int indentWidth;                // Indentation width for injected code (0 to disable)
  int minLines;                   // Minimum function lines to inject (0 to disable)
  bool inplace;                   // Overwrite input file
  bool revert;                    // Remove previously injected code
};

struct ArgumentInfo {
  std::string name;
  std::string type;
};

/**
 * Structure to hold function location information and name
 */
struct FunctionLocation {
  unsigned int startLine;       // Function definition start line
  unsigned int startColumn;     // Function definition start column
  unsigned int endLine;         // Function definition end line
  unsigned int endColumn;       // Function definition end column
  std::string functionName;     // Function name
  std::string className;        // Class name if it's a method
  std::string argNames;         // Comma-separated argument names
  std::string argTypes;         // Comma-separated argument types
  int numArgs;                  // Number of arguments
  std::vector<ArgumentInfo> detailedArgs; // Detailed argument info
  unsigned int bodyStartOffset; // Function body start position (byte offset
                                // from file beginning)
  unsigned int bodyBraceColumn; // Column of '{' (1-based, for indentation)
  unsigned int bodyIndentColumn; // Column of first statement (1-based, 0 if empty body)
};

/**
 * Structure to hold data passed to libclang visitor callbacks
 */
struct VisitorClientData {
  std::vector<FunctionLocation>
      *functionLocations; // Location information of all found functions
  CXFile inputFile;       // File to process (libclang file handle)
  std::vector<std::regex>
      *excludePatterns; // Regex patterns for functions to exclude
};

#endif // CORE_TYPES_H
