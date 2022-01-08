#include "processor.h"

#include <fstream>
#include <limits>
#include <sstream>
#include <string>

#include "format.h"

float Processor::Utilization() {
  const char* kStatFilePath{"/proc/stat"};

  std::ifstream stat_file{kStatFilePath};
  if (!stat_file) {
    throw std::logic_error(
        std::format("Could not open the stat file: {}", kStatFilePath));
  }

  // We need to parse the first line of the stat file. It has the following
  // format: cpu <user> <nice> <system> <idle> <iowait> <irq> <softirq> <steal>
  // <guest> <guest_nice>
  //
  // Each one of the aforementioned values are expressed in USER_HZ (a relative
  // time measurement unit). The meaning of the attributes are the following:
  // - user: normal processes executing in user mode
  // -nice: niced processes executing in user mode
  // - system: processes executing in kernel mode
  // - idle: twiddling thumbs
  // - iowait: In a word, iowait stands for waiting for I/O to complete. But
  // there
  //   are several problems:
  //   1. Cpu will not wait for I/O to complete, iowait is the time that a task
  //   is
  //      waiting for I/O to complete. When cpu goes into idle state for
  //      outstanding task io, another task will be scheduled on this CPU.
  //   2. In a multi-core CPU, the task waiting for I/O to complete is not
  //   running
  //      on any CPU, so the iowait of each CPU is difficult to calculate.
  //   3. The value of iowait field in /proc/stat will decrease in certain
  //      conditions.
  //   So, the iowait is not reliable by reading from /proc/stat.
  // - irq: servicing interrupts
  // - softirq: servicing softirqs
  // - steal: involuntary wait
  // - guest: running a normal guest
  // - guest_nice: running a niced guest

  std::string line{};
  if (!std::getline(stat_file, line)) {
    throw std::logic_error(std::format(
        "Could not read first line from stat file: {}", kStatFilePath));
  }
  std::istringstream line_parser{line};
  // discard cpu prefix
  line_parser.ignore(std::numeric_limits<std::streamsize>::max(), ' ');
  long user{};
  line_parser >> user;
  long nice{};
  line_parser >> nice;
  long system{};
  line_parser >> system;
  long idle{};
  line_parser >> idle;
  long iowait{};
  line_parser >> iowait;
  long irq{};
  line_parser >> irq;
  long softirq{};
  line_parser >> softirq;
  long steal{};
  line_parser >> steal;
  long guest{};
  line_parser >> guest;
  long guest_nice{};
  line_parser >> guest_nice;

  // The time the processor was active, i.e., doing work.
  long operation_time = user + nice + system + irq + softirq + steal;
  // The time the processor was in idle state.
  long idle_time = idle + iowait;
  long total_time = operation_time + idle_time;

  // We compute the usage based on the delta of processor time between the last
  // time we measured and now.
  long total_delta = total_time - previous_total_time_;
  long idle_delta = idle_time - previous_idle_time_;

  // Store the current values for the next measurement.
  previous_total_time_ = total_time;
  previous_idle_time_ = idle_time;

  float usage = (total_delta - idle_delta) * 1.0 / total_delta;
  return usage;
}
