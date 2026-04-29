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

    unsigned int offset, column, line;
    clang_getSpellingLocation(startLoc, nullptr /*&file*/, &line,
                              &column, &offset);

    // Get column of first statement inside body (for auto-indent)
    unsigned int firstStmtColumn = 0;
    clang_visitChildren(
        bodyStmt,
        [](CXCursor c, CXCursor, CXClientData data) {
          unsigned int col;
          clang_getSpellingLocation(clang_getCursorLocation(c), nullptr,
                                    nullptr, &col, nullptr);
          *static_cast<unsigned int *>(data) = col;
          return CXChildVisit_Break;
        },
        &firstStmtColumn);

    return {offset + 1, line, column, firstStmtColumn};
  }

  return {std::string::npos, 0, 0, 0}; // If not found
}

static std::string getFullFunctionName(CXCursor cursor, const std::string& functionName) {
  CXCursorKind kind = clang_getCursorKind(cursor);
  if (kind == CXCursor_CXXMethod) {
    CXCursor parentCursor = clang_getCursorSemanticParent(cursor);
    CXString parentName = clang_getCursorSpelling(parentCursor);
    std::string className = clang_getCString(parentName);
    std::string fullName = className + "::" + functionName;
    clang_disposeString(parentName);
    return fullName;
  }
  return functionName;
}

static std::vector<ArgumentInfo> extractArgumentInfos(CXCursor cursor) {
  std::vector<ArgumentInfo> args;
  int numArgs = clang_Cursor_getNumArguments(cursor);
  CXType funcType = clang_getCursorType(cursor);

  for (int i = 0; i < numArgs; ++i) {
    CXCursor argCursor = clang_Cursor_getArgument(cursor, i);
    CXString argName = clang_getCursorSpelling(argCursor);
    CXType argType = clang_getArgType(funcType, i);
    CXString typeName = clang_getTypeSpelling(argType);

    std::string argNameStr = clang_getCString(argName);
    std::string argTypeStr = clang_getCString(typeName);
    if (argNameStr.empty()) {
      argNameStr = "arg" + std::to_string(i);
    }
    args.push_back({argNameStr, argTypeStr});

    clang_disposeString(argName);
    clang_disposeString(typeName);
  }
  return args;
}

CXChildVisitResult functionVisitor(CXCursor cursor, CXCursor /*parent*/,
                                   CXClientData clientData) {
  VisitorClientData *visitorData = static_cast<VisitorClientData *>(clientData);
  CXCursorKind kind = clang_getCursorKind(cursor);

  if (kind != CXCursor_FunctionDecl && kind != CXCursor_CXXMethod) {
    return CXChildVisit_Recurse;
  }

  CXString cxFuncName = clang_getCursorSpelling(cursor);
  std::string functionName = clang_getCString(cxFuncName);
  
  CXSourceRange range = clang_getCursorExtent(cursor);
  CXSourceLocation startLoc = clang_getRangeStart(range);
  CXSourceLocation endLoc = clang_getRangeEnd(range);

  CXFile file;
  unsigned int startLine, startColumn, endLine, endColumn;
  clang_getSpellingLocation(startLoc, &file, &startLine, &startColumn, nullptr);
  clang_getSpellingLocation(endLoc, &file, &endLine, &endColumn, nullptr);

  if (!file || !clang_File_isEqual(file, visitorData->inputFile)) {
    clang_disposeString(cxFuncName);
    return CXChildVisit_Continue;
  }

  std::string fullName = getFullFunctionName(cursor, functionName);

  if (visitorData->excludePatterns &&
      shouldExcludeFunction(fullName, *(visitorData->excludePatterns))) {
    std::cerr << "Excluding function: " << fullName << " (matched exclude pattern)" << std::endl;
    clang_disposeString(cxFuncName);
    return CXChildVisit_Continue;
  }

  if (clang_isCursorDefinition(cursor)) {
    BodyStartInfo bodyInfo = findFunctionBodyStart(cursor);
    if (bodyInfo.offset != std::string::npos) {
      FunctionLocation func;
      func.startBodyLine = bodyInfo.line;
      func.startBodyColumn = bodyInfo.column;
      func.endLine = endLine;
      func.endColumn = endColumn;
      func.functionName = functionName;
      
      if (kind == CXCursor_CXXMethod) {
        size_t pos = fullName.find("::");
        func.className = fullName.substr(0, pos);
      }

      func.detailedArgs = extractArgumentInfos(cursor);
      func.numArgs = func.detailedArgs.size();
      
      // Build comma-separated strings for backward compatibility or simple use
      for (size_t i = 0; i < func.detailedArgs.size(); ++i) {
        func.argNames += func.detailedArgs[i].name;
        func.argTypes += func.detailedArgs[i].type;
        if (i < func.detailedArgs.size() - 1) {
          func.argNames += ", ";
          func.argTypes += ", ";
        }
      }

      func.bodyStartOffset = bodyInfo.offset;
      func.bodyBraceColumn = bodyInfo.column;
      func.bodyIndentColumn = bodyInfo.firstStmtColumn;
      visitorData->functionLocations->push_back(func);

      std::cerr << "Found function: " << fullName << " at body line "
                << func.startBodyLine << ":" << func.startBodyColumn << " to line " << endLine
                << ":" << endColumn << std::endl;
    }
  }

  clang_disposeString(cxFuncName);
  return CXChildVisit_Recurse;
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
