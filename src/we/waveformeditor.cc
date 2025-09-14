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

#include "documents.h"
#include "subtitleeditorwindow.h"
#include "utility.h"
#include "waveformeditor.h"
#include <utility> // the three following are for std:: min abs and pair
#include <algorithm>
#include <cmath>

// HACK!
WaveformRenderer *create_waveform_renderer_cairo();

#ifdef ENABLE_GL
WaveformRenderer *create_waveform_renderer_gl();
#endif  // ENABLE_GL

WaveformEditor::WaveformEditor(BaseObjectType *cobject,
                               const Glib::RefPtr<Gtk::Builder> &builder)
    : Gtk::Box(cobject),
      m_waveformRenderer(NULL),
      m_document(NULL),
      m_player(NULL) {
  set_size_request(240, 240);

  builder->get_widget("frame-waveform-renderer", m_frameWaveformRenderer);
  builder->get_widget("hscrollbar-waveform-renderer",
                      m_hscrollbarWaveformRenderer);

  builder->get_widget("vscale-zoom", m_sliderZoom);
  builder->get_widget("vscale-scale", m_sliderScale);

  // connect signal

  m_sliderZoom->signal_value_changed().connect(
      sigc::mem_fun(*this, &WaveformEditor::on_zoom_changed));

  m_sliderScale->signal_value_changed().connect(
      sigc::mem_fun(*this, &WaveformEditor::on_scale_changed));

  m_hscrollbarWaveformRenderer->signal_value_changed().connect(
      sigc::mem_fun(*this, &WaveformEditor::on_scrollbar_value_changed));

  // create the renderer after the widget is realized.
  signal_realize().connect(
      sigc::mem_fun(*this, &WaveformEditor::on_create_renderer), true);

  se::documents::signal_active_changed().connect(
      sigc::mem_fun(*this, &WaveformEditor::init_document));


  
  // FIXME init
  {
    m_cfg_scrolling_with_player = true;
    m_cfg_select_with_player = true;
    subtitle_start_and_end_times = {0, 0};
    m_cfg_scrolling_with_selection = true;
    // m_cfg_respect_min_display = true;
    // m_cfg_respect_gab_between_subtitles = true;
    m_cfg_respect_timing = true;
  }

  load_config();

  cfg::signal_changed("waveform")
      .connect(
          sigc::mem_fun(*this, &WaveformEditor::on_config_waveform_changed));

  set_child_sensitive(false);
}

WaveformEditor::~WaveformEditor() {
  se_dbg(SE_DBG_WAVEFORM);
}

void WaveformEditor::load_config() {
  se_dbg(SE_DBG_WAVEFORM);

  if (cfg::has_key("waveform", "scale"))
    m_sliderScale->set_value(cfg::get_int("waveform", "scale"));
  else
    m_sliderScale->set_value(3);

  if (cfg::has_key("waveform", "zoom"))
    m_sliderZoom->set_value(cfg::get_int("waveform", "zoom"));
  else
    m_sliderZoom->set_value(1);

  m_cfg_scrolling_with_player =
      cfg::get_boolean("waveform", "scrolling-with-player");

  m_cfg_select_with_player =
      cfg::get_boolean("waveform", "select-with-player");

  m_cfg_scrolling_with_selection =
      cfg::get_boolean("waveform", "scrolling-with-selection");

  m_cfg_respect_timing = cfg::get_boolean("waveform", "respect-timing");

  if (cfg::get_boolean("waveform", "display")) {
    show();
  } else {
    hide();
  }
}

// This callback is connected on the realize signal.
// It's used to create the renderer because some need a realized parent.
void WaveformEditor::on_create_renderer() {
  se_dbg(SE_DBG_WAVEFORM);

  Glib::ustring renderer_name = cfg::get_string("waveform", "renderer");

#ifdef ENABLE_GL
  if (renderer_name == "gl") {
    init_renderer(create_waveform_renderer_gl());
  } else if (renderer_name == "cairo") {
    init_renderer(create_waveform_renderer_cairo());
  } else {  // cairo by default
    init_renderer(create_waveform_renderer_cairo());
  }
#else   // ENABLE_GL
  if (renderer_name == "cairo") {
    init_renderer(create_waveform_renderer_cairo());
  } else {  // cairo by default
    init_renderer(create_waveform_renderer_cairo());
  }
#endif  // ENABLE_GL
}

