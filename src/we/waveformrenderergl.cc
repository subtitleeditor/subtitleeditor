#ifdef ENABLE_GL

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

#include <GL/gl.h>
#include <gtkglmm.h>

#include "document.h"
#include "keyframes.h"
#include "player.h"
#include "subtitleeditorwindow.h"
#include "utility.h"
#include "waveformrenderer.h"

#define FONT_SIZE 256

// OpenGL Waveform renderer
class WaveformRendererGL : public Gtk::GL::DrawingArea,
                           public WaveformRenderer {
 public:
  WaveformRendererGL();

  ~WaveformRendererGL();

  // Return the widget attached to the renderer.
  Gtk::Widget *widget();

  // Create GdkGLConfig (RGB | DEPTH | DOUBLE | STENCIL)
  Glib::RefPtr<Gdk::GL::Config> create_glconfig();

  // Try to create OpenGL font (with support of display list)
  bool create_gl_font(const Glib::ustring &font_desc);

  // Use pango to get the size of the text (pixel)
  int get_text_width(const Glib::ustring &text);

  // Use the Raster Position to display text
  void draw_text(float x, float y, const Glib::ustring &text);

  // Generate font
  void on_realize();

  // Just redisplay with redraw_all
  bool on_configure_event(GdkEventConfigure *ev);

  // Expose widget
  // Clear buffer
  // Draw timeline, channels and markers
  // Swap Buffer
  bool on_expose_event(GdkEventExpose *ev);

  // Display all scene:
  // - timeline (draw_timeline)
  // - waveform (draw_waveform)
  // - subtitle (draw_subtitles)
  // - time info (display_time_info)
  void draw(GdkEventExpose *ev);

  // Draw the left and the right marker of the subtitle selected.
  void draw_marker(const Gdk::Rectangle &rect);

  // Draw subtitles visible
  void draw_subtitles(const Gdk::Rectangle &rect);

  // Draw the text of the subtitles visible
  void draw_subtitles_text(const Gdk::Rectangle &rect);

  // Draw the channel in the area with the lines methods
  void draw_channel_with_line_strip(const Gdk::Rectangle &area, int channel);

  // Draw the channel in the area with the quad methods
  void draw_channel_with_quad_strip(const Gdk::Rectangle &area, int channel);

  // Display all of timeline: Time, seconds
  void draw_timeline(const Gdk::Rectangle &area);

  // Display the time every X seconds ("msec") with "upper" height
  void draw_timeline_msecs(const Gdk::Rectangle &area, long msec, int upper);

  // Display the time text every X seconds (msec)
  void draw_timeline_time(const Gdk::Rectangle &area, long msec);

  // Display the time of the mouse
  // and the duration of the selected subtitle
  void display_time_info(const Gdk::Rectangle &area);

  // Draw the keyframes position.
  void draw_keyframes(const Gdk::Rectangle &area);

  // Delete the OpenGL Display List (Waveform)
  void delete_display_lists();

  // The Waveform used a display list for optimize the render.
  // If the OpenGL display list (waveform) is not yet create:
  // - Create the display list and draw the waveform inside.
  // Call the display list for drawing the waveform.
  void draw_waveform(const Gdk::Rectangle &rect);

  // The waveform is changed.
  // Need to force to redisplay the waveform.
  // Delete the display list
  void waveform_changed();

  // The keyframe is changed.
  // Need to redisplay the waveform.
  void keyframes_changed();

  // Call queue_draw
  void redraw_all();

  // Delete display list and redraw
  void force_redraw_all();

 protected:
  Pango::FontDescription m_fontDescription;
  GLuint m_fontListBase;
  int m_fontHeight;

  // waveform
  Gdk::Rectangle m_displayListRect;
  GLuint m_displayList;
  GLsizei m_displayListSize;
};

// Constructor
WaveformRendererGL::WaveformRendererGL()
    : WaveformRenderer(),
      m_fontListBase(0),
      m_fontHeight(0),
      m_displayList(0),
      m_displayListSize(0) {
  Glib::RefPtr<Gdk::GL::Config> glconfig = create_glconfig();
  if (glconfig)
    set_gl_capability(glconfig);
}

