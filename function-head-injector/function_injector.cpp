#include <clang-c/Index.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

struct CommandLineArgs {
    std::string inputFile;
    std::string injectionFile;
    std::string outputFile;
};

struct FunctionLocation {
    unsigned int startLine;
    unsigned int startColumn;
    unsigned int endLine;
    unsigned int endColumn;
    std::string functionName;
    unsigned int bodyStartOffset;
};

std::vector<FunctionLocation> functionLocations;
std::string sourceContent;
std::string injectionCode;
CXFile inputFile = nullptr;

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool writeFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << " for writing" << std::endl;
        return false;
    }
    file << content;
    return true;
}

unsigned int getOffsetFromPosition(const std::string& content, unsigned int line, unsigned int column) {
    unsigned int currentLine = 1;
    unsigned int offset = 0;
    
    for (size_t i = 0; i < content.length(); ++i) {
        if (currentLine == line) {
            return offset + column - 1;
        }
        if (content[i] == '\n') {
            currentLine++;
        }
        offset++;
    }
    return offset;
}

unsigned int findFunctionBodyStart(CXCursor cursor, const std::string& content) {
    // Get the function body cursor
    CXCursor bodyStmt = clang_getNullCursor();
    
    // Visit children to find the compound statement (function body)
    struct BodyFinder {
        CXCursor* result;
        bool found;
    };
    
    BodyFinder finder = { &bodyStmt, false };
    
    clang_visitChildren(cursor, 
        [](CXCursor c, CXCursor parent, CXClientData data) -> CXChildVisitResult {
            BodyFinder* finder = static_cast<BodyFinder*>(data);
            if (!finder->found && clang_getCursorKind(c) == CXCursor_CompoundStmt) {
                *(finder->result) = c;
                finder->found = true;
                return CXChildVisit_Break;
            }
            return CXChildVisit_Continue;
        }, 
        &finder);
    
    if (finder.found && !clang_Cursor_isNull(bodyStmt)) {
        // Get the location of the compound statement
        CXSourceRange range = clang_getCursorExtent(bodyStmt);
        CXSourceLocation startLoc = clang_getRangeStart(range);
        
        CXFile file;
        unsigned int line, column;
        clang_getSpellingLocation(startLoc, &file, &line, &column, nullptr);
        
        // Find the opening brace of the compound statement
        unsigned int offset = getOffsetFromPosition(content, line, column);
        
        // The cursor points to the compound statement, find the '{'
        for (size_t i = offset; i < content.length(); ++i) {
            if (content[i] == '{') {
                return i + 1; // Return position after '{'
            }
        }
    }
    
    return std::string::npos;
}

CXChildVisitResult functionVisitor(CXCursor cursor, CXCursor parent, CXClientData clientData) {
    CXCursorKind kind = clang_getCursorKind(cursor);
    
    if (kind == CXCursor_FunctionDecl || kind == CXCursor_CXXMethod) {
        CXString functionName = clang_getCursorSpelling(cursor);
        CXSourceRange range = clang_getCursorExtent(cursor);
        CXSourceLocation startLoc = clang_getRangeStart(range);
        CXSourceLocation endLoc = clang_getRangeEnd(range);

        CXCursor definition = clang_getCursorDefinition(cursor);

        CXFile file;
        unsigned int startLine, startColumn, endLine, endColumn;
        clang_getSpellingLocation(startLoc, &file, &startLine, &startColumn, nullptr);
        clang_getSpellingLocation(endLoc, &file, &endLine, &endColumn, nullptr);
        
        // Check if the function is defined in the input file
        if (!file || !clang_File_isEqual(file, inputFile)) {
            clang_disposeString(functionName);
            return CXChildVisit_Continue;
        }
        
        if (clang_isCursorDefinition(cursor)) {
            FunctionLocation func;
            func.startLine = startLine;
            func.startColumn = startColumn;
            func.endLine = endLine;
            func.endColumn = endColumn;
            func.functionName = clang_getCString(functionName);
            
            unsigned int bodyStartOffset = findFunctionBodyStart(cursor, sourceContent);
            
            if (bodyStartOffset != std::string::npos) {
                func.bodyStartOffset = bodyStartOffset;
                functionLocations.push_back(func);
                
                std::cerr << "Found function: " << func.functionName 
                          << " at line " << startLine << ":" << startColumn
                          << " to line " << endLine << ":" << endColumn << std::endl;
            }
        }
        
        clang_disposeString(functionName);
    }
    
    return CXChildVisit_Recurse;
}

