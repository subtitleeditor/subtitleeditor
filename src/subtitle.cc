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

#include "subtitle.h"

#include <math.h>

#include <iomanip>

#include "document.h"
#include "utility.h"

class SubtitleCommand : public Command {
  public:
   SubtitleCommand(const Subtitle& sub, const Glib::ustring& name_value, const Glib::ustring& new_value)
       : Command(sub.m_document, "Subtitle edited " + name_value),
         m_path(sub.m_path),
         m_name_value(name_value),
         m_old(sub.get(name_value)),
         m_new(new_value) {
      se_dbg_msg(SE_DBG_APP, "name=<%s> old=<%s> new=<%s>", m_name_value.c_str(), m_old.c_str(), m_new.c_str());
   }

   void execute() {
      Subtitle subtitle(document(), m_path);

      subtitle.set(m_name_value, m_new);
   }

   void restore() {
      Subtitle subtitle(document(), m_path);

      subtitle.set(m_name_value, m_old);
   }

  protected:
   const Glib::ustring m_path;
   const Glib::ustring m_name_value;
   const Glib::ustring m_old;
   const Glib::ustring m_new;
};

// static
SubtitleColumnRecorder Subtitle::column;

Subtitle::Subtitle() {
}

Subtitle::Subtitle(Document* doc, const Glib::ustring& path) : m_document(doc), m_path(path) {
   m_iter = doc->get_subtitle_model()->get_iter(path);
}

Subtitle::Subtitle(Document* doc, const Gtk::TreeIter& it) : m_document(doc), m_iter(it) {
   if (it)
      m_path = doc->get_subtitle_model()->get_string(it);
}

Subtitle::~Subtitle() {
}

void Subtitle::push_command(const Glib::ustring& name, const Glib::ustring& value) {
   if (m_document->is_recording())
      m_document->add_command(new SubtitleCommand(*this, name, value));
}

Subtitle::operator bool() const {
   if (m_iter)
      return true;
   return false;
}

bool Subtitle::operator==(const Subtitle& sub) const {
   return m_iter == sub.m_iter;
}

bool Subtitle::operator!=(const Subtitle& sub) const {
   return m_iter != sub.m_iter;
}

Subtitle& Subtitle::operator++() {
   ++m_iter;

   m_path = (m_iter) ? m_document->get_subtitle_model()->get_string(m_iter) : "";

   return *this;
}

Subtitle& Subtitle::operator--() {
   --m_iter;

   m_path = (m_iter) ? m_document->get_subtitle_model()->get_string(m_iter) : "";

   return *this;
}

// Set the number of subtitle.
void Subtitle::set_num(unsigned int num) {
   (*m_iter)[column.num] = num;
}

// Return the number of subtitle.
unsigned int Subtitle::get_num() const {
   return (*m_iter)[column.num];
}

void Subtitle::set_layer(const Glib::ustring& layer) {
   push_command("layer", layer);

   (*m_iter)[column.layer] = layer;
}

Glib::ustring Subtitle::get_layer() const {
   return (*m_iter)[column.layer];
}

// Return the time mode of the subtitle.
// TIME or FRAME.
TIMING_MODE Subtitle::get_timing_mode() const {
   return m_document->get_timing_mode();  // (*m_iter)[column.mode];
}

// Return the framerate value. (from document)
float Subtitle::get_framerate() const {
   return get_framerate_value(m_document->get_framerate());  // (*m_iter)[column.framerate];
}

// petite optimisation qui permet de calculer
// qu'une seule fois duration
void Subtitle::set_start_and_end(const SubtitleTime& start, const SubtitleTime& end) {
   set_start_value(convert_to_value_mode(start));
   set_end_value(convert_to_value_mode(end));
   // update the duration
   set_duration_value(get_end_value() - get_start_value());
}

// Set the start from time.
void Subtitle::set_start(const SubtitleTime& time) {
   set_start_value(convert_to_value_mode(time));

   // update the duration
   set_duration_value(get_end_value() - get_start_value());
}