WaveformRendererGL::~WaveformRendererGL() {
  // Display lists are deleted with the opengl context
}

// Return the widget attached to the renderer.
Gtk::Widget *WaveformRendererGL::widget() {
  return this;
}

// Create GdkGLConfig (RGB | DEPTH | DOUBLE | STENCIL)
Glib::RefPtr<Gdk::GL::Config> WaveformRendererGL::create_glconfig() {
  Glib::RefPtr<Gdk::GL::Config> glconfig;

  glconfig = Gdk::GL::Config::create(
      Gdk::GL::MODE_RGB | Gdk::GL::MODE_DEPTH | Gdk::GL::MODE_DOUBLE |
      Gdk::GL::MODE_STENCIL /* | Gdk::GL::MODE_MULTISAMPLE*/);
  if (!glconfig) {
    std::cerr << "Gtk::GL::Config Failed To create double-buffered visual."
              << std::endl;

    glconfig = Gdk::GL::Config::create(Gdk::GL::MODE_RGB | Gdk::GL::MODE_DEPTH |
                                       Gdk::GL::MODE_STENCIL);
    if (!glconfig) {
      std::cerr << "Gtk::GL::Config Failed To create single-buffered visual."
                << std::endl;
      std::cerr << "Used another Waveform Renderer" << std::endl;

      return Glib::RefPtr<Gdk::GL::Config>(NULL);
    }
  }

  return glconfig;
}

// Try to create OpenGL font (with display list)
bool WaveformRendererGL::create_gl_font(const Glib::ustring &font_desc_str) {
  if (m_fontListBase > 0) {
    // destroy font list
    glDeleteLists(m_fontListBase, FONT_SIZE);
    m_fontListBase = 0;
    m_fontHeight = 0;
    m_fontDescription = Pango::FontDescription();
  }

  // OpenGL list
  m_fontListBase = glGenLists(FONT_SIZE);

  Pango::FontDescription font_desc(font_desc_str);

  Glib::RefPtr<Pango::Font> font =
      Gdk::GL::Font::use_pango_font(font_desc, 0, FONT_SIZE, m_fontListBase);
  if (!font) {
    std::cerr << "*** Can't load font :" << font_desc_str << std::endl;

    glDeleteLists(m_fontListBase, FONT_SIZE);
    return false;
  }

  Pango::FontMetrics font_metrics = font->get_metrics();

  m_fontHeight = font_metrics.get_ascent() + font_metrics.get_descent();
  m_fontHeight = PANGO_PIXELS(m_fontHeight);

  m_fontDescription = font_desc;

  return true;
}

// Use pango to get the size of the text (pixel)
int WaveformRendererGL::get_text_width(const Glib::ustring &text) {
  Glib::RefPtr<Pango::Layout> layout = create_pango_layout(text);
  if (layout) {
    int text_width = 0, text_height = 0;

    layout->set_font_description(m_fontDescription);
    layout->get_pixel_size(text_width, text_height);

    return text_width;
  }
  return 0;
}

// Use the Raster Position to display text
void WaveformRendererGL::draw_text(float x, float y,
                                   const Glib::ustring &text) {
  glRasterPos2f(x, y);

  glListBase(m_fontListBase);
  glCallLists(text.length(), GL_UNSIGNED_BYTE, text.c_str());
}

// Generate OpenGL font
void WaveformRendererGL::on_realize() {
  Gtk::DrawingArea::on_realize();

  if (!is_gl_capable())
    return;

  Glib::RefPtr<Gdk::GL::Window> gl = get_gl_window();

  if (!gl->gl_begin(get_gl_context()))
    return;

  if (!create_gl_font("courier bold 10")) {
    create_gl_font(get_pango_context()->get_font_description().to_string());
  }

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_MULTISAMPLE);

  gl->gl_end();
}

// Just redisplay with redraw_all
bool WaveformRendererGL::on_configure_event(GdkEventConfigure *ev) {
  Gtk::DrawingArea::on_configure_event(ev);

  redraw_all();

  // return false IMPORTANT!!!
  return false;
}