// Initialize the editor with the document.
// This callback is also connected at
// "se::documents::signal_active_changed"
void WaveformEditor::init_document(Document *doc) {
  se_dbg(SE_DBG_WAVEFORM);

  m_document = NULL;
  for (auto &doc_connection : m_document_connection) {
    doc_connection.disconnect();
  }
  m_document_connection.clear();

  if (doc != NULL) {
    m_document = doc;

#define CONNECT(signal, callback)                                  \
  m_document_connection.push_back(doc->get_signal(signal).connect( \
      sigc::mem_fun(*this, &WaveformEditor::callback)));

    CONNECT("document-changed", on_document_changed);
    CONNECT("subtitle-selection-changed", on_subtitle_selection_changed);
    CONNECT("subtitle-time-changed", on_subtitle_time_changed);

#undef CONNECT

    init_scrollbar();
  }

  redraw_renderer();
}

// Initializes the signals of the renderer like the button pressed, released ...
// Add events to the widget.
void WaveformEditor::init_renderer(WaveformRenderer *renderer) {
  se_dbg(SE_DBG_WAVEFORM);

  // Remove the old renderer and destroy.
  if (m_waveformRenderer != NULL) {
    Gtk::Widget *child = m_frameWaveformRenderer->get_child();

    m_frameWaveformRenderer->remove();

    delete child;

    m_waveformRenderer = NULL;
  }

  if (renderer) {
    m_waveformRenderer = renderer;

    renderer->signal_document().connect(
        sigc::mem_fun(*this, &WaveformEditor::document));

    renderer->signal_scale().connect(
        sigc::mem_fun(*this, &WaveformEditor::get_scale));

    renderer->signal_zoom().connect(
        sigc::mem_fun(*this, &WaveformEditor::get_zoom));

    renderer->signal_scrolling().connect(
        sigc::mem_fun(*this, &WaveformEditor::get_scrolling));

    renderer->player_time.connect(
        sigc::mem_fun(*this, &WaveformEditor::get_player_time));

    renderer->set_waveform(get_waveform());

    Gtk::Widget *widget = renderer->widget();

    m_frameWaveformRenderer->add(*widget);

    widget->add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
                       Gdk::BUTTON_MOTION_MASK | Gdk::SCROLL_MASK);

    widget->signal_configure_event().connect(
        sigc::mem_fun(*this, &WaveformEditor::on_configure_event_waveform),
        true);

    widget->signal_button_press_event().connect(
        sigc::mem_fun(*this, &WaveformEditor::on_button_press_event_renderer));

    widget->signal_button_release_event().connect(sigc::mem_fun(
        *this, &WaveformEditor::on_button_release_event_renderer));

    widget->signal_motion_notify_event().connect(
        sigc::mem_fun(*this, &WaveformEditor::on_motion_notify_event_renderer));

    widget->signal_scroll_event().connect(
        sigc::mem_fun(*this, &WaveformEditor::on_scroll_event_renderer));

    init_document(document());

    widget->show();
  }
}

void WaveformEditor::set_player(Player *player) {
  se_dbg(SE_DBG_WAVEFORM);

  m_player = player;

  // init
  m_connection_player_tick = m_player->signal_tick().connect(
      sigc::mem_fun(*this, &WaveformEditor::on_player_tick));
  // Update the view when the keyframes changed
  m_player->signal_message().connect(
      sigc::mem_fun(*this, &WaveformEditor::on_player_message));
}

Player *WaveformEditor::player() {
  if (m_player == NULL) {
    Player *player = SubtitleEditorWindow::get_instance()->get_player();

    if (player != NULL)
      set_player(player);
  }
  return m_player;
}

