/**
 * Function Head Injector - A tool using libclang to inject code at the beginning of C/C++ functions
 * 
 * This tool parses the specified source file, finds all function definitions,
 * and inserts specified code at the beginning of each function body (right after the opening brace '{').
 * Main use cases: Automatic insertion of debug code, trace processing, logging, etc.
 */

#include <clang-c/Index.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <regex>

/**
 * Structure to manage command line arguments
 */
struct CommandLineArgs {
    std::string inputFile;      // Input C/C++ source file
    std::string injectionFile;   // File containing code to inject
    std::string outputFile;      // Output file (default is stdout)
    std::string excludeFile;     // File containing regex patterns for functions to exclude
};

/**
 * Structure to hold function location information and name
 */
struct FunctionLocation {
    unsigned int startLine;      // Function definition start line
    unsigned int startColumn;    // Function definition start column
    unsigned int endLine;        // Function definition end line
    unsigned int endColumn;      // Function definition end column
    std::string functionName;    // Function name
    unsigned int bodyStartOffset; // Function body start position (byte offset from file beginning)
};

/**
 * Structure to hold data passed to libclang visitor callbacks
 */
struct VisitorClientData {
    std::vector<FunctionLocation>* functionLocations;  // Location information of all found functions
    CXFile inputFile;                                  // File to process (libclang file handle)
    std::vector<std::regex>* excludePatterns;          // Regex patterns for functions to exclude
};

/**
 * Read file contents and return as string
 * @param filename Path to the file to read
 * @return File contents (empty string on read failure)
 */
std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();  // Read entire file into buffer
    return buffer.str();
}

/**
 * Read regex patterns from file (one pattern per line)
 * @param filename Path to the file containing regex patterns
 * @return Vector of compiled regex patterns
 */
std::vector<std::regex> readExcludePatterns(const std::string& filename) {
    std::vector<std::regex> patterns;
    if (filename.empty()) {
        return patterns; // empty vector
    }
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Warning: Cannot open exclude pattern file " << filename << std::endl;
        return patterns;
    }else{
        std::cerr << "Loaded exclude patterns from " << filename << std::endl;
    }
    
    std::string line;
    int lineNum = 0;
    while (std::getline(file, line)) {
        lineNum++;
        // Skip empty lines and comment lines (starting with #)
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Since exceptions are disabled, we need to check regex validity differently
        // For now, we'll just add the pattern and let std::regex handle it
        // If the pattern is invalid, the program will terminate
        patterns.push_back(std::regex(line));
        std::cerr << "Added exclude pattern: " << line << std::endl;
    }
    
    return patterns;
}

/**
 * Check if function name matches any exclude pattern
 * @param functionName Function name to check
 * @param patterns Vector of regex patterns
 * @return true if function should be excluded, false otherwise
 */
bool shouldExcludeFunction(const std::string& functionName, const std::vector<std::regex>& patterns) {
    for (const auto& pattern : patterns) {
        if (std::regex_match(functionName, pattern)) {
            return true;
        }
    }
    return false;
}

/**
 * Write string to file
 * @param filename Path to the output file
 * @param content Content to write
 * @return true on success, false on failure
 */
bool writeFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << " for writing" << std::endl;
        return false;
    }
    file << content;
    return true;
}

/**
 * Find the start position of function body (right after the opening brace '{')
 * @param cursor Function cursor (libclang AST element)
 * @return Offset of function body start position (std::string::npos if not found)
 */
