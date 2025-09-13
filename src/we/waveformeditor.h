#pragma once

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

#include <gtkmm.h>
#include "player.h"
#include "waveformmanager.h"
#include "waveformrenderer.h"
#include "utility"

class WaveformEditor : public WaveformManager, public Gtk::Box {
 public:
  WaveformEditor(BaseObjectType* cobject,
                 const Glib::RefPtr<Gtk::Builder>& builder);

  ~WaveformEditor();

  // Try to open a waveform file and show or hide the editor.
  bool open_waveform(const Glib::ustring& uri);

  // Init the Waveform Editor and the WaveformRenderer with this wf
  void set_waveform(const Glib::RefPtr<Waveform>& wf);

  // Return the state of waveform. Cab be NULL.
  bool has_waveform();

  // Return a pointer to the waveform.
  Glib::RefPtr<Waveform> get_waveform();

  // A current waveform has changed.
  sigc::signal<void>& signal_waveform_changed();

  // FIXME HACK
  void set_player(Player* player);

  // return the player (like internal GStreamer Video player)
  Player* player();

  void on_player_tick(long current_time, long stream_length,
                      double current_position);

  // Return the current time of the player.
  long get_player_time();

  void on_config_waveform_changed(const Glib::ustring& key,
                                  const Glib::ustring& value);

  // Try to display the current subtitle at the center of the view.
  void center_with_selected_subtitle();

  // Increment the zoom
  void zoom_in();

  // Decrement the zoom
  void zoom_out();

  // Décrément completely the zoom
  void zoom_all();

  // Zooming on the current subtitle.
  void zoom_selection();

 protected:
  Gtk::Widget* create_control_widget();

  // Enable the signal tick (Player)
  void on_map();

  // Disable the signal tick (Player).
  void on_unmap();

  // The scroll bar depend on the size of the waveform widget.
  // This callback is connected to the signal "configure" of the waveform frame
  // (Gtk::Frame). Every time this size changed, the scrollbar need to be
  // recalculate.
  bool on_configure_event_waveform(GdkEventConfigure* ev);

  // Edit the position of the current subtitle.
  // Start the recorder command.
  bool on_button_press_event_renderer(GdkEventButton* ev);

  // Finish the editing of the current subtitle.
  // Stop the recorder command.
  bool on_button_release_event_renderer(GdkEventButton* ev);

  // Adjust the position of the current subtitle.
  bool on_motion_notify_event_renderer(GdkEventMotion* ev);

  // Manage the scrolling like the current position in the view (like
  // scrollbar), the scale or the zoom.
  bool on_scroll_event_renderer(GdkEventScroll* ev);

  void set_child_sensitive(bool status);

 protected:
  void load_config();

  // The editor has a renderer ?
  bool has_renderer();

  // Return the renderer. Can be NULL.
  WaveformRenderer* renderer();

  // Redisplay the renderer (call renderer->redraw_all)
  void redraw_renderer();

  // Return the state of current document.
  bool has_document();

  // Return a pointer to the current document. Can be NULL.
  Document* document();

  // Set the value of the scale (widget).
  void set_scale(float value);

  // Return the value of the scale (widget).
  float get_scale();

  // Set the value of the zoom (widget).
  void set_zoom(int value);

  // Return the value of the zoom (widget).
  int get_zoom();

  // Return the value of the scrolling (scrollbar)
  int get_scrolling();

  // Initialize the scrollbar depending
  // on the size of the widget renderer (waveform)
  // and the value of the zoom.
  void init_scrollbar();

  // The value of the scrollbar has changed.
  // Update the waveform renderer with the new value.
  void on_scrollbar_value_changed();

  // The value of the zoom has changed.
  // Call init_scrollbar and updates the config.
  // Redraw the waveform.
  void on_zoom_changed();

  // The value of the scale has changed.
  // Redraw the waveform.
  void on_scale_changed();

  // This callback is connected on the realize signal.
  // It's used to create the renderer because some need a realized parent.
  void on_create_renderer();

  // Initializes the signals of the renderer like the button pressed, released
  // ... Add events to the widget.
  void init_renderer(WaveformRenderer* renderer);

  // Initialize the editor with the document.
  // This callback is also connected at
  // "se::documents::signal_active_changed"
  void init_document(Document* doc);

  // This callback is connected at the current document.
  // The document has changed, it's need to redraw the view.
  void on_document_changed();

  // This callback is connected at the current document.
  // It's call when the selection of the subtitles has changed.
  // The view is centered with the new selection if the option is enable.
  // It's need to redraw the view.
  void on_subtitle_selection_changed();

  // This callback is connected at the current document.
  // The time of subtitle has changed, it's need to redraw the view.
  void on_subtitle_time_changed();

  // This callback is connected at the player.
  // The keyframes has changed, it's need to redraw the view.
  void on_player_message(Player::Message msg);

  // Go at the position on the scrollbar.
  // A little margin is added in the border.
  void scroll_to_position(int position);

  // Go at the position on the scrollbar and
  // try to place at the center of the view.
  void scroll_to_position_and_center(int position);

  // If scrolling with player is enabled,
  // scroll with the current time of the player.
  void scroll_with_player();

  // If select with player is enabled,
  // select current subtitles with the current time of the player.
  std::pair<long, long>  select_with_player();
  // Used to keep track whether player position is within a subtitle
  // in that case no need to look for another subtitle
  // when select_with_player is used. Though top says this optimalizations
  // is not noticeable. Anyway, this option when enabled
  // does not raise CPU value. 
  std::pair<long, long> subtitle_start_and_end_times;

  // Try to move the beginning of the current subtitle.
  // If the option 'respect-timing' is enabled,
  // try to respect the timing preferences.
  // disable_respect:
  // No test is doing if 'disable_respect' is enabled.
  // around:
  // If is true, the end of the previous subtitle can be moved
  // if necessary with respect of timing preferences.
  bool move_subtitle_start(const SubtitleTime& time, bool disable_respect,
                           bool around);

  // Try to move the end of the current subtitle.
  // If the option 'respect-timing' is enabled,
  // try to respect the timing preferences.
  // disable_respect:
  // No test is doing if 'disable_respect' is enabled.
  // around:
  // If is true, the beginning of the next subtitle can be moved
  // if necessary with respect of timing preferences.
  bool move_subtitle_end(const SubtitleTime& time, bool disable_respect,
                         bool around);

 protected:
  Gtk::Frame* m_frameWaveformRenderer;
  Gtk::Scrollbar* m_hscrollbarWaveformRenderer;
  Gtk::Scale* m_sliderZoom;
  Gtk::Scale* m_sliderScale;

  Glib::RefPtr<Waveform> m_waveform;
  WaveformRenderer* m_waveformRenderer;  // widget Gtk::DrawingArea
  sigc::signal<void> m_signal_waveform_changed;

  Document* m_document;
  std::vector<sigc::connection> m_document_connection;

  bool m_cfg_scrolling_with_player;
  bool m_cfg_select_with_player;
  bool m_cfg_scrolling_with_selection;
  bool m_cfg_respect_timing;

  Player* m_player;
  sigc::connection m_connection_player_tick;
};