// Enable the signal tick (Player)
void WaveformEditor::on_map() {
  Gtk::Box::on_map();

  if (m_connection_player_tick) {
    if (m_connection_player_tick.connected())
      m_connection_player_tick.unblock();
  }

  redraw_renderer();
}

// Disable the signal tick (Player)
void WaveformEditor::on_unmap() {
  Gtk::Box::on_unmap();

  if (m_connection_player_tick) {
    if (m_connection_player_tick.connected())
      m_connection_player_tick.block();
  }
}

void WaveformEditor::on_player_tick(long /*current_time*/,
                                    long /*stream_length*/,
                                    double /*current_position*/) {
  if (has_renderer() && player() && has_waveform()) {
    scroll_with_player();
	// only select new subtitle when player position is outside of the current one
	long playerpos = player()->get_position();
	if(!((subtitle_start_and_end_times.first <= playerpos) && (subtitle_start_and_end_times.second >= playerpos))) {
		subtitle_start_and_end_times = select_with_player();
		}
    redraw_renderer();
  }
}

// Return the current time of the player.
long WaveformEditor::get_player_time() {
  if (player()) {
    return player()->get_position();
  }
  return 0;
}

// Set the value of the scale (widget).
void WaveformEditor::set_scale(float value) {
  se_dbg_msg(SE_DBG_WAVEFORM, "scale=%f", value);

  m_sliderScale->set_value(value);
}

// Return the value of the scale (widget).
float WaveformEditor::get_scale() {
  return static_cast<float>(m_sliderScale->get_value());
}

// Set the value of the zoom (widget).
void WaveformEditor::set_zoom(int value) {
  se_dbg_msg(SE_DBG_WAVEFORM, "zoom=%d", value);

  m_sliderZoom->set_value(value);
}

// Return the value of the zoom (widget).
int WaveformEditor::get_zoom() {
  int value = static_cast<int>(m_sliderZoom->get_value());
  if (value < 1)
    return 1;
  return value;
}

// The scroll bar depend on the size of the waveform widget.
// This callback is connected to the signal "configure" of the waveform.
// Every time this size changed, the scrollbar need to be recalculate.
bool WaveformEditor::on_configure_event_waveform(GdkEventConfigure * /*ev*/) {
  se_dbg(SE_DBG_WAVEFORM);

  // init scrollbar
  init_scrollbar();

  redraw_renderer();
  return true;
}

// Return the value of the scrolling (scrollbar)
int WaveformEditor::get_scrolling() {
  return static_cast<int>(m_hscrollbarWaveformRenderer->get_value());
}

// Initialize the scrollbar depending
// on the size of the widget renderer (waveform)
// and the value of the zoom.
void WaveformEditor::init_scrollbar() {
  se_dbg(SE_DBG_WAVEFORM);

  if (!has_renderer())
    return;

  guint width = renderer()->widget()->get_width();

  int zoom = static_cast<int>(m_sliderZoom->get_value());

  double upper = m_hscrollbarWaveformRenderer->get_adjustment()->get_upper();
  double old_value = m_hscrollbarWaveformRenderer->get_value();

  auto adj = m_hscrollbarWaveformRenderer->get_adjustment();

  adj->set_page_size(static_cast<double>(width));
  adj->set_page_increment(width);
  adj->set_step_increment(width);

  m_hscrollbarWaveformRenderer->set_range(0, width * zoom);

  if (upper > 0)
    m_hscrollbarWaveformRenderer->set_value(width * zoom * (old_value / upper));

  se_dbg_msg(SE_DBG_WAVEFORM, "width=%d zoom=%d", width, zoom);
}

// The value of the scrollbar has changed.
// Update the waveform renderer with the new value.
void WaveformEditor::on_scrollbar_value_changed() {
  se_dbg(SE_DBG_WAVEFORM);

  redraw_renderer();
}