size_t findFunctionBodyStart(CXCursor cursor) {
    // Initialize cursor representing function body (CompoundStatement)
    CXCursor bodyStmt = clang_getNullCursor();
    
    // Helper structure to find function body
    struct BodyFinder {
        CXCursor* result;
        bool found;
    };
    
    BodyFinder finder = { &bodyStmt, false };
    
    // Visit function's child nodes to find CompoundStatement (function body)
    clang_visitChildren(cursor, 
        [](CXCursor c, CXCursor /*parent*/, CXClientData data) -> CXChildVisitResult {
            BodyFinder* finder = static_cast<BodyFinder*>(data);
            // Record CompoundStatement when found and stop searching
            if (!finder->found && clang_getCursorKind(c) == CXCursor_CompoundStmt) {
                *(finder->result) = c;
                finder->found = true;
                return CXChildVisit_Break;
            }
            return CXChildVisit_Continue;
        }, 
        &finder);
    
    if (finder.found && !clang_Cursor_isNull(bodyStmt)) {
        // Get CompoundStatement location information
        CXSourceRange range = clang_getCursorExtent(bodyStmt);
        CXSourceLocation startLoc = clang_getRangeStart(range);
        
        unsigned int offset;
        clang_getSpellingLocation(startLoc
                    , nullptr /*&file*/, nullptr /*&line*/, nullptr /*&column*/, &offset);
        
        return offset + 1; // Return position right after '{'
    }
    
    return std::string::npos;  // If not found
}

/**
 * Callback function to visit libclang AST and find function definitions
 * @param cursor Currently visiting AST node
 * @param parent Parent node  (unused in this case)
 * @param clientData User data containing visitor context
 * @return Instruction whether to continue visiting child nodes
 */
CXChildVisitResult functionVisitor(CXCursor cursor, CXCursor /*parent*/, CXClientData clientData) {
    VisitorClientData* visitorData = static_cast<VisitorClientData*>(clientData);
    CXCursorKind kind = clang_getCursorKind(cursor);
    
    // Process only function declarations or C++ methods
    if (kind == CXCursor_FunctionDecl || kind == CXCursor_CXXMethod) {
        CXString functionName = clang_getCursorSpelling(cursor);
        CXSourceRange range = clang_getCursorExtent(cursor);
        CXSourceLocation startLoc = clang_getRangeStart(range);
        CXSourceLocation endLoc = clang_getRangeEnd(range);

        CXFile file;
        unsigned int startLine, startColumn, endLine, endColumn;
        clang_getSpellingLocation(startLoc, &file, &startLine, &startColumn, nullptr);
        clang_getSpellingLocation(endLoc, &file, &endLine, &endColumn, nullptr);
        
        // Process only functions defined in the target input file
        // (Exclude functions from other files like headers, e.g.  __bswap_16)
        if (!file || !clang_File_isEqual(file, visitorData->inputFile)) {
            clang_disposeString(functionName);
            return CXChildVisit_Continue;
        }
        
        // Process only function definitions (implementations, not declarations)
        if (clang_isCursorDefinition(cursor)) {
            FunctionLocation func;
            func.startLine = startLine;
            func.startColumn = startColumn;
            func.endLine = endLine;
            func.endColumn = endColumn;
            func.functionName = clang_getCString(functionName);
            
            // Check if function should be excluded
            if (visitorData->excludePatterns && 
                shouldExcludeFunction(func.functionName, *(visitorData->excludePatterns))) {
                std::cerr << "Excluding function: " << func.functionName 
                          << " (matched exclude pattern)" << std::endl;
                clang_disposeString(functionName);
                return CXChildVisit_Continue;
            }
            
            // Find the start position of function body
            size_t bodyStartOffset = findFunctionBodyStart(cursor);
            
            if (bodyStartOffset != std::string::npos) {
                func.bodyStartOffset = bodyStartOffset;
                visitorData->functionLocations->push_back(func);  // Add found function to list
                
                // Debug output: Display information about found function
                std::cerr << "Found function: " << func.functionName 
                          << " at line " << startLine << ":" << startColumn
                          << " to line " << endLine << ":" << endColumn << std::endl;
            }
        }
        
        clang_disposeString(functionName);
    }
    
    return CXChildVisit_Recurse;  // Visit child nodes recursively
}

/**
 * Insert specified code at the beginning of all found functions
 * @param content Original source code
 * @param locations List of function location information
 * @param codeToInject Code to inject
 * @return Source code after code injection
 */
