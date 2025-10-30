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

#include "document.h"
#include "subtitleformatio.h"

class SubtitleFormat;

typedef std::list<SubtitleFormat *> SubtitleFormatList;

class SubtitleFormatSystem {
 public:
  // Return the instance.
  static SubtitleFormatSystem &instance();

  // Try to open a subtitle file from the uri.
  // If charset is empty, the automatically detection is used.
  // If format is empty, the automatically detection is used.
  // Exceptions: UnrecognizeFormatError, EncodingConvertError, IOFileError,
  // Glib::Error...
  void open_from_uri(Document *document, const Glib::ustring &uri,
                     const Glib::ustring &charset,
                     const Glib::ustring &format = Glib::ustring());

  // Try to open a ustring as a subtitle file
  // Charset is assumed to be UTF-8.
  // Exceptions: UnrecognizeFormatError, Glib::Error...
  void open_from_data(Document *document, const Glib::ustring &data,
                      const Glib::ustring &format = Glib::ustring());

  // Save the document in a file.
  // Exceptions: UnrecognizeFormatError, EncodingConvertError, IOFileError,
  // Glib::Error...
  void save_to_uri(Document *document, const Glib::ustring &uri,
                   const Glib::ustring &format, const Glib::ustring &charset,
                   const Glib::ustring &newline);

  // Save the document to a ustring. Charset is UTF-8, newline is Unix.
  // Exceptions: UnrecognizeFormatError, Glib::Error...
  void save_to_data(Document *document, Glib::ustring &dst,
                    const Glib::ustring &format);

  // Returns all information about supported subtitles.
  std::list<SubtitleFormatInfo> get_infos();

  // Return information about the subtitle format.
  bool get_info(const Glib::ustring &subtitle_format, SubtitleFormatInfo &info);

  // Check if the subtitle format is supported.
  bool is_supported(const Glib::ustring &format);

  // Return quickly the extension used by the format or an empty string
  Glib::ustring get_extension_of_format(const Glib::ustring &format);

 protected:
  // Constructor
  SubtitleFormatSystem();

  // Destructor
  ~SubtitleFormatSystem();

  // Try to determine the format of the file, and return the format name.
  // Exceptions:
  // UnrecognizeFormatError.
  // EncodingConvertError.
  Glib::ustring get_subtitle_format_from_small_contents(
      const Glib::ustring &uri, const Glib::ustring &charset);

  // Try to determine the format of the subtitles in the submitted ustring
  // Exceptions:
  // UnrecognizeFormatError.
  Glib::ustring get_subtitle_format_from_small_contents(
      const Glib::ustring &data);

  // Try to determine the format of the subtitles in the submitted FileReader
  // Exceptions:
  // UnrecognizeFormatError.
  Glib::ustring get_subtitle_format_from_small_contents(Reader *reader);

  // Create a SubtitleFormat from a name.
  // Throw UnrecognizeFormatError if failed.
  SubtitleFormatIO *create_subtitle_format_io(const Glib::ustring &name);

  // Return a list of SubtitleFormat from ExtensionManager.
  SubtitleFormatList get_subtitle_format_list();

  // Abstract way to read content from file or data (ustring)
  // Exceptions: UnrecognizeFormatError, Glib::Error...
  void open_from_reader(Document *document, Reader *reader,
                        const Glib::ustring &format = Glib::ustring());
};
