#ifndef CODE_GEN_H
#define CODE_GEN_H

#include "types.h"

std::string generateTemplateHook(std::string str, const FunctionLocation &func);
std::string getFormatSpecifier(const std::string& typeName);
std::string generatePrintfHook(const FunctionLocation& func);
std::string generateUsdtHook(const FunctionLocation& func);
std::string injectCode(const std::string &content,
                       const std::vector<FunctionLocation> &locations,
                       const std::string &codeToInject,
                       InjectionMode mode);

#endif // CODE_GEN_H