std::string injectCode(const std::string& content, const std::vector<FunctionLocation>& locations, const std::string& codeToInject) {
    std::string result = content;
    
    // Sort location information in descending order (from back)
    // Reason: Inserting from front would shift subsequent positions
    std::vector<FunctionLocation> sortedLocations = locations;
    std::sort(sortedLocations.begin(), sortedLocations.end(), 
              [](const FunctionLocation& a, const FunctionLocation& b) {
                  return a.bodyStartOffset > b.bodyStartOffset;  // Descending sort
              });
    
    // Insert code into each function
    for (const auto& func : sortedLocations) {
        // Format code to inject (add newline and indentation)
        std::string injection = "\n    " + codeToInject;
        if (!codeToInject.empty() && codeToInject.back() != '\n') {
            injection += "\n";
        }
        result.insert(func.bodyStartOffset, injection);
        std::cerr << "Injected code into function: " << func.functionName << std::endl;
    }
    
    return result;
}

/**
 * Parse command line arguments
 * @param argc Number of arguments
 * @param argv Array of arguments
 * @param args Structure to store parsing results
 * @return true on successful parsing, false on failure
 */
bool parseArguments(int argc, char* argv[], CommandLineArgs& args) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_source_file> <injection_code_file> [-o <output_file>] [-e <exclude_pattern_file>]" << std::endl;
        std::cerr << "  1st arg ... input_source_file   : C/C++ source file to analyze" << std::endl;
        std::cerr << "  2nd arg ... injection_code_file : File containing code to inject at function starts" << std::endl;
        std::cerr << "  -o output_file: Output file (if not specified, writes to stdout)" << std::endl;
        std::cerr << "  -e exclude_pattern_file: File containing regex patterns for functions to exclude (one per line)" << std::endl;
        return false;
    }
    
    args.outputFile = "/dev/stdout"; // Default is stdout
    args.excludeFile = ""; // Default is no exclude file
    
    std::vector<std::string> positionalArgs;  // Store positional arguments
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if ( (arg == "-o") || (arg == "--output") ) {
            // Output file option
            if (i + 1 >= argc) {
                std::cerr << "Error: -o requires an argument" << std::endl;
                return false;
            }
            args.outputFile = argv[++i];
        } else if ( (arg == "-e") || (arg == "--exclude") ) {
            // Exclude pattern file option
            if (i + 1 >= argc) {
                std::cerr << "Error: -e requires an argument" << std::endl;
                return false;
            }
            args.excludeFile = argv[++i];
        } else if (arg[0] == '-') {
            // Unknown option
            std::cerr << "Error: Unknown option: " << arg << std::endl;
            return false;
        } else {
            // Record as positional argument
            positionalArgs.push_back(arg);
        }
    }
    
    // Check number of positional arguments (need 2: input file and injection code file)
    if (positionalArgs.size() != 2) {
        std::cerr << "Error: Expected exactly 2 positional arguments (input_source_file and injection_code_file)" << std::endl;
        std::cerr << "Got " << positionalArgs.size() << " argument(s)" << std::endl;
        return false;
    }
    
    args.inputFile = positionalArgs[0];      // First argument: input source file
    args.injectionFile = positionalArgs[1];  // Second argument: injection code file
    
    return true;
}


/**
 * Parse source file using libclang and build AST
 * @param index libclang index
 * @param inputFile Path to source file to parse
 * @return Translation unit (AST), nullptr on failure
 */
CXTranslationUnit parseSourceFile(CXIndex index, const std::string& inputFile) {
    // Compiler options (include paths, etc.)
    const char* args[] = {
        "-I/usr/include",
        "-I/usr/local/include"
    };
    
    int argCount = sizeof(args) / sizeof(args[0]);
    
    // Parse source file and generate AST
    CXTranslationUnit translationUnit = clang_parseTranslationUnit(
        index,
        inputFile.c_str(),
        args, argCount,      // Compiler options
        nullptr, 0,          // No unsaved files
        CXTranslationUnit_None  // No special flags
    );
    
    if (translationUnit == nullptr) {
        std::cerr << "Error: Unable to parse translation unit: " << inputFile << std::endl;
    }
    
    return translationUnit;
}

