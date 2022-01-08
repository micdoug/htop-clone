#ifndef PROCESS_H
#define PROCESS_H

#include <chrono>
#include <string>

#include "uid_resolver.h"

// Represent a system running process. You can use it to retrieve some metrics
// related to the process.
class Process {
 public:
  // Constructor.
  //
  // Parameters:
  //  - pid: The process id.
  //  - boot_time: The system boot time.
  //  - uid_resolver: It is used to retrieve the name of the user running this
  //  process.
  explicit Process(const int pid,
                   const std::chrono::system_clock::time_point boot_time,
                   UidResolver* uid_resolver);

  // Get the process PID.
  int Pid() const;
  // Get the the user name that is running this process.
  std::string User() const;
  // Get the command line used to start this process.
  std::string Command() const;
  // Get the process CPU utilization in percent and in the interval [0, 1.0].
  // The first time it is called, it returns the average use of CPU for the
  // lifetime of the process so far. Consecutive calls will calculate the usage
  // of CPU in the interval of the last and current call.
  float CpuUtilization();
  // Get the amount of RAM allocated by this process in megabytes.
  std::string Ram();
  // Get the time in which this process has been running, in seconds.
  long int UpTime() const;
  // Sort the process by UID and PID.
  bool operator<(Process const& a) const;

 private:
  int pid_{};
  std::chrono::system_clock::time_point boot_time_{};
  std::chrono::system_clock::time_point process_start_time_{};
  std::string command_line_{};
  int uid_{};
  // Stores the name of the user that is running this process.
  std::string username_{};
  // Stores the last time we checked for CPU utilization.
  std::chrono::system_clock::time_point last_cpu_utilization_check_{};
  // Stores the last value of time spent in CPU in milliseconds.
  long last_cpu_time_spent_ms_{};
};

#endif
