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

#include "action.h"

#include "debug.h"
#include "document.h"

Action::Action() {
}

Action::~Action() {
}

SubtitleEditorWindow* Action::get_subtitleeditor_window() {
   se_dbg(SE_DBG_PLUGINS);

   SubtitleEditorWindow* window = SubtitleEditorWindow::get_instance();

   return window;
}

Document* Action::get_current_document() {
   se_dbg(SE_DBG_PLUGINS);

   SubtitleEditorWindow* window = SubtitleEditorWindow::get_instance();

   g_return_val_if_fail(window, NULL);

   Document* doc = window->get_current_document();

   se_dbg_msg(SE_DBG_PLUGINS, "document=%s", ((doc == NULL) ? "NULL" : doc->getFilename().c_str()));

   return doc;
}

Glib::RefPtr<Gtk::UIManager> Action::get_ui_manager() {
   se_dbg(SE_DBG_PLUGINS);

   SubtitleEditorWindow* window = SubtitleEditorWindow::get_instance();

   g_return_val_if_fail(window, Glib::RefPtr<Gtk::UIManager>());

   return window->get_ui_manager();
}

void Action::activate() {
   se_dbg(SE_DBG_PLUGINS);
}

void Action::deactivate() {
   se_dbg(SE_DBG_PLUGINS);
}

void Action::update_ui() {
   se_dbg(SE_DBG_PLUGINS);
}