/**
 * Write processing result to output file
 * @param outputFile Output file path
 * @param sourceContent Original source code
 * @param functionLocations Location information of found functions
 * @param injectionCode Code to inject
 * @return true on successful processing, false on failure
 */
bool processAndWriteOutput(const std::string& outputFile, 
                           const std::string& sourceContent,
                           const std::vector<FunctionLocation>& functionLocations,
                           const std::string& injectionCode) {
    if (functionLocations.empty()) {
        std::cerr << "No functions found in the source file." << std::endl;
        return true;  // Not an error even if no functions found
    }
    
    std::cerr << "\nTotal functions found: " << functionLocations.size() << std::endl;
    
    // Insert code
    std::string modifiedContent = injectCode(sourceContent, functionLocations, injectionCode);
    
    // Write to file
    if (!writeFile(outputFile, modifiedContent)) {
        std::cerr << "Error: Failed to write output file: " << outputFile << std::endl;
        return false;
    }
    
    // Display output destination if not stdout
    if (outputFile != "/dev/stdout") {
        std::cerr << "\nModified source written to: " << outputFile << std::endl;
    }
    
    return true;
}

/**
 * Main function: Program entry point
 */
int main(int argc, char* argv[]) {
    // Parse command line arguments
    CommandLineArgs args;
    if (!parseArguments(argc, argv, args)) {
        return 1;
    }
    
    // Read input source file
    std::string sourceContent = readFile(args.inputFile);
    if (sourceContent.empty()) {
        std::cerr << "Error: Could not read input file: " << args.inputFile << std::endl;
        return 1;
    }
    
    // Read code to inject
    std::string injectionCode;  // Code to inject
    injectionCode = readFile(args.injectionFile);
    if (injectionCode.empty()) {
        std::cerr << "Warning: Injection code file is empty or could not be read: " << args.injectionFile << std::endl;
    }
    
    // Remove trailing newlines from injection code (for cleaner formatting)
    while (!injectionCode.empty() && injectionCode.back() == '\n') {
        injectionCode.pop_back();
    }
    
    // Create libclang index
    // First argument: Exclude declarations from PCH (PreCompiledHeader)
    // Second argument: Don't display diagnostic (0)
    CXIndex index = clang_createIndex(1, 0);
    
    // Parse source file and build AST
    CXTranslationUnit translationUnit = parseSourceFile(index, args.inputFile);
    if (translationUnit == nullptr) {
        clang_disposeIndex(index);
        return 1;
    }
    
    // Get handle for input file (to identify file during function visiting)
    CXFile inputFile = clang_getFile(translationUnit, args.inputFile.c_str());
    if (!inputFile) {
        std::cerr << "Error: Unable to get CXFile for input file: " << args.inputFile << std::endl;
        clang_disposeTranslationUnit(translationUnit);
        clang_disposeIndex(index);
        return 1;
    }
    
    // Read exclude patterns from file (empty if no file specified)
    std::vector<std::regex> excludePatterns = readExcludePatterns(args.excludeFile);
    
    // Prepare client data for visitor
    std::vector<FunctionLocation> functionLocations;
    VisitorClientData visitorData = {
        &functionLocations,
        inputFile,
        &excludePatterns
    };
    
    // Visit all nodes from AST root to find functions
    CXCursor rootCursor = clang_getTranslationUnitCursor(translationUnit);
    clang_visitChildren(rootCursor, functionVisitor, &visitorData);
    
    // Process results and output to file
    bool success = processAndWriteOutput(args.outputFile, sourceContent, functionLocations, injectionCode);
    
    // Clean up resources
    clang_disposeTranslationUnit(translationUnit);
    clang_disposeIndex(index);
    
    return success ? 0 : 1;
}