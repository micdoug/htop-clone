#include "format.h"

#include <string>

using std::string;

string Format::ElapsedTime(long seconds) {
  return std::format("{:02d}:{:02d}:{:02d}", seconds / 3600,
                     (seconds % 3600) / 60, seconds % 60);
}
