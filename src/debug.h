#pragma once

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <stdio.h>

#include <string>

enum SE_DBG_MESSAGE_FLAG {
   SE_NO_DEBUG = 0,
   SE_DBG_INFORMATION = 1 << 0,  // simple message ex: "subtitleeditor start, ..."

   SE_DBG_APP = 1 << 1,
   SE_DBG_VIEW = 1 << 2,
   SE_DBG_IO = 1 << 3,
   SE_DBG_SEARCH = 1 << 4,
   SE_DBG_REGEX = 1 << 5,
   SE_DBG_VIDEO_PLAYER = 1 << 6,
   SE_DBG_SPELL_CHECKING = 1 << 7,
   SE_DBG_WAVEFORM = 1 << 8,
   SE_DBG_UTILITY = 1 << 9,
   SE_DBG_COMMAND = 1 << 10,
   SE_DBG_PLUGINS = 1 << 11,
   SE_DBG_NO_PROFILING = 1 << 12,

   SE_DBG_ALL = 1 << 20
};

void __se_dbg_init(int flags);

bool se_dbg_check_flags(int flags);

void __se_dbg(int flag, const gchar* file, gint line, const gchar* function);

void __se_dbg_msg(int flag, const gchar* file, gint line, const gchar* function, const char* string, ...);

void __se_dbg_msg(int flag, const gchar* file, gint line, const gchar* function, const std::string& message);

#ifdef DEBUG

#define se_dbg_init(flags) __se_dbg_init(flags);

#define se_dbg(flag)                                    \
   if (se_dbg_check_flags(flag)) {                      \
      __se_dbg(flag, __FILE__, __LINE__, __FUNCTION__); \
   }

#define se_dbg_msg(flag, ...)                                            \
   if (se_dbg_check_flags(flag)) {                                       \
      __se_dbg_msg(flag, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); \
   }

#else  // DEBUG
#define se_dbg_init(flags)
#define se_dbg(flag)
#define se_dbg_msg(flag, ...)
#endif  // DEBUG
