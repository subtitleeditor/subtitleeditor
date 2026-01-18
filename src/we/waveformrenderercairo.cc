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

#include "document.h"
#include "keyframes.h"
#include "player.h"
#include "subtitleeditorwindow.h"
#include "utility.h"
#include "waveformrenderer.h"

#define TRIANGLE_SIZE 10

// Cairo Waveform renderer
class WaveformRendererCairo : public Gtk::DrawingArea, public WaveformRenderer {
  public:
   WaveformRendererCairo();

   ~WaveformRendererCairo();

   // Return the widget attached to the renderer.
   Gtk::Widget* widget();

   // Set the current color at the context.
   void set_color(const Cairo::RefPtr<Cairo::Context>& cr, float color[4]);

   // The waveform is changed.
   // Need to force to redisplay the waveform (m_wf_surface)
   void waveform_changed();

   // The keyframe is changed.
   // Need to redisplay the waveform.
   void keyframes_changed();

   // Call queue_draw
   void redraw_all();

   // Delete the surface and redraw
   void force_redraw_all();

   bool on_configure_event(GdkEventConfigure* ev);

   // Display all scene:
   // - timeline (draw_timeline)
   // - waveform (draw_waveform)
   // - subtitle (draw_subtitles)
   // - time info (display_time_info)
   bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);

   // Display all of timeline: Time, seconds
   void draw_timeline(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::Rectangle& area);

   // Display the time every X seconds ("msec") with "upper" height
   void draw_timeline_msecs(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::Rectangle& area, long msec, int upper);

   // Display the time text every X seconds (msec)
   void draw_timeline_time(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::Rectangle& area, long msec);

   // Draw the waveform by the call of draw_channel.
   void draw_waveform(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::Rectangle& area);

   void draw_channel(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::Rectangle& area, unsigned int channel);

   // Display the text of the subtitle.
   // start:
   // position of the start in the area : get_pos_by_time(subtitle.get_start)
   // end:
   // position of the end in the area : get_pos_by_time(subtitle.get_end)
   void draw_subtitle_text(const Cairo::RefPtr<Cairo::Context>& cr, const Subtitle& sub, int start, int end);

   // Draw subtitles visible
   void draw_subtitles(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::Rectangle& area);

   // Draw the left and the right marker of the subtitle selected.
   void draw_marker(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::Rectangle& area);

   // Draw the current position of the player.
   void draw_player_position(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::Rectangle& area);

   // Display the time of the mouse
   // and the duration of the selected subtitle
   void display_time_info(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::Rectangle& area);

   void draw_keyframes(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::Rectangle& area);

  protected:
   Cairo::RefPtr<Cairo::Surface> m_wf_surface;
   Glib::RefPtr<Pango::Layout> m_layout_text;
};

WaveformRendererCairo::WaveformRendererCairo() : WaveformRenderer() {
   se_dbg(SE_DBG_WAVEFORM);
}

WaveformRendererCairo::~WaveformRendererCairo() {
   se_dbg(SE_DBG_WAVEFORM);
}

// Return the widget attached to the renderer.
Gtk::Widget* WaveformRendererCairo::widget() {
   se_dbg(SE_DBG_WAVEFORM);

   return this;
}

// Set the current color at the context.
void WaveformRendererCairo::set_color(const Cairo::RefPtr<Cairo::Context>& cr, float color[4]) {
   se_dbg(SE_DBG_WAVEFORM);

   cr->set_source_rgba(color[0], color[1], color[2], color[3]);
}

// The waveform is changed.
// Need to force to redisplay the waveform (m_wf_surface)
void WaveformRendererCairo::waveform_changed() {
   se_dbg(SE_DBG_WAVEFORM);

   if (m_wf_surface)
      m_wf_surface.clear();
   queue_draw();
}

void WaveformRendererCairo::keyframes_changed() {
   se_dbg(SE_DBG_WAVEFORM);

   queue_draw();
}

// Call queue_draw
void WaveformRendererCairo::redraw_all() {
   se_dbg(SE_DBG_WAVEFORM);

   queue_draw();
}

// Delete the surface and redraw
void WaveformRendererCairo::force_redraw_all() {
   se_dbg(SE_DBG_WAVEFORM);

   if (m_wf_surface)
      m_wf_surface.clear();
   queue_draw();
}

bool WaveformRendererCairo::on_configure_event(GdkEventConfigure* /*ev*/) {
   se_dbg(SE_DBG_WAVEFORM);

   if (m_wf_surface)
      m_wf_surface.clear();
   queue_draw();

   // return false IMPORTANT!!!
   return false;
}

