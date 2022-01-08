#include "uid_resolver.h"

#include <fstream>

#include "errors.h"

namespace {

// In Linux based systems user informatio can be found in the file
// "/etc/passwd". In this function we parse this file to extract the mapping
// between user ids and the name.
std::unordered_map<int, std::string> BuildUidsToUserNameMap() {
  const char* kPasswdFilePath{"/etc/passwd"};
  std::ifstream passwd_file{kPasswdFilePath};
  if (!passwd_file) {
    throw OpenFileError{kPasswdFilePath};
  }

  std::unordered_map<int, std::string> uid_to_names{};

  // the line is composed of values delimited by the ':' character
  // we are interested in the first and third value
  std::string line{};
  while (std::getline(passwd_file, line)) {
    std::size_t delimiter_pos{line.find(':')};
    std::string username{line.substr(0, delimiter_pos)};
    delimiter_pos = line.find(':', delimiter_pos + 1);
    int uid = std::stoi(
        line.substr(delimiter_pos + 1, line.find(':', delimiter_pos + 1)));
    uid_to_names[uid] = username;
  }
  return uid_to_names;
}
}  // namespace

UidResolver::UidResolver() : uid_to_names_{BuildUidsToUserNameMap()} {}

std::optional<std::string> UidResolver::FetchUserName(const int uid) {
  // If the target uid is not in the cache, we rebuild the cache.
  if (uid_to_names_.find(uid) == uid_to_names_.end()) {
    uid_to_names_ = BuildUidsToUserNameMap();
  }

  // If even after rebuilding the cache the uid is not found, we consider that
  // it doesn't exists in the system anymore.
  if (uid_to_names_.find(uid) == uid_to_names_.end()) {
    return std::nullopt;
  }
  return uid_to_names_[uid];
}
