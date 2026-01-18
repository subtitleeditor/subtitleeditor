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

#include <math.h>

#include <fstream>
#include <iostream>

// Open Wavefrom from file
Glib::RefPtr<Waveform> Waveform::create_from_file(const Glib::ustring& uri) {
   Glib::RefPtr<Waveform> wf = Glib::RefPtr<Waveform>(new Waveform);
   if (!wf->open(uri)) {
      std::cout << "SE Info: The file '" << uri << "' is not a waveform file" << std::endl;
      wf.clear();

      return Glib::RefPtr<Waveform>(NULL);
   }

   return wf;
}

Waveform::Waveform() {
   reference();
}

Waveform::~Waveform() {
}

void Waveform::reference() const {
   ++ref_count_;
}

void Waveform::unreference() const {
   if (!(--ref_count_))
      delete this;
}

guint Waveform::get_size() {
   return m_channels[0].size();
}

gint64 Waveform::get_duration() {
   return m_duration;
}

double Waveform::get_channel(unsigned int ch, guint64 pos) {
   pos = CLAMP(pos, 0, get_size());
   ch = CLAMP(ch, 0, m_n_channels);

   return m_channels[ch][pos];
}

unsigned int Waveform::get_n_channels() {
   return m_n_channels;
}

bool Waveform::open(const Glib::ustring& file_uri) {
   Glib::ustring filename = Glib::filename_from_uri(file_uri);

   std::ifstream file(filename.c_str(), std::ios_base::binary);

   if (!file) {
      return false;
   }

   std::string line;

   if (!std::getline(file, line)) {
      file.close();
      return false;
   }

   int version = 0;

   if (line == "waveform") {
      version = 1;
   } else if (line == "waveform v2") {
      version = 2;
   } else {
      file.close();
      return false;
   }

   if (!std::getline(file, line)) {
      file.close();
      return false;
   }

   m_video_uri = line;

   file.read((char*)&m_n_channels, sizeof(m_n_channels));
   file.read((char*)&m_duration, sizeof(m_duration));

   if (version == 1) {
      m_duration = m_duration / 1000000;  // GST_MSECOND=1000000;
   }

   for (unsigned int n = 0; n < m_n_channels; ++n) {
      std::vector<double>::size_type size = 0;

      file.read((char*)&size, sizeof(size));

      m_channels[n].resize(size);

      for (unsigned int i = 0; i < size; ++i) {
         file.read((char*)&m_channels[n][i], sizeof(double));
      }
   }

   file.close();

   m_waveform_uri = file_uri;

   return true;
}

bool Waveform::save(const Glib::ustring& file_uri) {
   Glib::ustring filename = Glib::filename_from_uri(file_uri);

   std::ofstream file(filename.c_str(), std::ios_base::binary);

   if (!file)
      return false;

   file << "waveform v2" << std::endl;

   file << m_video_uri << std::endl;

   file.write((const char*)&m_n_channels, sizeof(m_n_channels));
   file.write((const char*)&m_duration, sizeof(m_duration));

   for (unsigned int n = 0; n < m_n_channels; ++n) {
      std::vector<double>::size_type size = m_channels[n].size();

      file.write((const char*)&size, sizeof(size));

      for (unsigned int i = 0; i < size; ++i) {
         file.write((const char*)&m_channels[n][i], sizeof(double));
      }
   }

   file.close();

   m_waveform_uri = file_uri;

   return true;
}

Glib::ustring Waveform::get_uri() {
   return m_waveform_uri;
}

Glib::ustring Waveform::get_video_uri() {
   return m_video_uri;
}
