/*
 * This file is used to check for filesystem support by the compiler.
 * You can ignore the code.
 */

#include <filesystem>

namespace fs = std::filesystem;

int main() {
  fs::path path{"../"};

  return 0;
}
