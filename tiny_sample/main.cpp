#include <clang-c/Index.h>
#include <iostream>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source-file>" << std::endl;
        return 1;
    }

    // Create index
    CXIndex index = clang_createIndex(0, 0);
    
    // Parse translation unit
    CXTranslationUnit unit = clang_parseTranslationUnit(
        index,
        argv[1], nullptr, 0,
        nullptr, 0,
        CXTranslationUnit_None
    );
    
    if (unit == nullptr) {
        std::cerr << "Unable to parse translation unit. Quitting." << std::endl;
        clang_disposeIndex(index);
        return 1;
    }

    // Get cursor
    CXCursor cursor = clang_getTranslationUnitCursor(unit);
    
    // Visit children
    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor parent, CXClientData client_data) {
            CXString spelling = clang_getCursorSpelling(c);
            CXString kind = clang_getCursorKindSpelling(clang_getCursorKind(c));
            
            std::cout << "Found: " << clang_getCString(spelling) 
                      << " (Kind: " << clang_getCString(kind) << ")" << std::endl;
            
            clang_disposeString(spelling);
            clang_disposeString(kind);
            
            return CXChildVisit_Recurse;
        },
        nullptr
    );
    
    // Clean up
    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);
    
    return 0;
}