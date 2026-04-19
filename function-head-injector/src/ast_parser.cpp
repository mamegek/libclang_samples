#include "ast_parser.h"
#include "utils.h"
#include <iostream>

BodyStartInfo findFunctionBodyStart(CXCursor cursor) {
  // Initialize cursor representing function body (CompoundStatement)
  CXCursor bodyStmt = clang_getNullCursor();

  // Helper structure to find function body
  struct BodyFinder {
    CXCursor *result;
    bool found;
  };

  BodyFinder finder = {&bodyStmt, false};

  // Visit function's child nodes to find CompoundStatement (function body)
  clang_visitChildren(
      cursor,
      [](CXCursor c, CXCursor /*parent*/,
         CXClientData data) -> CXChildVisitResult {
        BodyFinder *finder = static_cast<BodyFinder *>(data);
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

    unsigned int offset, column;
    clang_getSpellingLocation(startLoc, nullptr /*&file*/, nullptr /*&line*/,
                              &column, &offset);

    return {offset + 1, column}; // Return position right after '{' and column of '{'
  }

  return {std::string::npos, 0}; // If not found
}

CXChildVisitResult functionVisitor(CXCursor cursor, CXCursor /*parent*/,
                                   CXClientData clientData) {
  VisitorClientData *visitorData = static_cast<VisitorClientData *>(clientData);
  CXCursorKind kind = clang_getCursorKind(cursor);

  // Process only function declarations or C++ methods
  if (kind == CXCursor_FunctionDecl || kind == CXCursor_CXXMethod) {
    CXString functionName = clang_getCursorSpelling(cursor);
    CXSourceRange range = clang_getCursorExtent(cursor);
    CXSourceLocation startLoc = clang_getRangeStart(range);
    CXSourceLocation endLoc = clang_getRangeEnd(range);

    CXFile file;
    unsigned int startLine, startColumn, endLine, endColumn;
    clang_getSpellingLocation(startLoc, &file, &startLine, &startColumn,
                              nullptr);
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

      // Get class name if it's a method
      if (kind == CXCursor_CXXMethod) {
        CXCursor parentCursor = clang_getCursorSemanticParent(cursor);
        CXString parentName = clang_getCursorSpelling(parentCursor);
        func.className = clang_getCString(parentName);
        clang_disposeString(parentName);
      } else {
        func.className = "";
      }

      // Get arguments
      int numArgs = clang_Cursor_getNumArguments(cursor);
      func.numArgs = numArgs;
      CXType funcType = clang_getCursorType(cursor);

      std::string argNamesStr = "";
      std::string argTypesStr = "";
      for (int i = 0; i < numArgs; ++i) {
        CXCursor argCursor = clang_Cursor_getArgument(cursor, i);
        CXString argName = clang_getCursorSpelling(argCursor);
        CXType argType = clang_getArgType(funcType, i);
        CXString typeName = clang_getTypeSpelling(argType);

        std::string argNameStr = clang_getCString(argName);
        std::string argTypeStr = clang_getCString(typeName);
        if (argNameStr.empty()) {
            argNameStr = "arg" + std::to_string(i); // Assign dummy if missing
        }
        func.detailedArgs.push_back({argNameStr, argTypeStr});

        argNamesStr += argNameStr;
        argTypesStr += argTypeStr;

        if (i < numArgs - 1) {
          argNamesStr += ", ";
          argTypesStr += ", ";
        }

        clang_disposeString(argName);
        clang_disposeString(typeName);
      }
      func.argNames = argNamesStr;
      func.argTypes = argTypesStr;

      // Check if function should be excluded
      if (visitorData->excludePatterns &&
          shouldExcludeFunction(func.functionName,
                                *(visitorData->excludePatterns))) {
        std::cerr << "Excluding function: " << func.functionName
                  << " (matched exclude pattern)" << std::endl;
        clang_disposeString(functionName);
        return CXChildVisit_Continue;
      }

      // Find the start position of function body
      BodyStartInfo bodyInfo = findFunctionBodyStart(cursor);

      if (bodyInfo.offset != std::string::npos) {
        func.bodyStartOffset = bodyInfo.offset;
        func.bodyBraceColumn = bodyInfo.column;
        visitorData->functionLocations->push_back(
            func); // Add found function to list

        // Debug output: Display information about found function
        std::cerr << "Found function: " << func.functionName << " at line "
                  << startLine << ":" << startColumn << " to line " << endLine
                  << ":" << endColumn << std::endl;
      }
    }

    clang_disposeString(functionName);
  }

  return CXChildVisit_Recurse; // Visit child nodes recursively
}

CXTranslationUnit parseSourceFile(CXIndex index, const std::string &inputFile) {
  // Compiler options (include paths, etc.)
  const char *args[] = {"-I/usr/include", "-I/usr/local/include"};

  int argCount = sizeof(args) / sizeof(args[0]);

  // Parse source file and generate AST
  CXTranslationUnit translationUnit = clang_parseTranslationUnit(
      index, inputFile.c_str(), args, argCount, // Compiler options
      nullptr, 0,                               // No unsaved files
      CXTranslationUnit_None                    // No special flags
  );

  if (translationUnit == nullptr) {
    std::cerr << "Error: Unable to parse translation unit: " << inputFile
              << std::endl;
  }

  return translationUnit;
}
