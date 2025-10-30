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

#include <giomm.h>
#include "debug.h"
#include "encodings.h"
#include "error.h"
#include "filereader.h"

// Reads an entire file into a string, with good error checking.
// If charset is empty, auto detection is try.
bool get_contents_from_file(const Glib::ustring &uri,
                            const Glib::ustring &charset,
                            Glib::ustring &utf8_contents,
                            Glib::ustring &charset_contents,
                            int max_data_size) {
  se_dbg_msg(SE_DBG_IO, "Try to get contents from file uri=%s with charset=%s",
             uri.c_str(), charset.c_str());

  try {
    Glib::ustring content;

    {
      Glib::RefPtr<Gio::File> file = Gio::File::create_for_uri(uri);
      if (!file)
        throw IOFileError(_("Couldn't open the file."));

      gchar *raw = NULL;
      gsize bytes_read = 0;
      std::string e_tag;

      if (file->load_contents(raw, bytes_read, e_tag) == false)
        throw IOFileError(_("Couldn't read the contents of the file."));

      content = std::string(raw, bytes_read);

      g_free(raw);
    }

    if (max_data_size > 0) {
      if (content.size() > max_data_size)
        content = content.substr(0, max_data_size);
    }

    if (charset.empty()) {
      // Try to autodetect
      utf8_contents = Encoding::convert_to_utf8(content, charset_contents);

      se_dbg_msg(SE_DBG_IO,
                 "Success to get the contents of the file %s with %s charset",
                 uri.c_str(), charset_contents.c_str());

      return true;
    } else {
      // try with charset
      utf8_contents = Encoding::convert_to_utf8_from_charset(content, charset);

      se_dbg_msg(SE_DBG_IO,
                 "Success to get the contents of the file %s with %s charset",
                 uri.c_str(), charset.c_str());
      return true;
    }
  } catch (const std::exception &ex) {
    throw IOFileError(ex.what());
  }

  return false;
}

// Constructor.
// Open the file from an uri and convert the contents from charset to UTF-8.
// If charset is empty, try to autodetect the character coding.
// Error: throw an IOFileError exception if failed.
FileReader::FileReader(const Glib::ustring &uri, const Glib::ustring &charset,
                       int max_data_size)
    : Reader(), m_charset("UTF-8") {
  if (get_contents_from_file(uri, charset, m_data, m_charset, max_data_size) ==
      false)
    return;

  m_uri = uri;
}

// Return the uri of the file.
Glib::ustring FileReader::get_uri() const {
  return m_uri;
}

// Return the charset of the file.
Glib::ustring FileReader::get_charset() const {
  return m_charset;
}