// Display all scene:
// - timeline (draw_timeline)
// - waveform (draw_waveform)
// - subtitle (draw_subtitles)
// - time info (display_time_info)
bool WaveformRendererCairo::on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
   se_dbg(SE_DBG_WAVEFORM);

   static Glib::Timer m_timer;

   // check minimum size
   if (get_width() < 20 || get_height() < 10)
      return false;

   if (se_dbg_check_flags(SE_DBG_WAVEFORM))
      m_timer.start();

   // background
   set_color(cr, m_color_background);
   cr->rectangle(0, 0, get_width(), get_height());
   cr->fill();

   if (m_waveform) {
      Gdk::Rectangle warea(0, 0, get_width(), get_height() - 30);

      // check
      {
         static int old_start_area = get_start_area();
         static double old_zoom = zoom();
         static double old_scale = scale();

         if (old_zoom != zoom() || old_scale != scale() || old_start_area != get_start_area()) {
            m_wf_surface.clear();
            old_start_area = get_start_area();
            old_zoom = zoom();
            old_scale = scale();
         }
      }

      if (!m_wf_surface) {
         m_wf_surface = Cairo::Surface::create(cr->get_target(), Cairo::CONTENT_COLOR_ALPHA, get_width(), get_height());

         Cairo::RefPtr<Cairo::Context> wf_cr = Cairo::Context::create(m_wf_surface);

         draw_waveform(wf_cr, warea);
      }

      if (m_wf_surface) {
         cr->save();
         cr->translate(0, 30);
         cr->set_source(m_wf_surface, 0, 0);
         cr->paint();
         cr->restore();
      }

      cr->save();
      cr->translate(-get_start_area(), 30);

      draw_keyframes(cr, warea);

      if (document()) {
         draw_subtitles(cr, warea);
         draw_marker(cr, warea);
      }

      draw_player_position(cr, warea);

      cr->restore();

      draw_timeline(cr, Gdk::Rectangle(0, 0, get_width(), 30));

      if (m_display_time_info)
         display_time_info(cr, warea);
   }  // has_waveform

   if (se_dbg_check_flags(SE_DBG_WAVEFORM)) {
      double seconds = m_timer.elapsed();

      Glib::ustring fps = build_message("%d frames in %f seconds = %.3f FPS", 1 /*frame*/, seconds, static_cast<float>(1 /*frame*/ / seconds));

      set_color(cr, m_color_text);
      cr->move_to(10, get_height() - 10);
      cr->show_text(fps);
      m_timer.reset();
   }
   return true;
}

// Display all of timeline: Time, seconds
void WaveformRendererCairo::draw_timeline(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::Rectangle& area) {
   se_dbg(SE_DBG_WAVEFORM);

   // timeline
   int width = area.get_width();
   int height = area.get_height();

   // horizontal line
   set_color(cr, m_color_text);
   cr->move_to(0, height);
   cr->line_to(width, height);
   cr->stroke();

   // seconds
   long sec_1 = SubtitleTime(0, 0, 1, 0).totalmsecs;
   long sec_5 = SubtitleTime(0, 0, 5, 0).totalmsecs;
   long sec_10 = SubtitleTime(0, 0, 10, 0).totalmsecs;

   if (get_pos_by_time(sec_1) <= 0)
      return;

   Cairo::TextExtents extents;
   cr->get_text_extents("0:00:00", extents);

   float margin = static_cast<float>(extents.width + extents.width * 0.5);
   while (get_pos_by_time(sec_1) < margin) {
      // for a sufficiently long duration sec_* will overflow before
      // the loop terminates. check the largest of them.
      if (sec_10 > (LONG_MAX / 2))
         break;
      sec_1 *= 2;
      sec_5 *= 2;
      sec_10 *= 2;
   }

   draw_timeline_msecs(cr, area, sec_1, 3);
   draw_timeline_msecs(cr, area, sec_5, 6);
   draw_timeline_msecs(cr, area, sec_10, 10);

   draw_timeline_time(cr, area, sec_1);
}

// Display the time every X seconds ("msec") with "upper" height
void WaveformRendererCairo::draw_timeline_msecs(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::Rectangle& area, long msec, int upper) {
   se_dbg(SE_DBG_WAVEFORM);

   int height = area.get_height();

   int start_area = get_start_area();

   long start = get_time_by_pos(start_area);
   long end = get_time_by_pos(get_end_area());

   long diff = start % msec;

   start -= diff;

   for (long t = start; t < end; t += msec) {
      int x = get_pos_by_time(t) - start_area;

      cr->move_to(x, height);
      cr->line_to(x, height - upper);
   }
   cr->stroke();
}

