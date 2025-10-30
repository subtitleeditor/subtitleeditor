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

#include <gst/gst.h>
#include "gstreamer_utility.h"
#include "utility.h"

namespace gstreamer_utility {

// Return time (nanoseconds) as string (h:mm:ss)
// time = GstClockTime = gint64
Glib::ustring time_to_string(gint64 time) {
  gchar *str =
      g_strdup_printf("%u:%02u:%02u", (guint)(time / (GST_SECOND * 60 * 60)),
                      (guint)((time / (GST_SECOND * 60)) % 60),
                      (guint)((time / GST_SECOND) % 60));

  Glib::ustring res(str);
  g_free(str);
  return res;
}

// Display a message for missing plugins.
void dialog_missing_plugins(const std::list<Glib::ustring> &list) {
  Glib::ustring plugins;

  for (const auto &el : list) {
    plugins += el;
    plugins += "\n";
  }

  Glib::ustring msg =
      _("GStreamer plugins missing.\n"
        "The playback of this movie requires the following decoders "
        "which are not installed:");

  dialog_error(msg, plugins);

  se_dbg_msg(SE_DBG_UTILITY, "%s %s", msg.c_str(), plugins.c_str());
}

// Checks if the element exists and whether its version is at least the version
// required. Display a dialog error if failed.
bool check_registry(const Glib::ustring &name, int min_major, int min_minor,
                    int min_micro) {
  // FIXME: gstreamer 1.0
  // if(gst_default_registry_check_feature_version(name.c_str(), min_major,
  // min_minor, min_micro))
  return true;

  dialog_error(build_message(_("Failed to create a GStreamer element '%s'."),
                             name.c_str()),
               _("Please check your GStreamer installation."));
  return false;
}

}  // namespace gstreamer_utility