// The value of the zoom has changed.
// Call init_scrollbar and updates the config.
// Redraw the waveform.
void WaveformEditor::on_zoom_changed() {
  int value = static_cast<int>(m_sliderZoom->get_value());

  // utility::clamp(value, 1, 1000);

  se_dbg_msg(SE_DBG_WAVEFORM, "zoom=%d", value);

  init_scrollbar();

  if (cfg::get_int("waveform", "zoom") != value)
    cfg::set_int("waveform", "zoom", value);

  redraw_renderer();
}

// The value of the scale has changed.
// Redraw the waveform.
void WaveformEditor::on_scale_changed() {
  int value = static_cast<int>(m_sliderScale->get_value());

  se_dbg_msg(SE_DBG_WAVEFORM, "scale=%d", value);

  if (cfg::get_int("waveform", "scale") != value)
    cfg::set_int("waveform", "scale", value);

  redraw_renderer();
}

// Try to open a waveform file and show or hide the editor.
bool WaveformEditor::open_waveform(const Glib::ustring &uri) {
  se_dbg(SE_DBG_WAVEFORM);

  if (m_waveform) {
    // FIXME
    // clear
  }

  Glib::RefPtr<Waveform> wf = Waveform::create_from_file(uri);

  set_waveform(wf);

  return static_cast<bool>(wf);
}

void WaveformEditor::set_waveform(const Glib::RefPtr<Waveform> &wf) {
  if (wf) {
    se_dbg_msg(SE_DBG_WAVEFORM,
               "uri='%s' video_uri='%s' size='%d' channels='%d' duration='%d'",
               wf->get_uri().c_str(), wf->get_video_uri().c_str(),
               wf->get_size(), wf->get_n_channels(), wf->get_duration());
  } else {
    se_dbg_msg(SE_DBG_WAVEFORM, "the waveform RefPtr is NULL");
  }

  m_waveform = wf;

  // Only show the wf view if it's not already visible
  // Don't show or hide using the wf status
  // bug #22482 : Another regression of gtk3 port
  if (cfg::get_boolean("waveform", "display") == false && wf) {
    cfg::set_boolean("waveform", "display", true);
  }

  if (has_renderer())
    renderer()->set_waveform(wf);
  else
    std::cerr << "You need a WaveformRenderer!!" << std::endl;

  set_child_sensitive(static_cast<bool>(wf) && has_renderer());

  m_signal_waveform_changed.emit();

  init_scrollbar();

  // force to redisplay
  redraw_renderer();
}

// Return the state of waveform. Can be NULL.
bool WaveformEditor::has_waveform() {
  return static_cast<bool>(get_waveform());
}

// Return a pointer to the waveform. Can be NULL.
Glib::RefPtr<Waveform> WaveformEditor::get_waveform() {
  return m_waveform;
}

sigc::signal<void> &WaveformEditor::signal_waveform_changed() {
  return m_signal_waveform_changed;
}

// The editor has a renderer ?
bool WaveformEditor::has_renderer() {
  return renderer() != NULL;
}

// Return the renderer. Can be NULL.
WaveformRenderer *WaveformEditor::renderer() {
  return m_waveformRenderer;
}

// Redisplay the renderer (call renderer->redraw_all)
void WaveformEditor::redraw_renderer() {
  if (has_renderer())
    renderer()->redraw_all();
}

// Return the state of current document.
bool WaveformEditor::has_document() {
  return document() != NULL;
}

// Return a pointer to the current document. Can be NULL.
Document *WaveformEditor::document() {
  return m_document;
}

// This callback is connected at the current document.
// The document has changed, it's needed to redraw the view.
void WaveformEditor::on_document_changed() {
  if ((has_renderer() && has_waveform()) == false)
    return;

  redraw_renderer();
}

