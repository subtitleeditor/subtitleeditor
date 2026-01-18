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

#include "filewriter.h"

#include <giomm.h>

#include "debug.h"
#include "encodings.h"
#include "error.h"

FileWriter::FileWriter(const Glib::ustring& uri, const Glib::ustring& charset, const Glib::ustring& newline) {
   m_uri = uri;
   m_charset = charset;
   m_newline = newline;
}

// Write to the file.
// Error: throw an IOFileError exception if failed.
void FileWriter::to_file() {
   // Convert newline if needs
   if (m_newline != "Unix")
      m_data = Glib::Regex::create("\n")->replace(m_data, 0, (m_newline == "Windows") ? "\r\n" : "\r", (Glib::RegexMatchFlags)0);

   try {
      std::string content = Encoding::convert_from_utf8_to_charset(m_data, m_charset);
      Glib::RefPtr<Gio::File> file = Gio::File::create_for_uri(m_uri);
      if (!file)
         throw IOFileError(_("Couldn't open the file."));

      Glib::RefPtr<Gio::FileOutputStream> stream = (file->query_exists()) ? file->replace() : file->create_file();
      if (!stream)
         throw IOFileError("Gio::File could not create stream.");

      stream->write(content);
      // Close the stream to make sure that changes are written now
      stream->close();
      stream.reset();

      se_dbg_msg(SE_DBG_IO, "Success to write the contents on the file '%s' with '%s' charset", m_uri.c_str(), m_charset.c_str());
   } catch (const std::exception& ex) {
      se_dbg_msg(SE_DBG_IO, "Failed to write the contents on the file '%s' with '%s' charset", m_uri.c_str(), m_charset.c_str());
      throw IOFileError(ex.what());
   }
}
