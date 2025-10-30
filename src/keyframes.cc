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
#include <cstdio>
#include <iostream>
#include "error.h"
#include "keyframes.h"
#include "utility.h"

KeyFrames::KeyFrames() {
  reference();
}

KeyFrames::~KeyFrames() {
}

void KeyFrames::reference() const {
  ++ref_count_;
}

void KeyFrames::unreference() const {
  if (!(--ref_count_))
    delete this;
}

void KeyFrames::set_uri(const Glib::ustring &uri) {
  m_uri = uri;
}

Glib::ustring KeyFrames::get_uri() const {
  return m_uri;
}

void KeyFrames::set_video_uri(const Glib::ustring &uri) {
  m_video_uri = uri;
}

Glib::ustring KeyFrames::get_video_uri() const {
  return m_video_uri;
}

bool KeyFrames::open(const Glib::ustring &uri) {
  try {
    Glib::RefPtr<Gio::File> file = Gio::File::create_for_uri(uri);
    Glib::RefPtr<Gio::FileInputStream> fstream = file->read();
    Glib::RefPtr<Gio::DataInputStream> dstream =
        Gio::DataInputStream::create(fstream);

    guint32 version = 0;
    guint32 num_of_kf = 0;
    std::string line;
    // Check the file type
    if (!dstream->read_line(line))
      throw SubtitleError(_("Couldn't recognize format of the file."));

    if (line == "#subtitleeditor keyframes v1")
      version = 1;
    else if (line == "#subtitleeditor keyframes v2")
      version = 2;
    else
      throw SubtitleError(_("Couldn't recognize format of the file."));

    if (version == 2) {
      // Read the video uri
      dstream->read_line(line);
      set_video_uri(line);
      // Read the keyframes number
      dstream->read_line(line);
      num_of_kf = utility::string_to_int(line);
      // Read the keyframes data
      resize(num_of_kf);
      dstream->read(&(*this)[0], sizeof(long) * num_of_kf);
    } else if (version == 1) {  // TODO deprecated
      // Read the keyframes number
      if ((dstream->read_line(line) &&
           sscanf(line.c_str(), "size: %d", &num_of_kf) != 0) == false)
        throw SubtitleError(_("Couldn't get the keyframe size on the file."));
      // Read the keyframes data
      reserve(num_of_kf);
      while (dstream->read_line(line)) {
        push_back(utility::string_to_int(line));
      }
    }
    // Update the uri of the keyframe
    set_uri(uri);
    return true;
  } catch (const std::exception &ex) {
    std::cerr << Glib::ustring::compose("KeyFrames::open failed '%1' : %2", uri,
                                        ex.what())
              << std::endl;
  }
  return false;
}

bool KeyFrames::save(const Glib::ustring &uri) {
  try {
    Glib::RefPtr<Gio::File> file = Gio::File::create_for_uri(uri);
    // If the file exists then replace it. Otherwise, create it.
    Glib::RefPtr<Gio::FileOutputStream> stream =
        (file->query_exists()) ? file->replace() : file->create_file();

    if (!stream)
      throw SubtitleError(Glib::ustring::compose(
          "Gio::File::create_file returned an empty ptr from the uri '%1'.",
          uri));

    // Write header (version + video uri + num of kf)
    stream->write("#subtitleeditor keyframes v2\n");
    stream->write(Glib::ustring::compose("%1\n", get_video_uri()));
    stream->write(Glib::ustring::compose("%1\n", size()));
    // Write keyframes data
    stream->write(&(*this)[0], sizeof(long) * size());
    // Close the stream to make sure that changes are write now.
    stream->close();
    stream.reset();
    // Update the uri of the keyframe
    set_uri(uri);
    return true;
  } catch (const std::exception &ex) {
    std::cerr << Glib::ustring::compose("KeyFrames::save failed '%1' : %2", uri,
                                        ex.what())
              << std::endl;
  }
  return false;
}

Glib::RefPtr<KeyFrames> KeyFrames::create_from_file(const Glib::ustring &uri) {
  Glib::RefPtr<KeyFrames> kf(new KeyFrames);
  if (kf->open(uri))
    return kf;
  return Glib::RefPtr<KeyFrames>(NULL);
}