// This callback is connected at the current document.
// It's called when the selection of the subtitles has changed.
// The view is centered with the new selection if the option is enabled.
// It's needed to redraw the view.
void WaveformEditor::on_subtitle_selection_changed() {
  if ((has_renderer() && has_waveform() && has_document()) == false)
    return;

  bool player_playing = (player() ? player()->is_playing() : false);

  if (m_cfg_scrolling_with_selection && player_playing == false)
    center_with_selected_subtitle();

  redraw_renderer();
}

// This callback is connected at the current document.
// The time of subtitle has changed, it's need to redraw the view.
void WaveformEditor::on_subtitle_time_changed() {
  if ((has_renderer() && has_waveform()) == false)
    return;

  redraw_renderer();
}

// This callback is connected at the player.
// The keyframes has changed, it's need to redraw the view.
void WaveformEditor::on_player_message(Player::Message msg) {
  if (msg != Player::KEYFRAME_CHANGED)
    return;
  if (!has_renderer())
    return;

  renderer()->keyframes_changed();
  redraw_renderer();
}

void WaveformEditor::scroll_to_position(int position) {
  if (!has_renderer())
    return;

  int start_area = renderer()->get_start_area();
  int end_area = renderer()->get_end_area();

  int margin = 20;

  if ((position - margin) < start_area)
    m_hscrollbarWaveformRenderer->set_value(position - margin);
  else if ((position + margin) > end_area)
    m_hscrollbarWaveformRenderer->set_value(position + margin);
}

// Go at the position on the scrollbar and
// try to place at the center of the view.
void WaveformEditor::scroll_to_position_and_center(int position) {
  if (!has_renderer())
    return;

  int start_area = renderer()->get_start_area();
  int end_area = renderer()->get_end_area();

  double center_area = start_area + (end_area - start_area) * 0.5;

  double diff = position - center_area;

  m_hscrollbarWaveformRenderer->set_value(start_area + diff);
}

// If scrolling with player is enabled,
// scroll with the current time of the player.
void WaveformEditor::scroll_with_player() {
  if (player() && has_renderer() && m_cfg_scrolling_with_player) {
    int position = renderer()->get_pos_by_time(player()->get_position());
    scroll_to_position(position);
  }
}

// If Select With Player is enabled,
// selects current subtitles with the current time of the player.
// Only fires when player position is outside of of the selected subtitled
// FIXME: This is hollowed out version of Find Subtitle by Time plugin, ideally they would share code
std::pair<long, long> WaveformEditor::select_with_player() {
	if (player() && has_document() &&m_cfg_select_with_player) {
    Subtitles subs = document()->subtitles();
    long playerpos = player()->get_position();
	for(Subtitle cursub = subs.get_first(); cursub; cursub = subs.get_next(cursub)) {
	    if((cursub.get_start().totalmsecs <= playerpos) && (cursub.get_end().totalmsecs > playerpos)) {
	        document()->subtitles().select(cursub);
		    document()->emit_signal("subtitle-selection-changed");
			std::pair<long, long> sub_timecodes = {cursub.get_start().totalmsecs, cursub.get_end().totalmsecs};
	        return sub_timecodes;
		}
    }
  }
  return {0, 0};
}
// Try to display the current subtitle at the center of the view.
void WaveformEditor::center_with_selected_subtitle() {
  if (!document())
    return;

  Subtitle subtitle = document()->subtitles().get_first_selected();
  if (!subtitle)
    return;

  int start = renderer()->get_pos_by_time(subtitle.get_start().totalmsecs);
  int end = renderer()->get_pos_by_time(subtitle.get_end().totalmsecs);

  if (start != 0 && end != 0) {
    int middle = start + static_cast<int>((end - start) * 0.5);

    scroll_to_position_and_center(middle);
  }
}

// Increment the zoom
void WaveformEditor::zoom_in() {
  set_zoom(get_zoom() + 1);
}

// Decrement the zoom
void WaveformEditor::zoom_out() {
  set_zoom(get_zoom() - 1);
}

// Décrément completely the zoom
void WaveformEditor::zoom_all() {
  set_zoom(1);
}

