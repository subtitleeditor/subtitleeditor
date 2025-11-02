// subtitleeditor -- a tool to create or edit subtitle
//
// https://subtitleeditor.github.io/subtitleeditor/
// https://github.com/subtitleeditor/subtitleeditor/
//
// Copyright @ 2005-2018, kitone
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.


#include <glibmm/timer.h>
#include <iostream>
#include <string>
#include "debug.h"

static int debug_flags = SE_NO_DEBUG;

// PROFILING
static bool profiling_enable = true;
static Glib::Timer profiling_timer;
static double profiling_timer_last = 0.0;

// Helper function to get profiling prefix (returns empty when profiling disabled)
static std::string get_profiling_prefix() {
  if (!profiling_enable) {
    return "";
  }

  double seconds = profiling_timer.elapsed();
  char buffer[64];
  g_snprintf(buffer, sizeof(buffer), "[%f (%f)] ",
             seconds, seconds - profiling_timer_last);
  profiling_timer_last = seconds;
  return std::string(buffer);
}

void __se_dbg_init(int flags) {
  debug_flags = flags;

  // Profiling enabled by default, disable if "debug-no-profiling" is set
  if (G_UNLIKELY(debug_flags & SE_DBG_NO_PROFILING) &&
      debug_flags != SE_NO_DEBUG) {
    profiling_enable = false;
    profiling_timer.start();
  }
}

bool se_dbg_check_flags(int flag) {
  if (G_UNLIKELY(debug_flags & SE_DBG_ALL))
    return true;

  return G_UNLIKELY(debug_flags & flag);
}

void __se_dbg(int flag, const gchar* file, const gint line,
              const gchar* function) {
  if (G_UNLIKELY(debug_flags & flag) || G_UNLIKELY(debug_flags & SE_DBG_ALL)) {
    std::string prefix = get_profiling_prefix();
    g_print("%s%s:%d (%s)\n", prefix.c_str(), file, line, function);
    fflush(stdout);
  }
}

void __se_dbg_msg(int flag, const gchar* file, gint line, const gchar* function,
                  const char* format, ...) {
  if (G_UNLIKELY(debug_flags & flag) || G_UNLIKELY(debug_flags & SE_DBG_ALL)) {
    va_list args;
    gchar* msg = NULL;

    g_return_if_fail(format);

    va_start(args, format);
    msg = g_strdup_vprintf(format, args);
    va_end(args);
    std::string prefix = get_profiling_prefix();
    g_print("%s%s:%d (%s) %s\n", prefix.c_str(), file, line, function, msg);
    fflush(stdout);

    g_free(msg);
  }
}
