#ifndef SYSTEM_H
#define SYSTEM_H

#include <chrono>
#include <ctime>
#include <string>
#include <unordered_set>
#include <vector>

#include "process.h"
#include "processor.h"
#include "uid_resolver.h"

// Represent an entire Linux based computer system. You can use it to get some
// metrics about the computer operation.
class System {
 public:
  // Basic constructor.
  explicit System();
  // Get the system's CPU instance.
  Processor& Cpu();
  // Get the list of running processes.
  std::vector<Process>& Processes();
  // Get the memory utilization in megabytes (it considers the cached and
  // buffered usage also).
  float MemoryUtilization() const;
  // Get the system uptime in seconds.
  long UpTime() const;
  // Get the total number of processes in the system.
  int TotalProcesses() const;
  // Get the number of current running processes.
  int RunningProcesses() const;
  // Get the current kernel description.
  std::string Kernel() const;
  // Get the current Linux system version description.
  std::string OperatingSystem() const;

 private:
  Processor cpu_{};
  std::vector<Process> processes_{};
  std::chrono::system_clock::time_point boot_time_{};
  std::string kernel_version_{};
  std::string os_name_{};
  UidResolver uid_resolver_{};
};

#endif
