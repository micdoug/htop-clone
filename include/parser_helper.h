#pragma once

#include <string>
#include <unordered_map>

namespace parser_helper {
// Extract pairs of key-value data from a given file.
//
// Each line is considered a pair and the data is extracted considering the
// first ocurrence of the separator in the line. Example:
//   Input: PRETTY_NAME="Ubuntu 18.04.2 LTS"
//   Output: key(PRETTY_NAME) value("Ubuntu 18.04.2 LTS")
//
// Parameters:
//  - file_path: The path to the file we will read the pair values from.
//  - separator: The string that is used to delimit key and values in the file.
std::unordered_map<std::string, std::string> ExtractKeyValuePairsFromFile(
    const std::string& file_path, const std::string& separator);

// Remove string delimiters from the provided string.
//
// Parameters:
//  - value: The string that will be edited.
void RemoveDelimiters(std::string& value);

}  // namespace parser_helper