// Set the start from frame.
void Subtitle::set_start_frame(const long& frame) {
   set_start_value(convert_to_value_mode(frame));

   // update the duration
   set_duration_value(get_end_value() - get_start_value());
}

// Get the start as time.
SubtitleTime Subtitle::get_start() const {
   return SubtitleTime(convert_value_to_mode(get_start_value(), TIME));
}

// Get the start as frame.
long Subtitle::get_start_frame() const {
   return convert_value_to_mode(get_start_value(), FRAME);
}

// Set the end from time.
void Subtitle::set_end(const SubtitleTime& time) {
   set_end_value(convert_to_value_mode(time));

   // update the duration
   set_duration_value(get_end_value() - get_start_value());
}

// Set the end from frame.
void Subtitle::set_end_frame(const long& frame) {
   set_end_value(convert_to_value_mode(frame));

   // update the duration
   set_duration_value(get_end_value() - get_start_value());
}

// Get the end as time.
SubtitleTime Subtitle::get_end() const {
   return SubtitleTime(convert_value_to_mode(get_end_value(), TIME));
}

// Get the end as frame.
long Subtitle::get_end_frame() const {
   return convert_value_to_mode(get_end_value(), FRAME);
}

// Set the duration from time.
void Subtitle::set_duration(const SubtitleTime& time) {
   set_duration_value(convert_to_value_mode(time));

   // update the end
   set_end_value(get_start_value() + get_duration_value());
}

// Set the duration from frame.
void Subtitle::set_duration_frame(const long& frame) {
   set_duration_value(convert_to_value_mode(frame));

   // update the end
   set_end_value(get_start_value() + get_duration_value());
}

// Get the duration as time.
SubtitleTime Subtitle::get_duration() const {
   return SubtitleTime(convert_value_to_mode(get_duration_value(), TIME));
}

// Get the duration as frame.
long Subtitle::get_duration_frame() const {
   return convert_value_to_mode(get_duration_value(), FRAME);
}

// Calculate the gap_before value from the start of this and the end of the
// previous subtitle. The gap is written into the appropriate column. The return
// value is false if this is the first subtitle (no gap before), true otherwise.
bool Subtitle::update_gap_before() {
   Subtitle prev_sub = m_document->subtitles().get_previous(*this);
   if (prev_sub == 0)
      return false;

   // gap is in milliseconds
   long gap = get_start().totalmsecs - prev_sub.get_end().totalmsecs;

   (*m_iter)[column.gap_before] = gap;
   (*prev_sub.m_iter)[column.gap_after] = gap;
   return true;
}

// Calculate the gap_before value from the start of the next and the end of this
// subtitle. The gap is written into the appropriate column. The return value is
// false if this is the last subtitle (no gap after), true otherwise.
bool Subtitle::update_gap_after() {
   Subtitle next_sub = m_document->subtitles().get_next(*this);
   if (next_sub == 0)
      return false;

   // gap is in milliseconds
   long gap = next_sub.get_start().totalmsecs - get_end().totalmsecs;

   (*m_iter)[column.gap_after] = gap;
   (*next_sub.m_iter)[column.gap_before] = gap;
   return true;
}

// Check if the gab between this and the previous subtitle is long enough.
// Mingap is the minimum gap in milliseconds.
bool Subtitle::check_gap_before(long mingap) {
   // const long mingap =
   //     convert_to_value_mode(SubtitleTime(cfg::get_int(
   //         "timing", "min-gap-between-subtitles")));
   if (((*m_iter)[column.gap_before] >= mingap) || (get_num() <= 1))
      return true;

   return false;
}

// Check if the gab between this and the next subtitle is long enough
// Mingap is the minimum gap in milliseconds.
bool Subtitle::check_gap_after(long mingap) {
   // const long mingap =
   //     convert_to_value_mode(SubtitleTime(cfg::get_int(
   //         "timing", "min-gap-between-subtitles")));
   Subtitle next_sub = m_document->subtitles().get_next(*this);

   if (((*m_iter)[column.gap_after] >= mingap) || (next_sub == 0))
      return true;

   return false;
}

