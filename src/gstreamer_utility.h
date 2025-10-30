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

#include <glibmm.h>
#include <list>

#define GSTREAMERMM_CHECK_VERSION(major, minor, micro) \
  (GSTREAMERMM_MAJOR_VERSION > (major) ||              \
   (GSTREAMERMM_MAJOR_VERSION == (major) &&            \
    GSTREAMERMM_MINOR_VERSION > (minor)) ||            \
   (GSTREAMERMM_MAJOR_VERSION == (major) &&            \
    GSTREAMERMM_MINOR_VERSION == (minor) &&            \
    GSTREAMERMM_MICRO_VERSION >= (micro)))

namespace gstreamer_utility {

// Return time (nanoseconds) as string (h:mm:ss)
// time = GstClockTime = gint64
Glib::ustring time_to_string(gint64 nanoseconds);

// Display a message for missing plugins.
void dialog_missing_plugins(const std::list<Glib::ustring> &missings);

// Checks if the element exists and whether its version is at least the version
// required. Display a dialog error if failed.
bool check_registry(const Glib::ustring &name, int min_major, int min_minor,
                    int min_micro);

}  // namespace gstreamer_utility
