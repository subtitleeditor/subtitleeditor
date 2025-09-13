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

#include "subtitlemodel.h"
#include "timeutility.h"

// Cette class fonctionne avec un iterator (SubtitleModel).
// Elle permet de modifier les attributs de l'iter
// il est possible de modifier directement l'iter mais
// dans le cas ou on modifie par exemple le temps "end"
// il faut aussi modifier le temps "duration", etc...
// Passer par cette class evite les oublies de ce genre.
// BEWARE:
// La durée de vie d'un Subtitle est la même que l'iter!
// On ne verifie pas la validiter des arguments! (pour les performances)
class SubtitleCommand;
class Subtitles;
class Document;

class Subtitle {
  friend class Subtitles;
  friend class SubtitleCommand;

 public:
  Subtitle();
  Subtitle(Document *doc, const Glib::ustring &path);
  Subtitle(Document *doc, const Gtk::TreeIter &iter);
  ~Subtitle();

  // valide ?
  operator bool() const;

  Subtitle &operator++();

  Subtitle &operator--();

  bool operator==(const Subtitle &sub) const;
  bool operator!=(const Subtitle &sub) const;

  // Set the number of subtitle.
  void set_num(unsigned int num);

  // Return the number of subtitle.
  unsigned int get_num() const;

  // Return the time mode of the subtitle.
  // TIME or FRAME.
  TIMING_MODE get_timing_mode() const;

  // Return the framerate value. (from document)
  float get_framerate() const;

  // Set the layer name (ASS/SSA).
  void set_layer(const Glib::ustring &layer);

  // Return the layer name (ASS/SSA).
  Glib::ustring get_layer() const;

  // Optimize the calculation by calculating the duration only once.
  void set_start_and_end(const SubtitleTime &start, const SubtitleTime &end);

  // Set the start from time.
  void set_start(const SubtitleTime &time);

  // Set the start from frame.
  void set_start_frame(const long &frame);

  // Get the start as time.
  SubtitleTime get_start() const;

  // Get the start as frame.
  long get_start_frame() const;

  // Set the end from time.
  void set_end(const SubtitleTime &time);

  // Set the end from frame;
  void set_end_frame(const long &frame);

  // Get the end as time.
  SubtitleTime get_end() const;

  // Get the end as frame.
  long get_end_frame() const;

  // Set the duration from time.
  void set_duration(const SubtitleTime &time);

  // Set the duration from frame.
  void set_duration_frame(const long &frame);

  // Get the duration as time.
  SubtitleTime get_duration() const;

  // Get the duration as frame.
  long get_duration_frame() const;

  // Set the style name.
  void set_style(const Glib::ustring &style);

  // Get the style name.
  Glib::ustring get_style() const;

  // Set the actor name. (ASS/SSA)
  void set_name(const Glib::ustring &name);

  // Get the actor name. (ASS/SSA)
  Glib::ustring get_name() const;

  // Set margin values. (ASS/SSA)
  void set_margin_l(const Glib::ustring &value);
  void set_margin_r(const Glib::ustring &value);
  void set_margin_v(const Glib::ustring &value);

  // Return margin values. (ASS/SSA)
  Glib::ustring get_margin_l() const;
  Glib::ustring get_margin_r() const;
  Glib::ustring get_margin_v() const;

  void set_effect(const Glib::ustring &effect);

  Glib::ustring get_effect() const;

  // Set subtitle main text.
  void set_text(const Glib::ustring &text);

  // Return subtitle main text.
  Glib::ustring get_text() const;

  void set_translation(const Glib::ustring &text);

  Glib::ustring get_translation() const;

  // ex: 6 or 3\n3
  Glib::ustring get_characters_per_line_text() const;

  // ex: 6 or 3\n3
  Glib::ustring get_characters_per_line_translation() const;

  void set_note(const Glib::ustring &text);

  Glib::ustring get_note() const;

  // copie le s-t dans sub
  void copy_to(Subtitle &sub);

  void set(const Glib::ustring &name, const Glib::ustring &value);

  Glib::ustring get(const Glib::ustring &name) const;

  void set(const std::map<Glib::ustring, Glib::ustring> &values);

  void get(std::map<Glib::ustring, Glib::ustring> &values);

  // Check if the gab between this and the previous subtitle is long enough
  bool check_gap_before(long mingap);

  // Check if the gab between this and the next subtitle is long enough
  bool check_gap_after(long mingap);

  // Calculate the gap_before value from the start of this and the end of the
  // previous subtitle. The gap is written into the appropriate column. The
  // return value is false if this is the first subtitle (no gap before), true
  // otherwise.
  bool update_gap_before();

  // Calculate the gap_before value from the start of the next and the end of
  // this subtitle. The gap is written into the appropriate column. The return
  // value is false if this is the last subtitle (no gap after), true otherwise.
  bool update_gap_after();

  Glib::ustring convert_value_to_time_string(
      long value, const Glib::ustring &color_name = "");

  void set_characters_per_second_text(double cps);

  double get_characters_per_second_text() const;

  Glib::ustring get_characters_per_second_text_string() const;

  // Checks if the cps of this subtitle is within the specified bounds
  // result: 0 - okay, <0 - too low, >0 - too high
  int check_cps_text(double mincps, double maxcps);

 protected:
  void push_command(const Glib::ustring &name, const Glib::ustring &value);

  void update_characters_per_sec();

  // Convert the value (subtitle timing mode) to the edit timing mode.
  Glib::ustring convert_value_to_view_mode(const long &value);

  // Convert the value (FRAME or TIME) and return as the subtitle time mode.
  long convert_value_to_mode(const long &value, TIMING_MODE viewmode) const;

  // Convert the time value and return as the subtitle time mode.
  long convert_to_value_mode(const SubtitleTime &time) const;

  // Convert the frame value and return as the subtitle time mode.
  long convert_to_value_mode(const long &frame) const;

  // Set the start value in the subtitle time mode. (FRAME or TIME)
  void set_start_value(const long &value);

  // Get the start value in the subtitle time mode. (FRAME or TIME)
  long get_start_value() const;

  // Set the end value in the subtitle time mode. (FRAME or TIME)
  void set_end_value(const long &value);

  // Get the end value in the subtitle time mode. (FRAME or TIME)
  long get_end_value() const;

  // Set the duration value in the subtitle time mode. (FRAME or TIME)
  void set_duration_value(const long &value);

  // Get the duration value in the subtitle time mode. (FRAME or TIME)
  long get_duration_value() const;

 protected:
  static SubtitleColumnRecorder column;
  Document *m_document{nullptr};
  Gtk::TreeIter m_iter;
  Glib::ustring m_path;
};