// Set the start value in the subtitle time mode. (FRAME or TIME)
void Subtitle::set_start_value(const long& value) {
   push_command("start", to_string(value));
   (*m_iter)[column.start_value] = value;
   update_gap_before();
}

// Set the end value in the subtitle time mode. (FRAME or TIME)
void Subtitle::set_end_value(const long& value) {
   push_command("end", to_string(value));
   (*m_iter)[column.end_value] = value;
   update_gap_after();
}

Glib::ustring Subtitle::convert_value_to_time_string(long value, const Glib::ustring& color_name) {
   if (color_name.empty())
      return convert_value_to_view_mode(value);
   return Glib::ustring::compose("<span foreground=\"%1\">%2</span>", color_name, convert_value_to_view_mode(value));
}

// Get the start value in the subtitle time mode. (FRAME or TIME)
long Subtitle::get_start_value() const {
   return (*m_iter)[column.start_value];
}

// Get the end value in the subtitle time mode. (FRAME or TIME)
long Subtitle::get_end_value() const {
   return (*m_iter)[column.end_value];
}

// Set the duration value in the subtitle time mode. (FRAME or TIME)
void Subtitle::set_duration_value(const long& value) {
   push_command("duration", to_string(value));

   (*m_iter)[column.duration_value] = value;
   update_characters_per_sec();
}

// Get the duration value in the subtitle time mode. (FRAME or TIME)
long Subtitle::get_duration_value() const {
   return (*m_iter)[column.duration_value];
}

// Convert the value (FRAME or TIME) and return as the subtitle time mode.
long Subtitle::convert_value_to_mode(const long& value, TIMING_MODE mode) const {
   if (get_timing_mode() == TIME) {
      if (mode == TIME)
         return value;
      else   // FRAME
         return SubtitleTime::time_to_frame(value, get_framerate());
   } else {  // viewmode == FRAME
      if (mode == FRAME)
         return value;
      else   // TIME
         return SubtitleTime::frame_to_time(value, get_framerate()).totalmsecs;
   }
   return 0;
}

// Convert the time value and return as the subtitle time mode.
long Subtitle::convert_to_value_mode(const SubtitleTime& time) const {
   if (get_timing_mode() == TIME)
      return time.totalmsecs;
   // else  FRAME
   return SubtitleTime::time_to_frame(time.totalmsecs, get_framerate());
}

// Convert the frame value and return as the subtitle time mode.
long Subtitle::convert_to_value_mode(const long& frame) const {
   if (get_timing_mode() == FRAME)
      return frame;
   // else TIME
   return SubtitleTime::frame_to_time(frame, get_framerate()).totalmsecs;
}

// Convert the value (subtitle timing mode) to the edit timing mode.
Glib::ustring Subtitle::convert_value_to_view_mode(const long& value) {
   TIMING_MODE view_mode = m_document->get_edit_timing_mode();

   Glib::ustring text;

   if (get_timing_mode() == TIME) {
      if (view_mode == TIME)
         return SubtitleTime(value).str();
      else   // FRAME
         return to_string(SubtitleTime::time_to_frame(SubtitleTime(value), get_framerate()));
   } else {  // if(get_timing_mode() == FRAME)
      if (view_mode == FRAME)
         return to_string(value);
      else   // TIME
         return SubtitleTime::frame_to_time(value, get_framerate()).str();
   }
   return "INVALID";
}

void Subtitle::set_style(const Glib::ustring& style) {
   push_command("style", style);

   (*m_iter)[column.style] = style;
}

Glib::ustring Subtitle::get_style() const {
   return (*m_iter)[column.style];
}

void Subtitle::set_name(const Glib::ustring& name) {
   push_command("name", name);

   (*m_iter)[column.name] = name;
}

Glib::ustring Subtitle::get_name() const {
   return (*m_iter)[column.name];
}

void Subtitle::set_margin_l(const Glib::ustring& value) {
   push_command("margin-l", value);

   (*m_iter)[column.marginL] = value;
}

Glib::ustring Subtitle::get_margin_l() const {
   return (*m_iter)[column.marginL];
}

void Subtitle::set_margin_r(const Glib::ustring& value) {
   push_command("margin-r", value);

   (*m_iter)[column.marginR] = value;
}