// Expose widget
// Clear buffer
// Draw timeline, channels and markers
// Swap Buffer
bool WaveformRendererGL::on_expose_event(GdkEventExpose *ev) {
  static Glib::Timer m_timer;

  // check minimum size
  if (get_width() < 20 || get_height() < 10)
    return false;

  if (se_dbg_check_flags(SE_DBG_WAVEFORM))
    m_timer.start();

  // If window system doesn't support OpenGL
  // display in the area a message
  if (!is_gl_capable()) {
    Glib::ustring error_msg(
        _("Window system doesn't support OpenGL.\n"
          "Please try with another renderer."));

    Glib::RefPtr<Pango::Layout> layout = create_pango_layout(error_msg);

    layout->set_alignment(Pango::ALIGN_CENTER);

    Pango::FontDescription desc = get_pango_context()->get_font_description();
    desc.set_weight(Pango::WEIGHT_BOLD);
    layout->set_font_description(desc);

    // display the message at the center
    int x, y, w, h;
    layout->get_pixel_size(w, h);

    x = (get_width() - w) / 2;
    y = (get_height() - h) / 2;

    get_window()->draw_layout(get_style()->get_black_gc(), x, y, layout);

    return true;
  }

  Glib::RefPtr<Gdk::GL::Window> gl = get_gl_window();

  if (!gl->gl_begin(get_gl_context()))
    return false;

  glClearColor(m_color_background[0], m_color_background[1],
               m_color_background[2], m_color_background[3]);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  glClearStencil(0);
  glStencilMask(1);

  // Reshape
  int width = get_width();
  int height = get_height();

  glViewport(0, 0, width, height);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glOrtho(0, width, 0, height, -10, 10);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Display Scene
  if (m_waveform && is_sensitive()) {
    draw(ev);

    if (se_dbg_check_flags(SE_DBG_WAVEFORM)) {
      double seconds = m_timer.elapsed();
      m_timer.reset();

      Glib::ustring fps =
          build_message("%d frames in %f seconds = %.3f FPS", 1, seconds,
                        static_cast<float>(1 / seconds));

      glColor4fv(m_color_text);
      draw_text(10, 10, fps);
    }
  }

  // Swap Buffer
  if (gl->is_double_buffered())
    gl->swap_buffers();
  else
    glFlush();

  gl->gl_end();
  return true;
}

// Display all scene:
// - timeline (draw_timeline)
// - waveform (draw_waveform)
// - subtitle (draw_subtitles)
// - time info (display_time_info)
void WaveformRendererGL::draw(GdkEventExpose *ev) {
  Gdk::Rectangle timeline_area(0, 0, get_width(), 30);
  Gdk::Rectangle waveform_area(0, 0, get_width(), get_height() - 30);

  // waveform
  glPushMatrix();
  draw_waveform(waveform_area);
  glPopMatrix();

  if (document()) {
    glEnable(GL_BLEND);

    draw_subtitles(waveform_area);

    glDisable(GL_BLEND);

    // draw_subtitles_text(waveform_area);

    draw_marker(waveform_area);
  }

  // time line
  glPushMatrix();
  glTranslatef(-get_start_area(), get_height() - timeline_area.get_height(), 0);
  draw_timeline(timeline_area);
  glPopMatrix();

  // FIXME: test it
  // keyframes
  // draw_keyframes(waveform_area);

  // player position
  {
    glColor4fv(m_color_player_position);

    int player_position = get_pos_by_time(player_time());

    glPushMatrix();
    glTranslatef(-get_start_area(), 0, 0);
    glBegin(GL_LINES);
    glVertex2f(player_position, 0);
    glVertex2f(player_position, waveform_area.get_height());
    glEnd();
    glPopMatrix();
  }

  if (document() && m_display_time_info)
    display_time_info(waveform_area);
}

// Draw the channel in the area with the lines methods
void WaveformRendererGL::draw_channel_with_line_strip(
    const Gdk::Rectangle &area, int channel) {
  if (!m_waveform)
    return;

  std::vector<double> &peaks = m_waveform->m_channels[channel];

  float skip = static_cast<float>(area.get_width()) / peaks.size();

  float px = 0;

  glColor4fv(m_color_wave_fill);

  glBegin(GL_LINE_STRIP);

  for (const auto it = peaks.begin(); it != peaks.end(); ++it, px += skip)
    glVertex2d(px, *it);

  glEnd();
}