// Zooming on the current subtitle.
void WaveformEditor::zoom_selection() {
  zoom_in();

  center_with_selected_subtitle();
}

// Edit the position of the current subtitle.
// Start the recorder command.
bool WaveformEditor::on_button_press_event_renderer(GdkEventButton *ev) {
  se_dbg(SE_DBG_WAVEFORM);

  if (!has_renderer() || !has_waveform())
    return true;

  // the time of the mouse in the area
  SubtitleTime time(renderer()->get_mouse_time((gint)ev->x));

  // If the button 2 is pressed without other keys, we only move to the current
  // position With CONTROL, we move and play
  if (ev->button == 2 && !(ev->state & Gdk::CONTROL_MASK) && player()) {
    player()->seek(time.totalmsecs);

    if (player()->is_playing() == false && (ev->state & Gdk::SHIFT_MASK))
      player()->play();
    return true;
  }

  if (!has_document())
    return true;

  // button 2 with CONTROL select subtitle
  // try to select subtitle
  if (ev->button == 2 && ev->state & Gdk::CONTROL_MASK) {
    Subtitle sub = document()->subtitles().find(time);
    if (sub)
      document()->subtitles().select(sub);

    redraw_renderer();
    return true;
  }

  // now actions with document and subtitle
  if (!has_document())
    return true;

  Subtitle subtitle = document()->subtitles().get_first_selected();
  if (!subtitle)
    return true;

  if (ev->button == 1) {
    document()->start_command(_("Editing position"));
    move_subtitle_start(time, (ev->state & Gdk::SHIFT_MASK),
                        (ev->state & Gdk::CONTROL_MASK));
  } else if (ev->button == 3) {
    document()->start_command(_("Editing position"));
    move_subtitle_end(time, (ev->state & Gdk::SHIFT_MASK),
                      (ev->state & Gdk::CONTROL_MASK));
  }

  renderer()->m_display_time_info = true;

  return false;
}

// Finish the editing of the current subtitle.
// Stop the recorder command.
bool WaveformEditor::on_button_release_event_renderer(GdkEventButton * /*ev*/) {
  se_dbg(SE_DBG_WAVEFORM);

  if (!(has_renderer() && has_document()))
    return true;

  document()->finish_command();

  renderer()->m_display_time_info = false;
  redraw_renderer();

  return true;
}

// Adjust the position of the current subtitle.
bool WaveformEditor::on_motion_notify_event_renderer(GdkEventMotion *ev) {
  se_dbg(SE_DBG_WAVEFORM);

  if (!(has_renderer() && has_document()))
    return true;

  Subtitle subtitle = document()->subtitles().get_first_selected();
  if (!subtitle)
    return true;

  SubtitleTime time = renderer()->get_mouse_time(static_cast<int>(ev->x));

  if ((ev->state & Gdk::BUTTON1_MASK)) {
    move_subtitle_start(time, (ev->state & Gdk::SHIFT_MASK),
                        (ev->state & Gdk::CONTROL_MASK));
  } else if ((ev->state & Gdk::BUTTON3_MASK)) {
    move_subtitle_end(time, (ev->state & Gdk::SHIFT_MASK),
                      (ev->state & Gdk::CONTROL_MASK));
  }

  redraw_renderer();
  return true;
}

// Manage the scrolling like the current position in the view (like scrollbar),
// the scale or the zoom.
bool WaveformEditor::on_scroll_event_renderer(GdkEventScroll *ev) {
  se_dbg(SE_DBG_WAVEFORM);

  if (!(has_waveform() && has_renderer()))
    return true;

  int value = 0;

  if (ev->direction == GDK_SCROLL_UP)
    value = 1;
  else if (ev->direction == GDK_SCROLL_DOWN)
    value = -1;
  else
    return true;

  if (ev->state & Gdk::SHIFT_MASK) {  // Scale
    set_scale(get_scale() + value);
  } else if (ev->state & Gdk::CONTROL_MASK) {  // Zoom
    int center_area =
        renderer()->get_start_area() +
        static_cast<int>(
            (renderer()->get_end_area() - renderer()->get_start_area()) * 0.5);

    long time = renderer()->get_time_by_pos(center_area);

    set_zoom(get_zoom() + value);

    scroll_to_position_and_center(renderer()->get_pos_by_time(time));
  } else {  // Scrolling like scrollbar
    double page_size =
        m_hscrollbarWaveformRenderer->get_adjustment()->get_page_size();
    double delta = pow(page_size, 2.0 / 3.0);
    m_hscrollbarWaveformRenderer->set_value(
        m_hscrollbarWaveformRenderer->get_value() - delta * value);
  }

  return true;
}