Glib::ustring Subtitle::get_margin_r() const {
   return (*m_iter)[column.marginR];
}

void Subtitle::set_margin_v(const Glib::ustring& value) {
   push_command("margin-v", value);

   (*m_iter)[column.marginV] = value;
}

Glib::ustring Subtitle::get_margin_v() const {
   return (*m_iter)[column.marginV];
}

void Subtitle::set_effect(const Glib::ustring& effect) {
   push_command("effect", effect);

   (*m_iter)[column.effect] = effect;
}

Glib::ustring Subtitle::get_effect() const {
   return (*m_iter)[column.effect];
}

void Subtitle::set_text(const Glib::ustring& text) {
   push_command("text", text);

   (*m_iter)[column.text] = text;

   // characters per line
   if (text.size() == 0) {
      (*m_iter)[column.characters_per_line_text] = "0";
   } else {
      std::vector<int> num_characters = utility::get_characters_per_line(text);
      std::string cpl;

      unsigned int count = 0;
      for (const auto& number : num_characters) {
         if (count == 0) {
            cpl += to_string(number);
         } else {
            cpl += "\n" + to_string(number);
         }
         ++count;
      }
      (*m_iter)[column.characters_per_line_text] = cpl;
   }

   update_characters_per_sec();
}

Glib::ustring Subtitle::get_text() const {
   return (*m_iter)[column.text];
}

void Subtitle::set_translation(const Glib::ustring& text) {
   push_command("translation", text);

   (*m_iter)[column.translation] = text;

   // characters per line
   if (text.size() == 0) {
      (*m_iter)[column.characters_per_line_translation] = "0";
   } else {
      std::vector<int> num_characters = utility::get_characters_per_line(text);
      std::string cpl;

      unsigned int count = 0;
      for (const auto& number : num_characters) {
         if (count == 0) {
            cpl += to_string(number);
         } else {
            cpl += "\n" + to_string(number);
         }
         ++count;
      }

      (*m_iter)[column.characters_per_line_translation] = cpl;
   }
}

Glib::ustring Subtitle::get_translation() const {
   return (*m_iter)[column.translation];
}

// ex: 6 or 3\n3
Glib::ustring Subtitle::get_characters_per_line_text() const {
   return (*m_iter)[column.characters_per_line_text];
}

// ex: 6 or 3\n3
Glib::ustring Subtitle::get_characters_per_line_translation() const {
   return (*m_iter)[column.characters_per_line_translation];
}

void Subtitle::set_characters_per_second_text(double cps) {
   push_command("characters-per-second-text", Glib::ustring::format(std::fixed, std::setprecision(1), cps));

   (*m_iter)[column.characters_per_second_text] = cps;
}

double Subtitle::get_characters_per_second_text() const {
   return static_cast<double>((*m_iter)[column.characters_per_second_text]);
}

Glib::ustring Subtitle::get_characters_per_second_text_string() const {
   return Glib::ustring::format(std::fixed, std::setprecision(1), get_characters_per_second_text());
}

// Checks if the cps of this subtitle is within the specified bounds
// result: 0 - okay, <0 - too low, >0 - too high
int Subtitle::check_cps_text(double mincps, double maxcps) {
   int retval = 0;

   // round cps to 1/10 precision
   double cps = round(10.0 * get_characters_per_second_text()) / 10.0;

   // FIXME tomas-kitone, before fixing this strange comparing code,
   // try uncommenting the printf below, compiling subtitleeditor, setting max
   // cps to a non-integer value (e.g. 16.7) and minimizing the duration of a
   // subtitle. Then look at maxcps-cps that is printed out for the minimized
   // subtitle. It was always greater than zero when I tested this. I think it's
   // because the rounding code actually leaves some binary residue that looks
   // like a value of the order of 10^-15.

   // compare cps to min and max values while ignoring the fuzz left after the
   // floating-point division
   if ((mincps - cps) > 0.0001)
      retval = -1;
   else if ((cps - maxcps) > 0.0001)
      retval = 1;

   // printf("sub # : %i, cps: %e, maxcps: %e, cps-maxcps=%e, retval: %i\n",
   // get_num(), cps, maxcps, (maxcps-cps), retval );

   return (retval);
}