std::string injectCode(const std::string& content, const std::vector<FunctionLocation>& locations, const std::string& codeToInject) {
    std::string result = content;
    
    std::vector<FunctionLocation> sortedLocations = locations;
    std::sort(sortedLocations.begin(), sortedLocations.end(), 
              [](const FunctionLocation& a, const FunctionLocation& b) {
                  return a.bodyStartOffset > b.bodyStartOffset;  // Sort in descending order
              });
    
    for (const auto& func : sortedLocations) {
        std::string injection = "\n    " + codeToInject;
        if (!codeToInject.empty() && codeToInject.back() != '\n') {
            injection += "\n";
        }
        result.insert(func.bodyStartOffset, injection);
        std::cerr << "Injected code into function: " << func.functionName << std::endl;
    }
    
    return result;
}

bool parseArguments(int argc, char* argv[], CommandLineArgs& args) {
    if (argc < 3 || argc > 5) {
        std::cerr << "Usage: " << argv[0] << " <input_source_file> <injection_code_file> [-o <output_file>]" << std::endl;
        std::cerr << "  input_source_file: C/C++ source file to analyze" << std::endl;
        std::cerr << "  injection_code_file: File containing code to inject at function starts" << std::endl;
        std::cerr << "  -o output_file: Output file (if not specified, writes to stdout)" << std::endl;
        return false;
    }
    
    args.inputFile = argv[1];
    args.injectionFile = argv[2];
    args.outputFile = "/dev/stdout"; // Default to stdout
    
    // Parse optional -o flag
    for (int i = 3; i < argc; i++) {
        if (std::string(argv[i]) == "-o" && i + 1 < argc) {
            args.outputFile = argv[i + 1];
            i++; // Skip the next argument as it's the filename
        }
    }
    
    return true;
}


CXTranslationUnit parseSourceFile(CXIndex index, const std::string& inputFile) {
    const char* args[] = {
        "-I/usr/include",
        "-I/usr/local/include"
    };
    
    int argCount = sizeof(args) / sizeof(args[0]);
    
    CXTranslationUnit translationUnit = clang_parseTranslationUnit(
        index,
        inputFile.c_str(),
        args, argCount,
        nullptr, 0,
        CXTranslationUnit_None
    );
    
    if (translationUnit == nullptr) {
        std::cerr << "Error: Unable to parse translation unit: " << inputFile << std::endl;
    }
    
    return translationUnit;
}

bool processAndWriteOutput(const std::string& outputFile, 
                           const std::string& sourceContent,
                           const std::vector<FunctionLocation>& functionLocations,
                           const std::string& injectionCode) {
    if (functionLocations.empty()) {
        std::cerr << "No functions found in the source file." << std::endl;
        return true;
    }
    
    std::cerr << "\nTotal functions found: " << functionLocations.size() << std::endl;
    
    std::string modifiedContent = injectCode(sourceContent, functionLocations, injectionCode);
    
    if (!writeFile(outputFile, modifiedContent)) {
        std::cerr << "Error: Failed to write output file: " << outputFile << std::endl;
        return false;
    }
    
    if (outputFile != "/dev/stdout") {
        std::cerr << "\nModified source written to: " << outputFile << std::endl;
    }
    
    return true;
}

int main(int argc, char* argv[]) {
    CommandLineArgs args;
    if (!parseArguments(argc, argv, args)) {
        return 1;
    }
    
    sourceContent = readFile(args.inputFile);
    if (sourceContent.empty()) {
        std::cerr << "Error: Could not read input file: " << args.inputFile << std::endl;
        return 1;
    }
    
    injectionCode = readFile(args.injectionFile);
    if (injectionCode.empty()) {
        std::cerr << "Warning: Injection code file is empty or could not be read: " << args.injectionFile << std::endl;
    }
    
    while (!injectionCode.empty() && injectionCode.back() == '\n') {
        injectionCode.pop_back();
    }
    
    CXIndex index = clang_createIndex(0, 0);
    
    CXTranslationUnit translationUnit = parseSourceFile(index, args.inputFile);
    if (translationUnit == nullptr) {
        clang_disposeIndex(index);
        return 1;
    }
    
    ::inputFile = clang_getFile(translationUnit, args.inputFile.c_str());
    if (!::inputFile) {
        std::cerr << "Error: Unable to get CXFile for input file: " << args.inputFile << std::endl;
        clang_disposeTranslationUnit(translationUnit);
        clang_disposeIndex(index);
        return 1;
    }
    
    CXCursor rootCursor = clang_getTranslationUnitCursor(translationUnit);
    clang_visitChildren(rootCursor, functionVisitor, nullptr);
    
    bool success = processAndWriteOutput(args.outputFile, sourceContent, functionLocations, injectionCode);
    
    clang_disposeTranslationUnit(translationUnit);
    clang_disposeIndex(index);
    
    return success ? 0 : 1;
}