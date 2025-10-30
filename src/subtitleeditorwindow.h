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

#include <gtkmm.h>

class Document;
class Player;
class WaveformManager;

class SubtitleEditorWindow {
 public:
  // init static window pointer with this instance
  SubtitleEditorWindow();

  virtual ~SubtitleEditorWindow();

  virtual Glib::RefPtr<Gtk::UIManager> get_ui_manager() = 0;

  virtual Document* get_current_document() = 0;

  virtual std::vector<Document*> get_documents() = 0;

  virtual Player* get_player() = 0;

  virtual WaveformManager* get_waveform_manager() = 0;

  static SubtitleEditorWindow* get_instance();

 protected:
  static SubtitleEditorWindow* m_static_window;
};