// Draw the channel in the area with the lines methods
void WaveformRendererGL::draw_channel_with_quad_strip(
    const Gdk::Rectangle &area, int channel) {
  if (!m_waveform)
    return;

  std::vector<double> &peaks = m_waveform->m_channels[channel];

  float skip = static_cast<float>(area.get_width()) / peaks.size();

  float px = 0;

  glColor4fv(m_color_wave);

  glBegin(GL_QUAD_STRIP);
  for (const auto it it = peaks.begin(); it != peaks.end(); ++it, px += skip) {
    glVertex2d(px, 0);
    glVertex2d(px, *it);
  }
  glEnd();
}

// Delete the OpenGL Display List (Waveform)
void WaveformRendererGL::delete_display_lists() {
  if (m_displayListSize > 0) {
    glDeleteLists(m_displayList, m_displayListSize);

    m_displayList = m_displayListSize = 0;
  }
}

// The Waveform used a display list for optimize the render.
// If the OpenGL display list (waveform) is not yet create:
// - Create the display list and draw the waveform inside.
// Call the display list for drawing the waveform.
void WaveformRendererGL::draw_waveform(const Gdk::Rectangle &rect) {
  if (!m_waveform)
    return;

  unsigned int n_channels = m_waveform->get_n_channels();

  int h = rect.get_height() / n_channels;

  if (m_displayListSize == 0) {
    // display to rectangle 10x10
    // after is scale to the widget area (size + zoom)
    Gdk::Rectangle area(0, 0, 10, 10);
    m_displayListRect = area;

    m_displayListSize = n_channels;

    m_displayList = glGenLists(m_displayListSize);
    for (unsigned int i = 0; i < n_channels; ++i) {
      glNewList(m_displayList + i, GL_COMPILE);

      draw_channel_with_quad_strip(area, i);
      draw_channel_with_line_strip(area, i);

      glEndList();
    }
  }

  glEnable(GL_SCISSOR_TEST);
  glEnable(GL_BLEND);

  float d = m_displayListRect.get_width();
  float r = rect.get_width();

  float s = r / d;

  for (unsigned int i = 0; i < n_channels; ++i) {
    // clamp in waveform area
    glScissor(0, h * i, rect.get_width(), h);

    // position to channel area
    glPushMatrix();
    glTranslatef(-get_start_area(), h * i, 0);
    // apply scale and zoom
    glScalef(s * zoom(), scale() * rect.get_height(), 1);
    // display the channel
    glCallList(m_displayList + i);

    glPopMatrix();
  }
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_BLEND);
}

// Display all of timeline: Time, seconds
void WaveformRendererGL::draw_timeline(const Gdk::Rectangle &area) {
  long start = 0;
  long end = get_width() * zoom();

  // horizontal line
  glColor4fv(m_color_text);

  glBegin(GL_LINES);
  glVertex2f(start, 0);
  glVertex2f(end, 0);
  glEnd();

  // seconds
  long sec_1 = SubtitleTime(0, 0, 1, 0).totalmsecs;
  long sec_5 = SubtitleTime(0, 0, 5, 0).totalmsecs;
  long sec_10 = SubtitleTime(0, 0, 10, 0).totalmsecs;

  if (get_pos_by_time(sec_1) <= 0)
    return;

  int text_width = get_text_width("0:00:00");

  float margin = text_width + text_width * 0.5;
  while (get_pos_by_time(sec_1) < margin) {
    // for a sufficiently long duration sec_* will overflow before
    // the loop terminates. check the largest of them.
    if (sec_10 > (LONG_MAX / 2))
      break;
    sec_1 *= 2;
    sec_5 *= 2;
    sec_10 *= 2;
  }

  draw_timeline_msecs(area, sec_1, 3);
  draw_timeline_msecs(area, sec_5, 6);
  draw_timeline_msecs(area, sec_10, 10);

  draw_timeline_time(area, sec_1);
}