void WaveformEditor::set_child_sensitive(bool status) {
  m_hscrollbarWaveformRenderer->set_sensitive(status);
  m_sliderZoom->set_sensitive(status);
  m_sliderScale->set_sensitive(status);
}

// Try to move the beginning of the current subtitle.
// If the option 'respect-timing' is enabled,
// try to respect the timing preferences.
// disable_respect:
// No test is doing if 'disable_respect' is enabled.
// around:
// If is true, the end of the previous subtitle can be moved
// if necessary with respect of timing preferences.
bool WaveformEditor::move_subtitle_start(const SubtitleTime &_time,
                                         bool disable_respect, bool around) {
  if (!(has_renderer() && has_document()))
    return false;

  Subtitle subtitle = document()->subtitles().get_first_selected();

  if (!subtitle)
    return false;

  SubtitleTime min_gap = cfg::get_int("timing", "min-gap-between-subtitles");
  SubtitleTime min_display = cfg::get_int("timing", "min-display");

  SubtitleTime time = _time;

  // clamp [0:]
  if (time.totalmsecs < 0)
    time = SubtitleTime();

  SubtitleTime diff = time - subtitle.get_start();

  // this is the start of the current subtitle
  SubtitleTime subtitle_start = time;

  if (!disable_respect) {
    // Check the min display respect of the current subtitle
    // Clamped if need
    // if(m_cfg_respect_min_display)
    if (m_cfg_respect_timing) {
      SubtitleTime duration = subtitle.get_end() - subtitle_start;
      if (duration < min_display)
        subtitle_start = subtitle.get_end() - min_display;
    }

    // check the timing respect with the previous subtitle
    // Respect just the gap between subtitle
    // the start of the current subtitle is clamped if need
    // if(m_cfg_respect_gab_between_subtitles)
    if (m_cfg_respect_timing) {
      Subtitle previous = document()->subtitles().get_previous(subtitle);
      if (previous) {
        SubtitleTime end_of_previous_sub = previous.get_end();

        // Only if the previous subtitle is really before this one.
        // Avoid to invalidate the subtitle start when the previous is
        // equal to 0:00:000.0 or superior
        bool is_really_previous = previous.get_start() < subtitle.get_start();

        // Have we an overlapping ?
        bool is_on_previous = subtitle_start < end_of_previous_sub + min_gap;

        if (is_on_previous && is_really_previous) {
          // if around is enable
          // try to move the end of the previous subtitle
          if (around) {
            SubtitleTime new_previous_end = subtitle_start - min_gap;

            // try to move the end of the previous
            // check first if after move is respect the min display
            if (new_previous_end - previous.get_start() > min_display) {
              previous.set_end(new_previous_end);
            } else {  // I can, move with gap respect
              new_previous_end = previous.get_start() + min_display;

              previous.set_end(new_previous_end);

              subtitle_start = new_previous_end + min_gap;
            }
          } else {  // around is disable
            // clamp at the end of the previous with gap respect
            subtitle_start = end_of_previous_sub + min_gap;
          }
        }
      }
    }  // m_cfg_respect_timing && gab between subtitles
  }    //! disable_respect

  if (subtitle.get_start() != subtitle_start && subtitle_start.totalmsecs >= 0)
    subtitle.set_start(subtitle_start);

  document()->emit_signal("subtitle-time-changed");
  return true;
}

