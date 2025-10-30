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
#include "i18n.h"

struct EncodingInfo {
  const gchar *charset;
  const gchar *name;
};

class Encodings {
 public:
  static EncodingInfo *get_encodings_info();

  static EncodingInfo *get_from_charset(const Glib::ustring &charset);

  // Return a human readable string or empty string, ex:
  // "name (charset)"
  // "Unicode (UTF-8)"
  static Glib::ustring get_label_from_charset(const Glib::ustring &charset);

 protected:
  static bool is_initialized;

  static bool initialize();
};

namespace Encoding {

// Trying to convert from charset to UTF-8.
// Return utf8 string or throw EncodingConvertError exception.
Glib::ustring convert_to_utf8_from_charset(const std::string &content,
                                           const Glib::ustring &charset);

// Trying to autodetect the charset and convert to UTF-8.
// 3 steps:
// - Try UTF-8
// - Try with user encoding preferences
// - Try with all encodings
// Return utf8 string and sets charset found
// or throw EncodingConvertError exception.
Glib::ustring convert_to_utf8(const std::string &content,
                              Glib::ustring &charset);

// Convert the UTF-8 text to the charset.
// Throw EncodingConvertError exception.
std::string convert_from_utf8_to_charset(const Glib::ustring &utf8_content,
                                         const Glib::ustring &charset);

}  // namespace Encoding
