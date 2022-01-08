#include "process.h"

#include <unistd.h>

#include <cctype>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "errors.h"
#include "format.h"
#include "parser_helper.h"

using std::string;
using std::to_string;
using std::vector;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::system_clock;

namespace {

// Return the start time of the process.
//
// In Linux systems we can find the time a process started after the system
// boot in the "/proc/<pid>/stat" file as the 22th property of the first line.
// We use this value combined with the system boot time to discover the process
// start time.
system_clock::time_point CalculateProcessStartTime(
    const int pid, const system_clock::time_point system_boot_time) {
  const char* kStatFilePathMask{"/proc/{}/stat"};
  std::string file_path{std::format(kStatFilePathMask, pid)};
  std::ifstream stat_file{file_path};
  if (!stat_file) {
    // The process related files can be deleted between the time we discover its
    // pid and we try to get information about it. In this case the process will
    // be removed in the next iteration. So we just return a dummy value here.
    return system_boot_time;
  }
  std::string line{};
  if (!std::getline(stat_file, line)) {
    // It is the same case of the file not existing. Se the previous comment.
    return system_boot_time;
  }
  std::istringstream line_parser{line};
  // ignore the first 21 words
  for (int i = 0; i < 21; i++) {
    line_parser.ignore(std::numeric_limits<std::streamsize>::max(), ' ');
  }

  // this is the process start time after the boot time
  long long start_time{};
  line_parser >> start_time;
  start_time /= sysconf(_SC_CLK_TCK);  // since it is expressed in clock ticks,
                                       // we need to convert to seconds
  return system_boot_time + seconds{start_time};
}

// Fetch the command line used to launch the process.
//
// In Linux systems the command used to launch the process is stored in the
// "/proc/<pid>/cmdline" file.
std::string FetchCommandLine(const int pid) {
  const char* kCommandLineFilePathMask{"/proc/{}/cmdline"};
  std::string file_path{std::format(kCommandLineFilePathMask, pid)};

  std::ifstream cmd_file{file_path};
  if (!cmd_file) {
    // The process related files can be deleted between the time we discover its
    // pid and we try to get information about it. In this case the process will
    // be removed in the next iteration. So we just return a dummy value here.
    return "-";
  }
  std::string line{};
  if (!std::getline(cmd_file, line)) {
    // It is the same case of the file not existing. Se the previous comment.
    return "-";
  }
  return line;
}

// On Linux systems we can find the user that is running a process in the file
// "/proc/<pid>/status" as the Uid property.
std::tuple<int, std::string> FetchProcessOwnerUidAndName(
    const int pid, UidResolver* uid_resolver) {
  const char* kStatusFilePathMask{"/proc/{}/status"};
  std::string file_path{std::format(kStatusFilePathMask, pid)};

  std::ifstream status_file{file_path};
  if (!status_file) {
    // This error can happen if process file is deleted between the time
    // we discover its pid and we try to get information about it. In this case
    // the process will be removed in the next iteration, so we just return a
    // dummy value here.
    return {0, "-"};
  }
  std::string line{};
  while (std::getline(status_file, line)) {
    if (std::string_view{line}.substr(0, 4) == "Uid:") {
      std::istringstream line_parser{line.substr(5)};
      int uid{};
      line_parser >> uid;

      return {uid, uid_resolver->FetchUserName(uid).value_or(
                       std::format("UID({})", uid))};
    }
  }
  return {0, "-"};
}

}  // namespace

Process::Process(const int pid, const system_clock::time_point boot_time,
                 UidResolver* uid_resolver)
    : pid_{pid},
      boot_time_{boot_time},
      process_start_time_{CalculateProcessStartTime(pid, boot_time)},
      command_line_{FetchCommandLine(pid)},
      last_cpu_utilization_check_(CalculateProcessStartTime(pid, boot_time)) {
  const auto& [uid, username] = FetchProcessOwnerUidAndName(pid, uid_resolver);
  uid_ = uid;
  username_ = username;
}

int Process::Pid() const { return pid_; }

