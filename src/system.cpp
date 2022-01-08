#include "system.h"

#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "errors.h"
#include "format.h"
#include "parser_helper.h"
#include "process.h"
#include "processor.h"

// We check for the support of filesystem module.
// In the version that runs in the udacity workspace, the experimental module is
// only available as an experimental feature.
#ifdef USE_EXPERIMENTAL_FILESYSTEM
#include <experimental/filesystem>
#else
#include <filesystem>
#endif

using std::set;
using std::size_t;
using std::string;
using std::vector;
using std::chrono::system_clock;

// We check for the support of filesystem module.
// In the version that runs in the udacity workspace, the experimental module is
// only available as an experimental feature.
#ifdef USE_EXPERIMENTAL_FILESYSTEM
namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif

namespace {
// Inspect the OS proc directory for process descriptors.
//
// On Linux like systems we can find directories under "/proc/" that contains
// files related to running processes. The directories are named with the
// process id. So if we scan the "/proc" directory for other directories that
// are named with a valid number, we can assume that it contains information
// about a running process.
std::unordered_set<int> FetchProcessIds() {
  const char* kProcDictoryPath{"/proc"};
  std::unordered_set<int> pids{};

  fs::path proc_path{kProcDictoryPath};
  if (!fs::exists(proc_path)) {
    throw std::logic_error(std::format(
        "Could not find the '{}' directory in this system.", kProcDictoryPath));
  }

  // scan all content under the proc diretory
  for (const auto& entry : fs::directory_iterator(proc_path)) {
    const std::string& filename{entry.path().filename().string()};
    if (!fs::is_directory(entry) ||
        std::any_of(
            filename.begin(), filename.end(),
            [](const char& character) { return !std::isdigit(character); })) {
      continue;
    }
    pids.emplace(std::stoi(filename));
  }
  return pids;
}

// Inspect the OS proc directory for the system boot time.
//
// On Linux like systems we can find the boot time in the file "/proc/stat" in
// the line with the property btime.
system_clock::time_point FetchBootTime() {
  const char* kStatFilePath{"/proc/stat"};
  std::time_t boot_time{};
  const auto map =
      parser_helper::ExtractKeyValuePairsFromFile(kStatFilePath, " ");
  std::istringstream str_parser{map.at("btime")};
  str_parser >> boot_time;
  return system_clock::from_time_t(boot_time);
}

// Inspect the OS proc directory to get the current kernel version.
//
// On Linux like systems we can find the kernel version in the file
// "/proc/version" as the third value in the first line of the file.
std::string FetchKernelVersion() {
  const char* kVersionFilePath{"/proc/version"};
  std::ifstream version_file{kVersionFilePath};
  if (!version_file) {
    throw OpenFileError{kVersionFilePath};
  }
  std::string line{};
  if (!std::getline(version_file, line)) {
    throw UnexpectedFormatError{
        std::format("Error while trying to read kernel info from file: {}",
                    kVersionFilePath)};
  }

  std::istringstream str_parser{line};
  std::string kernel_version{};
  // we want to get the third word in the line
  str_parser.ignore(std::numeric_limits<std::streamsize>::max(),
                    static_cast<int>(' '));  // ignore first word
  str_parser.ignore(std::numeric_limits<std::streamsize>::max(),
                    static_cast<int>(' '));  // ignore second word
  str_parser >> kernel_version;
  return kernel_version;
}

// Inspect the release file to get the Operating System name.
//
// On linux systems we can find the OS name in the "/etc/os-release" file in the
// PRETTY_NAME value.
std::string FetchOperatingSystem() {
  const char* kOsReleaseFilePath{"/etc/os-release"};
  const auto parsed_file{
      parser_helper::ExtractKeyValuePairsFromFile(kOsReleaseFilePath, "=")};
  std::string os_name{parsed_file.at("PRETTY_NAME")};
  parser_helper::RemoveDelimiters(os_name);
  return os_name;
}

}  // namespace

System::System()
    : boot_time_{FetchBootTime()},
      kernel_version_{FetchKernelVersion()},
      os_name_{FetchOperatingSystem()} {}

