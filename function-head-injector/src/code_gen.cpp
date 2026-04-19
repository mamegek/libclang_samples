#include "code_gen.h"
#include <iostream>

std::string generateTemplateHook(std::string str,
                                 const FunctionLocation &func) {
  str = std::regex_replace(
      str,
      std::regex(R"(\{\{\s*function_name\s*\}\})", std::regex_constants::icase),
      func.functionName);
  str = std::regex_replace(
      str,
      std::regex(R"(\{\{\s*class_name\s*\}\})", std::regex_constants::icase),
      func.className);
  str = std::regex_replace(
      str,
      std::regex(R"(\{\{\s*arg_names\s*\}\})", std::regex_constants::icase),
      func.argNames);
  str = std::regex_replace(
      str,
      std::regex(R"(\{\{\s*arg_types\s*\}\})", std::regex_constants::icase),
      func.argTypes);
  str = std::regex_replace(
      str, std::regex(R"(\{\{\s*num_args\s*\}\})", std::regex_constants::icase),
      std::to_string(func.numArgs));
  return str;
}

std::string getFormatSpecifier(const std::string& typeName) {
    if (typeName == "int" || typeName == "short" || typeName == "char" || typeName == "bool") return "%d";
    if (typeName == "long") return "%ld";
    if (typeName == "long long") return "%lld";
    if (typeName == "unsigned int" || typeName == "unsigned short" || typeName == "unsigned char") return "%u";
    if (typeName == "unsigned long") return "%lu";
    if (typeName == "unsigned long long") return "%llu";
    if (typeName == "float" || typeName == "double") return "%f";
    if (typeName == "const char *" || typeName == "char *") return "%s";
    if (typeName.find("*") != std::string::npos) return "%p";
    return ""; // Empty means it's unsupported/complex type
}

std::string generatePrintfHook(const FunctionLocation& func) {
    std::string prefix = func.className.empty() ? func.functionName : func.className + "::" + func.functionName;
    std::string formatStr = "printf(\">> " + prefix + " called";
    
    std::vector<std::string> validArgNames;
    std::vector<std::string> validArgFormats;
    
    for (const auto& arg : func.detailedArgs) {
        std::string fmt = getFormatSpecifier(arg.type);
        if (!fmt.empty()) {
            validArgNames.push_back(arg.name);
            validArgFormats.push_back(arg.name + "=" + fmt);
        }
    }
    
    if (!validArgFormats.empty()) {
        formatStr += " (";
        for (size_t i = 0; i < validArgFormats.size(); ++i) {
            formatStr += validArgFormats[i];
            if (i < validArgFormats.size() - 1) formatStr += ", ";
        }
        formatStr += ")";
    }
    formatStr += "\\n\"";
    
    for (const auto& name : validArgNames) {
        formatStr += ", " + name;
    }
    formatStr += ");";
    return formatStr;
}

std::string generateUsdtHook(const FunctionLocation& func) {
    std::vector<std::string> validArgNames;
    
    for (const auto& arg : func.detailedArgs) {
        std::string fmt = getFormatSpecifier(arg.type);
        if (!fmt.empty()) {
            validArgNames.push_back(arg.name);
        }
    }
    
    int n = validArgNames.size();
    if (n > 12) n = 12; // SDT macros cap at 12
    
    std::string result = "DTRACE_PROBE" + (n > 0 ? std::to_string(n) : "") + "(app, " + func.functionName;
    for (int i = 0; i < n; ++i) {
        result += ", " + validArgNames[i];
    }
    result += ");";
    return result;
}

std::string generateHookCode(const FunctionLocation &func,
                             const std::string &templateCode,
                             InjectionMode mode) {
  switch (mode) {
  case InjectionMode::PRINTF:
    return generatePrintfHook(func);
  case InjectionMode::USDT:
    return generateUsdtHook(func);
  case InjectionMode::TEMPLATE:
  default:
    return generateTemplateHook(templateCode, func);
  }
}