float Process::CpuUtilization() {
  // In Linux systems we can calculate the process CPU utilization by inspecting
  // some values found in the file "/proc/<pid>/stat".
  // We calculate the CPU utilization considering the time spent in CPU since
  // the last measurement and the time since last measurement. So in the first
  // call, it will calculate the average usage of cpu since the process started.

  // We have no way to measure the cpu utilization if the process has just
  // started.
  if (UpTime() == 0) {
    return 0;
  }

  const char* kStatFilePathMask{"/proc/{}/stat"};
  std::string file_path{std::format(kStatFilePathMask, Pid())};
  std::ifstream stat_file{file_path};
  if (!stat_file) {
    // The process related files can be deleted between the time we discover its
    // pid and we try to get information about it. In this case the process will
    // be removed in the next iteration. So we just return a dummy value here.
    return 0;
  }
  std::string line{};
  if (!std::getline(stat_file, line)) {
    // It is the same case of the previous comment. We just return a dummy
    // value.
    return 0;
  }
  system_clock::time_point measured_at = system_clock::now();
  std::istringstream line_parser{line};
  // ignore first 13 values in the first line (they are delimited by a ' ' char)
  for (int i = 0; i < 13; i++) {
    line_parser.ignore(std::numeric_limits<std::streamsize>::max(), ' ');
  }
  // Amount of time that this process has been scheduled in  user  mode
  long utime{};  // it is the 14th property in the line.
  line_parser >> utime;

  // Amount  of  time  that this process has been scheduled in kernel mode.
  long stime{};  // it is the 15th property in the line
  line_parser >> stime;

  // Amount of time that this process's waited-for children have been scheduled
  // in user mode
  long cutime{};  // it is the 16th property in the line
  line_parser >> cutime;

  // Amount of time that this process's waited-for children have  been scheduled
  // in  kernel mode
  long cstime{};  // it is the 17th property in the line
  line_parser >> cstime;

  // We are considering cutime and cstime, so the time spent by child processes
  // will be accounted too.
  long total_time_spent_ticks = utime + stime + cutime + cstime;
  long total_time_spent_ms =
      1'000 * total_time_spent_ticks / sysconf(_SC_CLK_TCK);

  long time_since_last_measurement_ms =
      duration_cast<milliseconds>(measured_at - last_cpu_utilization_check_)
          .count();

  long time_spent_delta = total_time_spent_ms - last_cpu_time_spent_ms_;

  last_cpu_utilization_check_ = measured_at;
  last_cpu_time_spent_ms_ = total_time_spent_ms;

  return time_spent_delta * 1.0 / time_since_last_measurement_ms;
}

string Process::Command() const { return command_line_; }

string Process::Ram() {
  // In Linux systems the amount of memory used by a process is available in the
  // file "/proc/<pid>/status" as the VmSize property.
  const char* kStatusFilePathMask{"/proc/{}/status"};
  std::ifstream status_file{std::format(kStatusFilePathMask, Pid())};
  if (!status_file) {
    // The process related files can be deleted between the time we discover its
    // pid and we try to get information about it. In this case the process will
    // be removed in the next iteration. So we just return a dummy value here.
    return "-";
  }
  std::string line{};
  while (std::getline(status_file, line)) {
    if (std::string_view{line}.substr(0, 6) == "VmSize") {
      long memory_usage_in_kB = std::stol(line.substr(7));
      return std::format("{} MB", memory_usage_in_kB /
                                      1024);  // convert to megabytes and return
    }
  }
  return "-";
}

string Process::User() const { return username_; }

long int Process::UpTime() const {
  return duration_cast<seconds>(system_clock::now() - process_start_time_)
      .count();
}

bool Process::operator<(Process const& a) const {
  // This function was implemented in such a way that when sorting a list of
  // processes we promote the ones with higher UIDs (that tend to be the uids of
  // non-reserved system users), and then promote more recent launched jobs
  // (higher pids).
  if (User() == a.User()) {
    return Pid() > a.Pid();
  }
  return uid_ > a.uid_;
}
