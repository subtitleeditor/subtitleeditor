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
static bool profiling_enable = false;
static Glib::Timer profiling_timer;
static double profiling_timer_last = 0.0;

void __se_dbg_init(int flags) {
  debug_flags = flags;

  if (G_UNLIKELY(debug_flags & SE_DBG_PROFILING) &&
      debug_flags != SE_NO_DEBUG) {
    profiling_enable = true;
    profiling_timer.start();
  }
}

bool se_dbg_check_flags(int flag) {
  if (G_UNLIKELY(debug_flags & SE_DBG_ALL))
    return true;

  return G_UNLIKELY(debug_flags & flag);
}

void __se_dbg(int flag, const gchar* file, const gint line,
              const gchar* fonction) {
  if (G_UNLIKELY(debug_flags & flag) || G_UNLIKELY(debug_flags & SE_DBG_ALL)) {
    if (profiling_enable) {
      double seconds = profiling_timer.elapsed();

      g_print("[%f (%f)] %s:%d (%s)\n", seconds, seconds - profiling_timer_last,
              file, line, fonction);
      profiling_timer_last = seconds;
    } else {
      g_print("%s:%d (%s)\n", file, line, fonction);
    }

    fflush(stdout);
  }
}

void __se_dbg_msg(int flag, const gchar* file, gint line, const gchar* fonction,
                  const char* format, ...) {
  if (G_UNLIKELY(debug_flags & flag) || G_UNLIKELY(debug_flags & SE_DBG_ALL)) {
    va_list args;
    gchar* msg = NULL;

    g_return_if_fail(format);

    va_start(args, format);
    msg = g_strdup_vprintf(format, args);
    va_end(args);

    if (profiling_enable) {
      double seconds = profiling_timer.elapsed();

      g_print("[%f (%f)] %s:%d (%s) %s\n", seconds,
              seconds - profiling_timer_last, file, line, fonction, msg);
      profiling_timer_last = seconds;
    } else {
      g_print("%s:%d (%s) %s\n", file, line, fonction, msg);
    }

    fflush(stdout);

    g_free(msg);
  }
}
