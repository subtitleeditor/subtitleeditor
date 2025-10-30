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

#include "writer.h"

// Helper to write a file.
// Convert from UTF-8 to the character coding.
// Convert Unix newline to Windows or Macintosh if need.
class FileWriter : public Writer {
 public:
  FileWriter(const Glib::ustring &uri, const Glib::ustring &charset,
             const Glib::ustring &newline);

  // Write to the file.
  // Error: throw an IOFileError exception if failed.
  void to_file();

 protected:
  Glib::ustring m_uri;
  Glib::ustring m_charset;
  Glib::ustring m_newline;
};