// Display the time every X seconds ("msec") with "upper" height
void WaveformRendererGL::draw_timeline_msecs(const Gdk::Rectangle &area,
                                             long msec, int upper) {
  long start = 0;
  long end = get_time_by_pos(get_width() * zoom());

  glBegin(GL_LINES);
  for (long t = start; t < end; t += msec) {
    int x = get_pos_by_time(t);

    glVertex2f(x, 0);
    glVertex2f(x, upper);
  }
  glEnd();
}

// Display the time text every X seconds (msec)
void WaveformRendererGL::draw_timeline_time(const Gdk::Rectangle &area,
                                            long msec) {
  int height = area.get_height();

  long start = 0;
  long end = get_time_by_pos(get_width() * zoom());

  // make the list for text
  glListBase(m_fontListBase);

  Glib::ustring text = "0:00:00";

  // center the text
  float middle = get_text_width(text) * 0.5;

  glPushMatrix();
  glTranslatef(-middle, height - 15, 0);

  for (long t = start; t < end; t += msec) {
    int x = get_pos_by_time(t);

    text = SubtitleTime(t).str().substr(0, 7);

    glRasterPos2f(x, 0);
    glCallLists(text.length(), GL_UNSIGNED_BYTE, text.c_str());
  }
  glPopMatrix();
}

// Display the time of the mouse
// and the duration of the selected subtitle
void WaveformRendererGL::display_time_info(const Gdk::Rectangle &area) {
  int text_width = get_text_width(SubtitleTime::null());

  int xpos = 0, ypos = 0;
  Gdk::ModifierType mod;

  get_window()->get_pointer(xpos, ypos, mod);
  // opengl
  ypos = area.get_height() - ypos;

  int xpos_area = get_mouse_coords(xpos);

  // display the time of the mouse in the area
  Glib::ustring time = SubtitleTime(get_time_by_pos(xpos_area)).str();

  glColor4fv(m_color_text);
  draw_text(xpos - text_width * 0.5, ypos, time);

  // display the duration of the current subtitle
  Subtitle selected = document()->subtitles().get_first_selected();
  if (selected) {
    SubtitleTime start = selected.get_start();
    SubtitleTime duration = selected.get_duration();

    int sub_center =
        get_pos_by_time(start.totalmsecs + duration.totalmsecs / 2);

    glColor4fv(m_color_text);
    draw_text(sub_center - get_start_area() - text_width * 0.5,
              ypos - m_fontHeight, duration.str());
  }
}

void WaveformRendererGL::draw_keyframes(const Gdk::Rectangle &rect) {
  Player *player = SubtitleEditorWindow::get_instance()->get_player();
  if (player == NULL)
    return;

  Glib::RefPtr<KeyFrames> keyframes = player->get_keyframes();
  if (!keyframes)
    return;

  int height = rect.get_height();

  glColor4fv(m_color_keyframe);

  long start_clip = get_time_by_pos(get_start_area());
  long end_clip = get_time_by_pos(get_end_area());

  glBegin(GL_LINES);
  for (KeyFrames::const_iterator it = keyframes->begin();
       it != keyframes->end(); ++it) {
    // display only if it's in the area
    if (*it < start_clip && *it < end_clip)
      continue;
    if (*it > end_clip)
      break;  // the next keyframes are out of the area

    long pos = get_pos_by_time(*it);
    glVertex2f(pos, 0);
    glVertex2f(pos, height);
  }
  glEnd();
}

// Draw the left and the right marker of the subtitle selected.
void WaveformRendererGL::draw_marker(const Gdk::Rectangle &rect) {
  Subtitle selected = document()->subtitles().get_first_selected();
  if (!selected)
    return;

  int height = rect.get_height();

  int start =
      get_pos_by_time(selected.get_start().totalmsecs) - get_start_area();
  int end = get_pos_by_time(selected.get_end().totalmsecs) - get_start_area();

  glBegin(GL_LINES);
  // start line
  glColor3f(1, 0, 0);
  glVertex2f(start, 0);
  glVertex2f(start, height);
  // end line
  glColor3f(1, .6, 0);
  glVertex2f(end, 0);
  glVertex2f(end, height);
  glEnd();

  // triangle size (top and bottom)
  int size = 10;

  glBegin(GL_TRIANGLES);

  glColor3f(1, 0, 0);
  // start + bottom
  glVertex2f(start, 0);
  glVertex2f(start, size);
  glVertex2f(start + size, 0);
  // start + top
  glVertex2f(start, height);
  glVertex2f(start + size, height);
  glVertex2f(start, height - size);

  glColor3f(1, .6, 0);
  // end + bottom
  glVertex2f(end, 0);
  glVertex2f(end - size, 0);
  glVertex2f(end, size);
  // end + top
  glVertex2f(end, height);
  glVertex2f(end, height - size);
  glVertex2f(end - size, height);
  glEnd();
}

