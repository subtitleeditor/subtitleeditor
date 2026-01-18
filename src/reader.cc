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

#include "reader.h"

#include <giomm.h>

#include "debug.h"
#include "encodings.h"
#include "error.h"

// Constructor.
Reader::Reader(const Glib::ustring& data) : m_data(data) {
}

Reader::~Reader() {
}

// Return the contents of the file.
const Glib::ustring& Reader::get_data() const {
   return m_data;
}

// Return the newline detected of the file.
Glib::ustring Reader::get_newline() {
   Glib::ustring newline;

   if (Glib::Regex::match_simple("\\r\\n", m_data))
      newline = "Windows";
   else if (Glib::Regex::match_simple("\\r", m_data))
      newline = "Macintosh";
   else if (Glib::Regex::match_simple("\\n", m_data))
      newline = "Unix";
   else
      newline = "Unix";

   se_dbg_msg(SE_DBG_IO, "newline=%s", newline.c_str());

   // default
   return newline;
}

// Get the next line of the file without newline character (CR, LF or CRLF).
bool Reader::getline(Glib::ustring& line) {
   initialize_lines();

   if (m_iter == m_lines.end()) {
      se_dbg_msg(SE_DBG_IO, "EOF");
      return false;
   }

   line = *m_iter;
   ++m_iter;

   se_dbg_msg(SE_DBG_IO, "\"%s\"", line.c_str());

   return true;
}

// Return all lines detected of the file, without newline character (CR, LF or
// CRLF).
std::vector<Glib::ustring> Reader::get_lines() {
   initialize_lines();

   return m_lines;
}

// Split the data to separate lines.
void Reader::initialize_lines() {
   // init only if needs
   if (m_lines_init)
      return;

   se_dbg_msg(SE_DBG_IO, "split lines...");

   m_lines = Glib::Regex::split_simple("\\R", m_data);
   m_iter = m_lines.begin();
   m_lines_init = true;
}
