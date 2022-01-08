#ifndef NCURSES_DISPLAY_H
#define NCURSES_DISPLAY_H

#include <curses.h>

#include "process.h"
#include "system.h"

namespace NCursesDisplay {
// Displays the main program UI.
//
// Parameters:
//  - system: The system from which we are collecting metrics to show.
//  - n: The number of processes that we want to show information about.
void Display(System& system, int n = 20);

// Mount the basic system info section in the UI (the top part of the screen).
//
// Parameters:
//  - system: The system from which we are collecting metrics to show.
//  - window: The window that we want to mount the UI on.
void DisplaySystem(System& system, WINDOW* window);

// Mount the processes detail portion of the UI (the bottom part of the screen).
//
// Parameters:
//  - processes: The set of processes that we are collecting metrics to show.
//  - window: The window that we want mount the UI on.
//  - n: The number of processes that we want to show information about.
void DisplayProcesses(std::vector<Process>& processes, WINDOW* window, int n);

// Build a progress bar to attach in the UI.
//
// Parameters:
//  - percent: The percent of completion of the progress bar. Should be in the
//  interval [0, 1.0].
std::string ProgressBar(float percent);
};  // namespace NCursesDisplay

#endif