// Draw subtitles visible
void WaveformRendererGL::draw_subtitles(const Gdk::Rectangle &rect) {
  if (!document())
    return;

  SubtitleTime start_clip(get_time_by_pos(get_start_area()));
  SubtitleTime end_clip(get_time_by_pos(get_end_area()));

  int height = rect.get_height();

  Subtitles subs = document()->subtitles();
  Subtitle selected = subs.get_first_selected();

  glPushMatrix();
  glTranslatef(-get_start_area(), 0, 0);

  if (selected) {
    for (Subtitle sub = subs.get_first(); sub; ++sub) {
      SubtitleTime start = sub.get_start();
      SubtitleTime end = sub.get_end();

      if (start < start_clip && end < start_clip)
        continue;
      if (start > end_clip && end > end_clip)
        break;

      int s = get_pos_by_time(start.totalmsecs);
      int e = get_pos_by_time(end.totalmsecs);

      if (s > e)
        glColor4fv(m_color_subtitle_invalid);
      else if (selected == sub)
        glColor4fv(m_color_subtitle_selected);
      else
        glColor4fv(m_color_subtitle);

      glRectf(s, 0, e, height);
    }
  } else {
    for (Subtitle sub = subs.get_first(); sub; ++sub) {
      SubtitleTime start = sub.get_start();
      SubtitleTime end = sub.get_end();

      if (start < start_clip && end < start_clip)
        continue;
      if (start > end_clip && end > end_clip)
        break;

      int s = get_pos_by_time(start.totalmsecs);
      int e = get_pos_by_time(end.totalmsecs);

      if (s > e)
        glColor4fv(m_color_subtitle_invalid);
      else
        glColor4fv(m_color_subtitle);

      glRectf(s, 0, e, height);
    }
  }
  glPopMatrix();
}

// Draw the text of the subtitles visible
void WaveformRendererGL::draw_subtitles_text(const Gdk::Rectangle &rect) {
  if (!document())
    return;

  SubtitleTime start_clip(get_time_by_pos(get_start_area()));
  SubtitleTime end_clip(get_time_by_pos(get_end_area()));

  float height = rect.get_height() - m_fontHeight * 2;

  Subtitles subs = document()->subtitles();
  Subtitle selected = subs.get_first_selected();

  glPushMatrix();
  glTranslatef(-get_start_area(), 0, 0);

  glColor4fv(m_color_text);
  glListBase(m_fontListBase);

  for (Subtitle sub = subs.get_first(); sub; ++sub) {
    SubtitleTime start = sub.get_start();
    SubtitleTime end = sub.get_end();

    if (start < start_clip && end < start_clip)
      continue;
    if (start > end_clip && end > end_clip)
      break;

    int s = get_pos_by_time(start.totalmsecs);
    // int e = get_pos_by_time(end.totalmsecs);

    glRasterPos2f(s, height);

    std::string text = sub.get_text();
    // glRectf(s, 0, e, height);
    glCallLists(text.length(), GL_UNSIGNED_BYTE, text.c_str());
  }

  glPopMatrix();
}

// The waveform is changed.
// Need to force to redisplay the waveform.
// Delete the display list
void WaveformRendererGL::waveform_changed() {
  delete_display_lists();

  queue_draw();
}

void WaveformRendererGL::keyframes_changed() {
  queue_draw();
}

// Call queue_draw
void WaveformRendererGL::redraw_all() {
  queue_draw();
}

// Delete display list and redraw
void WaveformRendererGL::force_redraw_all() {
  delete_display_lists();

  queue_draw();
}

// HACK!
WaveformRenderer *create_waveform_renderer_gl() {
  return manage(new WaveformRendererGL());
}

#endif  // ENABLE_GL
