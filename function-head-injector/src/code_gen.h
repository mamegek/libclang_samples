#ifndef CODE_GEN_H
#define CODE_GEN_H

#include "types.h"

std::string generateTemplateHook(std::string str, const FunctionLocation &func);
std::string getFormatSpecifier(const std::string& typeName);
std::string generatePrintfHook(const FunctionLocation& func);
std::string generateUsdtHook(const FunctionLocation& func);
std::string generateHookCode(const FunctionLocation &func,
                             const std::string &templateCode,
                             InjectionMode mode);

#endif // CODE_GEN_H