// Display the time text every X seconds (msec)
void WaveformRendererCairo::draw_timeline_time(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::Rectangle& /*area*/, long msec) {
   se_dbg(SE_DBG_WAVEFORM);

   int start_area = get_start_area();

   long start = get_time_by_pos(start_area);
   long end = get_time_by_pos(get_end_area());

   long diff = start % msec;

   start -= diff;

   // font
   cr->set_font_size(13);

   Cairo::TextExtents extents;
   cr->get_text_extents("0:00:00", extents);

   double height = extents.height + 5;

   double center = extents.width * 0.5;

   for (long t = start; t < end; t += msec) {
      int x = get_pos_by_time(t) - start_area;

      cr->move_to(x - center, height);

      cr->show_text(SubtitleTime(t).str().substr(0, 7));
   }
   cr->stroke();
}

// Draw the waveform by the call of draw_channel.
void WaveformRendererCairo::draw_waveform(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::Rectangle& area) {
   se_dbg(SE_DBG_WAVEFORM);

   if (!m_waveform)
      return;

   unsigned int n_channels = m_waveform->get_n_channels();

   int ch_height = area.get_height() / n_channels;

   for (unsigned int i = 0; i < n_channels; ++i) {
      cr->save();
      cr->translate(0, i * ch_height);
      draw_channel(cr, Gdk::Rectangle(0, 0, area.get_width(), ch_height), i);
      cr->restore();
   }
}

void WaveformRendererCairo::draw_channel(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::Rectangle& area, unsigned int channel) {
   se_dbg(SE_DBG_WAVEFORM);

   if (!m_waveform)
      return;

   std::vector<double>& peaks = m_waveform->m_channels[channel];

   set_color(cr, m_color_wave);

   int bottom = area.get_height();

   double scale_value = scale() * area.get_height();

   int width = get_width();

   se_dbg_msg(SE_DBG_WAVEFORM, "init drawing values");

   int skip = 4;
   int z = zoom();

   double begin = peaks.size() * (static_cast<double>(get_start_area()) / (width * z));
   double move = peaks.size() * (static_cast<double>(skip) / (width * z));
   int length = width;
   int peaks_size = peaks.size();

   double x = begin;

   se_dbg_msg(SE_DBG_WAVEFORM, "begin %f  move %f  length %d  peaks_size %d", begin, move, length, peaks_size);

   se_dbg_msg(SE_DBG_WAVEFORM, "start drawing peaks");

   cr->line_to(0, bottom);
   for (int t = 0; t < length; t += skip, x += move) {
      int px = static_cast<int>(x);
      if (px > peaks_size)
         break;
      double peakOnScreen = peaks[px] * scale_value;

      peakOnScreen = CLAMP(peakOnScreen, 0, bottom);

      cr->line_to(t, bottom - peakOnScreen);
   }
   cr->line_to(length, bottom);
   cr->fill();

   se_dbg_msg(SE_DBG_WAVEFORM, "end of drawing peaks");
}

// Display the text of the subtitle.
// start:
// position of the start in the area : get_pos_by_time(subtitle.get_start)
// end:
// position of the end in the area : get_pos_by_time(subtitle.get_end)
void WaveformRendererCairo::draw_subtitle_text(const Cairo::RefPtr<Cairo::Context>& cr, const Subtitle& sub, int start, int end) {
   se_dbg(SE_DBG_WAVEFORM);

   cr->save();

   cr->rectangle(start, 0, end - start, get_height());
   cr->clip();

   set_color(cr, m_color_text);

   cr->move_to(start, TRIANGLE_SIZE * 2);

   if (!m_layout_text)
      m_layout_text = Pango::Layout::create(cr);

   m_layout_text->set_text(sub.get_text());
   m_layout_text->update_from_cairo_context(cr);
   m_layout_text->add_to_cairo_context(cr);

   cr->fill();

   cr->restore();
}

// Draw subtitles visible
void WaveformRendererCairo::draw_subtitles(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::Rectangle& area) {
   se_dbg(SE_DBG_WAVEFORM);

   if (!document())
      return;

   int h = area.get_height();

   SubtitleTime start_clip(get_time_by_pos(get_start_area()));
   SubtitleTime end_clip(get_time_by_pos(get_end_area()));

   Subtitles subs = document()->subtitles();
   Subtitle selected = subs.get_first_selected();

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

         if (s > e) {
            set_color(cr, m_color_subtitle_invalid);
         } else if (selected == sub) {
            set_color(cr, m_color_subtitle_selected);
         } else {
            set_color(cr, m_color_subtitle);
         }

         cr->rectangle(s, 0, e - s, h);
         cr->fill();

         if (m_display_subtitle_text)
            draw_subtitle_text(cr, sub, s, e);
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
            set_color(cr, m_color_subtitle_invalid);
         else
            set_color(cr, m_color_subtitle);

         cr->rectangle(s, 0, e - s, h);
         cr->fill();

         if (m_display_subtitle_text)
            draw_subtitle_text(cr, sub, s, e);
      }
   }
}

