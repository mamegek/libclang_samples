#ifndef UTILS_H
#define UTILS_H

#include "types.h"
#include <string>
#include <vector>
#include <regex>

std::string readFile(const std::string &filename);
std::vector<std::regex> readExcludePatterns(const std::string &filename);
bool shouldExcludeFunction(const std::string &functionName, const std::vector<std::regex> &patterns);
bool writeFile(const std::string &filename, const std::string &content);
bool parseArguments(int argc, char *argv[], CommandLineArgs &args);

#endif // UTILS_H
