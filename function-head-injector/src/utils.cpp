#include "utils.h"
#include <fstream>
#include <iostream>
#include <sstream>

std::string readFile(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error: Cannot open file " << filename << std::endl;
    return "";
  }
  std::stringstream buffer;
  buffer << file.rdbuf(); // Read entire file into buffer
  return buffer.str();
}

std::vector<std::regex> readExcludePatterns(const std::string &filename) {
  std::vector<std::regex> patterns;
  if (filename.empty()) {
    return patterns; // empty vector
  }

  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Warning: Cannot open exclude pattern file " << filename
              << std::endl;
    return patterns;
  } else {
    std::cerr << "Loaded exclude patterns from " << filename << std::endl;
  }

  std::string line;
  while (std::getline(file, line)) {
    // Skip empty lines and comment lines (starting with #)
    if (line.empty() || line[0] == '#') {
      continue;
    }

    std::cerr << "\tAdding exclude regex pattern: " << line << std::endl;
    patterns.push_back(std::regex(line));
  }

  return patterns;
}

bool shouldExcludeFunction(const std::string &functionName,
                           const std::vector<std::regex> &patterns) {
  for (const auto &pattern : patterns) {
    if (std::regex_match(functionName, pattern)) {
      return true;
    }
  }
  return false;
}

bool writeFile(const std::string &filename, const std::string &content) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error: Cannot open file " << filename << " for writing"
              << std::endl;
    return false;
  }
  file << content;
  return true;
}

bool parseArguments(int argc, char *argv[], CommandLineArgs &args) {
  auto printUsage = [&]() {
    std::cerr << "Usage: " << argv[0]
              << " <input_source_file> [-m <mode>] [injection_code_file] [-o "
                 "<output_file>]"
                 " [-e <exclude_pattern_file>] [-H <header_injection_file>]"
                 " [-i <indent_width>] [-l <min_lines>]"
              << std::endl;
    std::cerr << "  input_source_file   : C/C++ source file to analyze"
              << std::endl;
    std::cerr << "  injection_code_file : File containing code to "
                 "inject at function starts (required for template mode, "
                 "ignored otherwise)"
              << std::endl;
    std::cerr << "  -m mode                 : Generation mode: template "
                 "(default), printf, usdt"
              << std::endl;
    std::cerr << "  -o output_file          : Output file (if not specified, "
                 "writes to stdout)"
              << std::endl;
    std::cerr << "  -e exclude_pattern_file : File containing regex patterns "
                 "for functions to exclude (one per line)"
              << std::endl;
    std::cerr << "  -H header_content_file  : File containing code to inject "
                 "at the top of the file"
              << std::endl;
    std::cerr << "  -i indent_width         : Indentation width for injected "
                 "code (default: 4, 0 to disable, 'auto' to detect)"
              << std::endl;
    std::cerr << "  --min-lines minimum_line: Skip functions shorter than "
                 "this many lines (default: 0 = no skip)"
              << std::endl;
    std::cerr << "  --inplace               : Overwrite the input file"
              << std::endl;
  };

  if (argc < 2) {
    printUsage();
    return false;
  }

  args.outputFile = "/dev/stdout";              // Default is stdout
  args.excludePatternFile = "";                 // Default is no exclude file
  args.headerContentFile = "";                  // Default is no header file
  args.injectionMode = InjectionMode::TEMPLATE; // Default mode
  args.indentWidth = 4;                         // Default indent width
  args.minLines = 0;                            // Default: no minimum
  args.inplace = false;                         // Default: don't overwrite

  std::vector<std::string> positionalArgs; // Store positional arguments

  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if ((arg == "-o") || (arg == "--output")) {
      // Output file option
      if (i + 1 >= argc) {
        std::cerr << "Error: -o requires an argument" << std::endl;
        return false;
      }
      args.outputFile = argv[++i];
    } else if ((arg == "-e") || (arg == "--exclude")) {
      // Exclude pattern file option
      if (i + 1 >= argc) {
        std::cerr << "Error: -e requires an argument" << std::endl;
        return false;
      }
      args.excludePatternFile = argv[++i];
    } else if ((arg == "-H") || (arg == "--header")) {
      // Header injection file option
      if (i + 1 >= argc) {
        std::cerr << "Error: -H requires an argument" << std::endl;
        return false;
      }
      args.headerContentFile = argv[++i];
    } else if ((arg == "-m") || (arg == "--mode")) {
      // Mode option
      if (i + 1 >= argc) {
        std::cerr << "Error: -m requires an argument (template | printf | usdt)"
                  << std::endl;
        return false;
      }
      std::string modeStr = argv[++i];
      if (modeStr == "template") {
        args.injectionMode = InjectionMode::TEMPLATE;
      } else if (modeStr == "printf") {
        args.injectionMode = InjectionMode::PRINTF;
      } else if (modeStr == "usdt") {
        args.injectionMode = InjectionMode::USDT;
      } else {
        std::cerr << "Error: Unknown mode: " << modeStr
                  << ". Allowed modes: template, printf, usdt" << std::endl;
        return false;
      }
    } else if ((arg == "-i") || (arg == "--indent")) {
      // Indent width option
      if (i + 1 >= argc) {
        std::cerr << "Error: -i requires an argument" << std::endl;
        return false;
      }
      std::string val = argv[++i];
      args.indentWidth = (val == "auto") ? -1 : std::stoi(val);
    } else if ( arg == "--min-lines") {
      // Minimum function lines option
      if (i + 1 >= argc) {
        std::cerr << "Error: --min-line requires an argument" << std::endl;
        return false;
      }
      args.minLines = std::stoi(argv[++i]);
    } else if (arg == "--inplace") {
      args.inplace = true;
    } else if (arg[0] == '-') {
      // Unknown option
      std::cerr << "Error: Unknown option: " << arg << std::endl;
      return false;
    } else {
      // Record as positional argument
      positionalArgs.push_back(arg);
    }
  }

  // Check number of positional arguments based on mode
  if (positionalArgs.size() < 1) {
    std::cerr
        << "Error: Expected at least 1 positional argument (input_source_file)"
        << std::endl;
    printUsage();
    return false;
  }
  if (args.injectionMode == InjectionMode::TEMPLATE &&
      positionalArgs.size() < 2) {
    std::cerr << "Error: Template mode requires a 2nd positional argument "
                 "(injection_code_file)"
              << std::endl;
    return false;
  }

  args.inputSourceFile = positionalArgs[0]; // First argument: input source file
  if (args.inplace) {
    args.outputFile = args.inputSourceFile;
  }
  if (positionalArgs.size() > 1) {
    args.hookContentFile = positionalArgs[1]; // Second argument: injection code
                                              // file (optional for printf/usdt)
  } else {
    args.hookContentFile = "";
  }

  return true;
}