// Draw the left and the right marker of the subtitle selected.
void WaveformRendererCairo::draw_marker(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::Rectangle& area) {
   se_dbg(SE_DBG_WAVEFORM);

   if (!document())
      return;

   int height = area.get_height();

   Subtitle selected = document()->subtitles().get_first_selected();
   if (!selected)
      return;

   int start = get_pos_by_time(selected.get_start().totalmsecs);
   int end = get_pos_by_time(selected.get_end().totalmsecs);

   float m_color_marker_left[] = {1.0f, 0.0f, 0.0f, 1.0f};
   float m_color_marker_right[] = {1.0f, 0.6f, 0.0f, 1.0f};

   // left
   set_color(cr, m_color_marker_left);
   cr->move_to(start, 0);
   cr->line_to(start, height);
   cr->stroke();

   // triangle
   int size = TRIANGLE_SIZE;

   // left + top
   cr->move_to(start, 0);
   cr->line_to(start, size);
   cr->line_to(start + size, 0);
   cr->fill();
   // left + bottom
   cr->move_to(start, height);
   cr->line_to(start + size, height);
   cr->line_to(start, height - size);
   cr->fill();

   // right
   set_color(cr, m_color_marker_right);
   cr->move_to(end, 0);
   cr->line_to(end, height);
   cr->stroke();

   // right + top
   cr->move_to(end, 0);
   cr->line_to(end - size, 0);
   cr->line_to(end, size);
   cr->fill();
   // right + bottom
   cr->move_to(end, height);
   cr->line_to(end, height - size);
   cr->line_to(end - size, height);
   cr->fill();
}

// Draw the current position of the player.
void WaveformRendererCairo::draw_player_position(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::Rectangle& area) {
   se_dbg(SE_DBG_WAVEFORM);

   set_color(cr, m_color_player_position);

   int pos = get_pos_by_time(player_time());

   cr->move_to(pos, 0);
   cr->line_to(pos, area.get_height());
   cr->stroke();
}

// Display the time of the mouse
// and the duration of the selected subtitle
void WaveformRendererCairo::display_time_info(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::Rectangle& /*area*/) {
   se_dbg(SE_DBG_WAVEFORM);

   Cairo::TextExtents extents;
   cr->get_text_extents(SubtitleTime::null(), extents);

   double text_width = extents.width;
   double text_height = extents.height;

   int xpos = 0, ypos = 0;
   Gdk::ModifierType mod;

   get_window()->get_pointer(xpos, ypos, mod);

   // display the time of the mouse in the area
   Glib::ustring time = SubtitleTime(get_time_by_pos(get_mouse_coords(xpos))).str();

   set_color(cr, m_color_text);

   cr->move_to(xpos - text_width * 0.5, ypos - text_height);
   cr->show_text(time);

   if (document()) {
      Subtitle selected = document()->subtitles().get_first_selected();
      if (selected) {
         SubtitleTime start = selected.get_start();
         SubtitleTime duration = selected.get_duration();

         int sub_center = get_pos_by_time(start.totalmsecs + duration.totalmsecs / 2);

         cr->move_to(sub_center - get_start_area() - text_width * 0.5, ypos + text_height * 2);
         cr->show_text(duration.str());
      }
   }
}

void WaveformRendererCairo::draw_keyframes(const Cairo::RefPtr<Cairo::Context>& cr, const Gdk::Rectangle& area) {
   se_dbg(SE_DBG_WAVEFORM);

   Player* player = SubtitleEditorWindow::get_instance()->get_player();
   if (player == NULL)
      return;

   Glib::RefPtr<KeyFrames> keyframes = player->get_keyframes();
   if (!keyframes)
      return;

   set_color(cr, m_color_keyframe);

   long start_clip = get_time_by_pos(get_start_area());
   long end_clip = get_time_by_pos(get_end_area());

   for (auto it = keyframes->begin(); it != keyframes->end(); ++it) {
      // display only if it's in the area
      if (*it < start_clip && *it < end_clip)
         continue;
      if (*it > end_clip)
         break;  // the next keyframes are out of the area

      long pos = get_pos_by_time(*it);
      cr->move_to(pos, 0);
      cr->line_to(pos, area.get_height());
      cr->stroke();
   }
}

// HACK!
WaveformRenderer* create_waveform_renderer_cairo() {
   return manage(new WaveformRendererCairo);
}
