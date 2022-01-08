#ifndef FORMAT_H
#define FORMAT_H

#include <fmt/format.h>

#include <string>

// Create aliases for the fmt library to be compatible with the format spec in
// c++20. This approached was learned in the book "Professional C++ 5th edition"
namespace std {
using fmt::format;
using fmt::format_error;
using fmt::format_parse_context;
using fmt::formatter;
}  // namespace std

namespace Format {
// Format a elapsed time in seconds in the format HH:MM:SS.
std::string ElapsedTime(long times);
};  // namespace Format

#endif
