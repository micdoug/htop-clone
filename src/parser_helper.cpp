#include "parser_helper.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>

#include "errors.h"
#include "format.h"

namespace parser_helper {
std::unordered_map<std::string, std::string> ExtractKeyValuePairsFromFile(
    const std::string_view file_path, const std::string_view separator) {
  std::ifstream input_file{std::string{file_path}};
  if (!input_file) {
    throw OpenFileError{file_path};
  }
  std::unordered_map<std::string, std::string> map{};
  std::string line{};
  while (std::getline(input_file, line)) {
    std::size_t separator_pos = line.find(separator);
    if (separator_pos == std::string::npos) {
      throw UnexpectedFormatError{std::format(
          "Could not find the separator '{}' in the line '{}' of file {}.",
          separator, line, file_path)};
    }
    std::string key{line.substr(0, separator_pos)};
    std::string value{line.substr(separator_pos + separator.size())};
    map[key] = value;
  }
  return map;
}

void RemoveDelimiters(std::string& value) {
  value.erase(std::remove(value.begin(), value.end(), '"'), value.end());
  value.erase(std::remove(value.begin(), value.end(), '\''), value.end());
}

}  // namespace parser_helper
