/* This file defines come common exceptions used by the program.
 */
#pragma once

#include <exception>
#include <string>
#include <string_view>

#include "format.h"

// Should be thrown when there is an error while opening a file.
class OpenFileError : public std::exception {
 public:
  explicit OpenFileError(const std::string_view file_path)
      : what_{std::format("Could not open file: {}", file_path)} {}
  const char* what() const noexcept override { return what_.c_str(); }

 private:
  std::string what_{};
};

// Should be thrown when there is an error reading some input that has a
// different format than expected.
class UnexpectedFormatError : public std::exception {
 public:
  explicit UnexpectedFormatError(std::string_view description)
      : what_{description} {}
  const char* what() const noexcept override { return what_.c_str(); }

 private:
  std::string what_;
};
