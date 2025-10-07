// subtitleeditor -- a tool to create or edit subtitle
//
// https://kitone.github.io/subtitleeditor/
// https://github.com/kitone/subtitleeditor/
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

#include <config.h>
#include <gtkmm/main.h>

#include <iostream>

#include "gtkmm_utility.h"
#include "gui/application.h"
#include "options.h"
#include "utility.h"

// #include <gdk/gdkx.h>
#include <glib.h>
#include <gstreamermm.h>

#include <ctime>

#ifdef ENABLE_GL
#include <gtkglmm.h>
#endif  // ENABLE_GL

int main(int argc, char *argv[]) {
  if (!g_thread_supported())
    g_thread_init(NULL);

  bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);

  // init Gtk+
  Gtk::Main kit(argc, argv);

#ifdef ENABLE_GL
  // init OpenGL
  Gtk::GL::init_check(argc, argv);
#endif  // ENABLE_GL

  Glib::set_application_name("subtitleeditor");

  // SubtitleEditor Options
  OptionGroup options;
  try {
    Glib::OptionContext context(_(" — edit subtitles files"));
    context.set_main_group(options);

    Glib::OptionGroup gst_group(gst_init_get_option_group());
    context.add_group(gst_group);

    Glib::OptionGroup gtk_group(gtk_get_option_group(TRUE));
    context.add_group(gtk_group);

    context.parse(argc, argv);
  } catch (const Glib::Error &ex) {
    std::cerr << "Error loading options : " << ex.what() << std::endl;
  }

  // Init the debug options
  se_dbg_init(options.get_debug_flags());
  se_dbg_msg(SE_DBG_APP, "Startup subtitle version %s", VERSION);

  // If the user want to use a other profile
  // this is the last time we can do that.
  if (!options.profile.empty())
    set_profile_name(options.profile);

  se_dbg_msg(SE_DBG_APP, "Init GStreamer");

  Gst::init(argc, argv);

  // Run Application
  Application *application = gtkmm_utility::get_widget_derived<Application>(
      SE_DEV_VALUE(PACKAGE_UI_DIR, PACKAGE_UI_DIR_DEV), "subtitleeditor.ui",
      "window-main");
  if (!application)
    return EXIT_FAILURE;

  application->init(options);
  application->show();

  se_dbg_msg(SE_DBG_APP, "Run the main loop");

  kit.run(*application);

  delete application;

  return EXIT_SUCCESS;
}