Processor& System::Cpu() { return cpu_; }

vector<Process>& System::Processes() {
  // Since the processes objects have internal state to calculate some metrics
  // like CPU utilization, we can't just clear the list and create new ones. So
  // we query the system for the current running processes and compare with the
  // list that we currenlty have. We add new ones, remove those that are not
  // running anymore and just keep the same object for the ones that are still
  // running.
  std::unordered_set<int> current_pids{FetchProcessIds()};

  std::unordered_set<int> previous_pids{};
  for (const Process& process : processes_) {
    previous_pids.emplace(process.Pid());
  }

  // current_pids - previous_pids
  std::unordered_set<int> pids_to_add{};
  std::set_difference(current_pids.begin(), current_pids.end(),
                      previous_pids.begin(), previous_pids.end(),
                      std::inserter(pids_to_add, pids_to_add.begin()));

  // previous_pids - current_pids
  std::unordered_set<int> pids_to_remove{};
  std::set_difference(previous_pids.begin(), previous_pids.end(),
                      current_pids.begin(), current_pids.end(),
                      std::inserter(pids_to_remove, pids_to_remove.begin()));

  // remove obsolete process objects
  if (!pids_to_remove.empty()) {
    processes_.erase(
        std::remove_if(processes_.begin(), processes_.end(),
                       [&pids_to_remove](const Process& process) {
                         return pids_to_remove.find(process.Pid()) !=
                                pids_to_remove.end();
                       }),
        processes_.end());
  }

  // add new process objects for new pids
  for (const int pid : pids_to_add) {
    processes_.emplace_back(pid, boot_time_, &uid_resolver_);
  }
  std::sort(processes_.begin(), processes_.end());

  return processes_;
}

std::string System::Kernel() const { return kernel_version_; }

float System::MemoryUtilization() const {
  // On Unix like systems the memory info can be fetched in the file
  // /proc/meminfo. We decided to calculate the memory utilization considering
  // the difference between the total system memory and the available memory
  // reported by Operating System. So buffered and cached RAM usage is also
  // considered.
  const char* kMemoryInfoFilePath{"/proc/meminfo"};
  const auto map =
      parser_helper::ExtractKeyValuePairsFromFile(kMemoryInfoFilePath, ":");

  // All the values in the meminfo file have the format "[0-9]+ kB", so we
  // ignore the suffix kB to extract the number value.
  const std::string& total_memory_kB_str{map.at("MemTotal")};
  const long total_memory_kB =
      std::stol(total_memory_kB_str.substr(0, total_memory_kB_str.size() - 3));
  const std::string& free_memory_kB_str{map.at("MemFree")};
  const long free_memory_kB =
      std::stol(free_memory_kB_str.substr(0, free_memory_kB_str.size() - 3));
  return ((total_memory_kB - free_memory_kB) * 1.0 / (total_memory_kB));
}

std::string System::OperatingSystem() const { return os_name_; }

int System::RunningProcesses() const {
  // On Linux systems we can find the number of running processes in the
  // "/proc/stat" file as the procs_running property.
  const char* kStatFilePath{"/proc/stat"};
  const auto map =
      parser_helper::ExtractKeyValuePairsFromFile(kStatFilePath, " ");
  return std::stoi(map.at("procs_running"));
}

int System::TotalProcesses() const {
  // On Linux systems we can find the total number of processes in the
  // "/proc/stat" file as the processes property.
  const char* kStatFilePath{"/proc/stat"};
  const auto map =
      parser_helper::ExtractKeyValuePairsFromFile(kStatFilePath, " ");
  return std::stoi(map.at("processes"));
}

long int System::UpTime() const {
  // The uptime is calculated by comparing the actual time with the stored
  // boot time.
  using std::chrono::duration_cast;
  using std::chrono::seconds;
  using std::chrono::system_clock;

  const system_clock::time_point now_time_point = system_clock::now();
  return duration_cast<seconds>(now_time_point - boot_time_).count();
}
