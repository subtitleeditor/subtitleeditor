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

#include <glibmm.h>
#include <vector>

class Waveform {
 public:
  Waveform();
  ~Waveform();

  // Open Wavefrom from file
  static Glib::RefPtr<Waveform> create_from_file(const Glib::ustring &uri);

  guint get_size();

  // long = SubtitleTime.totalmsec
  gint64 get_duration();

  // long = SubtitleTime.totalmsecs
  double get_channel(unsigned int channel, guint64 pos);

  unsigned int get_n_channels();

  bool open(const Glib::ustring &uri);

  bool save(const Glib::ustring &uri);

  // l'uri de la video source du waveform
  Glib::ustring get_video_uri();

  Glib::ustring get_uri();

  void reference() const;
  void unreference() const;

  // protected:

  Glib::ustring m_waveform_uri;
  Glib::ustring m_video_uri;
  guint m_n_channels{0};
  std::vector<double> m_channels[3];
  gint64 m_duration{0};

 protected:
  mutable int ref_count_{0};
};
