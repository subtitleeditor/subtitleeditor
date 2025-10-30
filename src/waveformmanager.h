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

#include "waveform.h"

class WaveformManager {
 public:
  // Virtual destructor
  virtual ~WaveformManager() {
    // nothing
  }

  // Try to open a waveform file and show or hide the editor.
  virtual bool open_waveform(const Glib::ustring &uri) = 0;

  // Init the Waveform Editor and the WaveformRenderer with this wf
  virtual void set_waveform(const Glib::RefPtr<Waveform> &wf) = 0;

  // Return the state of waveform. Cab be NULL.
  virtual bool has_waveform() = 0;

  // Return a pointer to the waveform.
  virtual Glib::RefPtr<Waveform> get_waveform() = 0;

  // A current waveform has changed.
  virtual sigc::signal<void> &signal_waveform_changed() = 0;

  // Try to display the current subtitle at the center of the view.
  virtual void center_with_selected_subtitle() = 0;

  // Increment the zoom
  virtual void zoom_in() = 0;

  // Decrement the zoom
  virtual void zoom_out() = 0;

  // Décrément completely the zoom
  virtual void zoom_all() = 0;

  // Zooming on the current subtitle.
  virtual void zoom_selection() = 0;
};
