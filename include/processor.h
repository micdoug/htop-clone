#ifndef PROCESSOR_H
#define PROCESSOR_H

// Represent the processor in the machine. You can use it to retrieve some
// metrics.
class Processor {
 public:
  // Calculate the CPU utilization in percent and in the interval [0, 1.0].
  // For multicore machines (that it is the usual nowadays) it consider the
  // average use of all cores. The first call returns the average use of CPU for
  // entire machine uptime. Consecutive calls will calculate the usage
  // considering the interval between the last and current call.
  float Utilization();

 private:
  // Stores the previous total time of CPU available in the system.
  long previous_total_time_{};
  // Stores the previous CPU idle time in the system.
  long previous_idle_time_{};
};

#endif
