#ifndef AST_PARSER_H
#define AST_PARSER_H

#include "types.h"
#include <clang-c/Index.h>

size_t findFunctionBodyStart(CXCursor cursor);

CXChildVisitResult functionVisitor(CXCursor cursor, CXCursor parent,
                                   CXClientData clientData);

CXTranslationUnit parseSourceFile(CXIndex index, const std::string &inputFile);

#endif // AST_PARSER_H
