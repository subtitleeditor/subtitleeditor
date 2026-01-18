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

// Helper to read data (UTF-8) from memory.
// Return lines without character of newline (CR,LF or CRLF)
class Reader {
  public:
   // Constructor.
   explicit Reader(const Glib::ustring& data = Glib::ustring());

   // Destructor
   virtual ~Reader();

   // Return the contents of the file.
   const Glib::ustring& get_data() const;

   // Return the newline detected of the file.
   Glib::ustring get_newline();

   // Get the next line of the file without newline character (CR, LF or CRLF).
   bool getline(Glib::ustring& line);

   // Return all lines detected of the file, without newline character (CR, LF or
   // CRLF).
   std::vector<Glib::ustring> get_lines();

  private:
   // Split the data to separate lines.
   void initialize_lines();

  protected:
   Glib::ustring m_data;
   bool m_lines_init{false};
   std::vector<Glib::ustring>::const_iterator m_iter;
   std::vector<Glib::ustring> m_lines;
};