void Subtitle::set_note(const Glib::ustring& text) {
   push_command("note", text);

   (*m_iter)[column.note] = text;
}

Glib::ustring Subtitle::get_note() const {
   return (*m_iter)[column.note];
}

// copie le s-t dans sub
void Subtitle::copy_to(Subtitle& sub) {
   sub.set_layer(get_layer());
   sub.set_start_and_end(get_start(), get_end());
   sub.set_style(get_style());
   sub.set_name(get_name());
   sub.set_margin_l(get_margin_l());
   sub.set_margin_r(get_margin_r());
   sub.set_margin_v(get_margin_v());
   sub.set_effect(get_effect());
   sub.set_text(get_text());
   sub.set_translation(get_translation());
   sub.set_note(get_note());
}

void Subtitle::set(const Glib::ustring& name, const Glib::ustring& value) {
   se_dbg_msg(SE_DBG_APP, "name=<%s> value=<%s>", name.c_str(), value.c_str());

   if (name == "path")
      m_path = value;
   else if (name == "start")
      set_start_value(utility::string_to_long(value));
   else if (name == "end")
      set_end_value(utility::string_to_long(value));
   else if (name == "duration")
      set_duration_value(utility::string_to_long(value));
   else if (name == "text")
      set_text(value);
   else if (name == "translation")
      set_translation(value);
   else if (name == "layer")
      set_layer(value);
   else if (name == "style")
      set_style(value);
   else if (name == "name")
      set_name(value);
   else if (name == "margin-l")
      set_margin_l(value);
   else if (name == "margin-r")
      set_margin_r(value);
   else if (name == "margin-v")
      set_margin_v(value);
   else if (name == "effect")
      set_effect(value);
   else if (name == "note")
      set_note(value);
   else if (name == "characters-per-second-text")
      set_characters_per_second_text(utility::string_to_double(value));
   else
      std::cerr << "Subtitle::set UNKNOWN " << name << " " << value << std::endl;
}

Glib::ustring Subtitle::get(const Glib::ustring& name) const {
   if (name == "path")
      return m_path;
   else if (name == "start")
      return to_string(get_start_value());
   else if (name == "end")
      return to_string(get_end_value());
   else if (name == "duration")
      return to_string(get_duration_value());
   else if (name == "text")
      return get_text();
   else if (name == "translation")
      return get_translation();
   else if (name == "layer")
      return get_layer();
   else if (name == "style")
      return get_style();
   else if (name == "name")
      return get_name();
   else if (name == "margin-l")
      return get_margin_l();
   else if (name == "margin-r")
      return get_margin_r();
   else if (name == "margin-v")
      return get_margin_v();
   else if (name == "effect")
      return get_effect();
   else if (name == "note")
      return get_note();
   else if (name == "characters-per-second-text")
      return get_characters_per_second_text_string();
   else
      std::cerr << "Subtitle::get UNKNOWN " << name << std::endl;

   return Glib::ustring();
}

void Subtitle::set(const std::map<Glib::ustring, Glib::ustring>& values) {
   for (const auto& m_v : values) {
      set(m_v.first, m_v.second);
   }
}

void Subtitle::get(std::map<Glib::ustring, Glib::ustring>& values) {
   values["path"] = get("path");
   values["layer"] = get("layer");
   values["start"] = get("start");
   values["end"] = get("end");
   values["duration"] = get("duration");
   values["style"] = get("style");
   values["name"] = get("name");
   values["margin-l"] = get("margin-l");
   values["margin-r"] = get("margin-r");
   values["margin-v"] = get("margin-v");
   values["effect"] = get("effect");
   values["text"] = get("text");
   values["translation"] = get("translation");
   values["note"] = get("note");
}

void Subtitle::update_characters_per_sec() {
   SubtitleTime duration = get_duration();
   double cps = utility::get_characters_per_second(get_text(), duration.totalmsecs);
   (*m_iter)[column.characters_per_second_text] = cps;
}
