#pragma once

#include <optional>
#include <string>
#include <unordered_map>

// This class resolves system UIDs to the local username.
//
// It keeps an internal cache, so that it can resolve already known UIDs without
// fetching info from the OS. When there is a request for an unknown user, it
// updates its internal database to get new users.
class UidResolver {
 public:
  explicit UidResolver();
  virtual ~UidResolver() = default;

  // Get the username associated with the given uid.
  //
  // Parameters:
  //  - uid: The id of the target user.
  std::optional<std::string> FetchUserName(const int uid);

 private:
  // Stores a cache of uid -> names map.
  std::unordered_map<int, std::string> uid_to_names_{};
};