// Try to move the end of the current subtitle.
// If the option 'respect-timing' is enabled,
// try to respect the timing preferences.
// disable_respect:
// No test is doing if 'disable_respect' is enabled.
// around:
// If is true, the beginning of the next subtitle can be moved
// if necessary with respect of timing preferences.
bool WaveformEditor::move_subtitle_end(const SubtitleTime &_time,
                                       bool disable_respect, bool around) {
  if (!(has_renderer() && has_document()))
    return false;

  Subtitle subtitle = document()->subtitles().get_first_selected();

  if (!subtitle)
    return false;

  SubtitleTime min_gap = cfg::get_int("timing", "min-gap-between-subtitles");
  SubtitleTime min_display = cfg::get_int("timing", "min-display");

  SubtitleTime time = _time;

  // clamp [0:]
  if (time.totalmsecs < 0)
    time = SubtitleTime();

  SubtitleTime diff = time - subtitle.get_end();

  // this is the end of the current subtitle
  SubtitleTime subtitle_end = time;

  if (!disable_respect) {
    // Check the min display respect of the current subtitle
    // Clamped if need
    // if(m_cfg_respect_min_display)
    if (m_cfg_respect_timing) {
      SubtitleTime duration = subtitle_end - subtitle.get_start();
      if (duration < min_display)
        subtitle_end = subtitle.get_start() + min_display;
    }

    // check the timing respect with the next subtitle
    // Respect just the gap between subtitle
    // the end of the current subtitle is clamped if need
    // if(m_cfg_respect_gab_between_subtitles)
    if (m_cfg_respect_timing) {
      Subtitle next = document()->subtitles().get_next(subtitle);
      if (next) {
        SubtitleTime start_of_next_sub = next.get_start();

        // Only if the next subtitle is really after this one.
        // Avoid to invalidate the subtitle end when the next is
        // equal to 0:00:000.0 or inferior
        bool is_really_next = next.get_start() > subtitle.get_start();

        // Have we an overlapping ?
        bool is_on_next = subtitle_end + min_gap > start_of_next_sub;

        if (is_on_next && is_really_next) {
          // if around is enable
          // try to move the start of the next subtitle
          if (around) {
            SubtitleTime new_next_start = subtitle_end + min_gap;

            // try to move the start of the next
            {
              if (next.get_end() - new_next_start > min_display) {
                next.set_start(new_next_start);
              } else {  // I can, move with gap respect
                new_next_start = next.get_end() - min_display;

                next.set_start(new_next_start);

                subtitle_end = new_next_start - min_gap;
              }
            }
          } else {  // around is disable
            // clamp at the start of the next with gap respect
            subtitle_end = start_of_next_sub - min_gap;
          }
        }
      }
    }
  }

  if (subtitle.get_end() != subtitle_end)
    subtitle.set_end(subtitle_end);

  document()->emit_signal("subtitle-time-changed");
  return true;
}

void WaveformEditor::on_config_waveform_changed(const Glib::ustring &key,
                                                const Glib::ustring &value) {
  if (key == "scrolling-with-player") {
    m_cfg_scrolling_with_player = utility::string_to_bool(value);
  } else if (key == "select-with-player") {
    m_cfg_select_with_player = utility::string_to_bool(value);
  } else if (key == "scrolling-with-selection") {
    m_cfg_scrolling_with_selection = utility::string_to_bool(value);
  } else if (key == "respect-timing") {
    m_cfg_respect_timing = utility::string_to_bool(value);
  } else if (key == "display") {
    utility::string_to_bool(value) ? show() : hide();
  } else if (key == "renderer") {
    on_create_renderer();
  }
  // else if(key == "respect-min-display") {
  //   m_cfg_respect_min_display = utility::string_to_bool(value);
  // } else if(key == "respect-gap-between-subtitles") {
  //   m_cfg_respect_gab_between_subtitles = utility::string_to_bool(value);
  // }
}
