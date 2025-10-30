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

#include <gtkmm.h>
#include <memory>

#include "dialogutility.h"
#include "widget_config_utility.h"

// Internal class
class ComboBoxSubtitleFormat;
class ComboBoxNewLine;
class ComboBoxEncoding;
class ComboBoxVideo;

class DialogFileChooser : public Gtk::FileChooserDialog {
 public:
  // Glade constructor
  DialogFileChooser(BaseObjectType* cobject, const Glib::ustring& name);

  DialogFileChooser(const Glib::ustring& title, Gtk::FileChooserAction action,
                    const Glib::ustring& name);

  virtual ~DialogFileChooser();

  // Define the current file filter.
  // ex: 'Subtitle Editor Project', 'SubRip', 'MicroDVD' ...
  void set_current_filter(const Glib::ustring& subtitleformat_name);

  // This can be use to setup the document name based on video uri
  void set_filename_from_another_uri(const Glib::ustring& another_uri,
                                     const Glib::ustring& ext);

  // Internally call set_current_folder and set_current_name with dirname and
  // basename
  void set_current_folder_and_name(const Glib::ustring& filename);

 protected:
  Glib::ustring m_name;
};

// Dialog open file chooser with Encoding and Video options.
class DialogOpenDocument : public DialogFileChooser {
 public:
  typedef std::unique_ptr<DialogOpenDocument> unique_ptr;

  // Constructor
  DialogOpenDocument(BaseObjectType* cobject,
                     const Glib::RefPtr<Gtk::Builder>& builder);

  // Returns the encoding value.
  // Charset or empty string (Auto Detected)
  Glib::ustring get_encoding() const;

  // Returns the video uri or empty string.
  Glib::ustring get_video_uri() const;

  void show_video(bool state);

  // Create a instance of the dialog.
  static unique_ptr create();

 protected:
  // The current folder has changed, need to update the ComboBox Video
  void on_current_folder_changed();

  // The file selection has changed, need to update the ComboBox Video
  void on_selection_changed();

 protected:
  ComboBoxEncoding* m_comboEncodings;
  Gtk::Label* m_labelVideo;
  ComboBoxVideo* m_comboVideo;
};

// Dialog save file chooser with Format, Encoding and NewLine options.
class DialogSaveDocument : public DialogFileChooser {
 public:
  typedef std::unique_ptr<DialogSaveDocument> unique_ptr;

  // Constructor
  DialogSaveDocument(BaseObjectType* cobject,
                     const Glib::RefPtr<Gtk::Builder>& builder);

  // Sets the current format value.
  void set_format(const Glib::ustring& format);

  // Returns the subtitle format value.
  Glib::ustring get_format() const;

  // Sets the current encoding value.
  void set_encoding(const Glib::ustring& encoding);

  // Returns the encoding value. or empty string (Auto Detected).
  Glib::ustring get_encoding() const;

  // Sets the current newline.
  void set_newline(const Glib::ustring& newline);

  // Returns the newline value.
  // Windows or Unix.
  Glib::ustring get_newline() const;

  // Update the extension of the current filename.
  void on_combo_format_changed();

  // Create a instance of the dialog.
  static unique_ptr create();

 protected:
  ComboBoxSubtitleFormat* m_comboFormat;
  ComboBoxEncoding* m_comboEncodings;
  ComboBoxNewLine* m_comboNewLine;
};

// Dialog Import file chooser with Encoding option.
class DialogImportText : public DialogFileChooser {
 public:
  typedef std::unique_ptr<DialogImportText> unique_ptr;

  // Constructor
  DialogImportText(BaseObjectType* cobject,
                   const Glib::RefPtr<Gtk::Builder>& builder);

  // Returns the encoding value.
  // Charset or empty string (Auto Detected)
  Glib::ustring get_encoding() const;

  // Returns whether blank lines separate subtitles
  bool get_blank_line_mode() const;

  // Create a instance of the dialog.
  static unique_ptr create();

 protected:
  ComboBoxEncoding* m_comboEncodings;
  Gtk::CheckButton* m_checkBlankLines;
};

// Dialog export file chooser with Encoding and NewLine options.
class DialogExportText : public DialogFileChooser {
 public:
  typedef std::unique_ptr<DialogExportText> unique_ptr;

  // Constructor
  DialogExportText(BaseObjectType* cobject,
                   const Glib::RefPtr<Gtk::Builder>& builder);

  // Returns the encoding value. or empty string (Auto Detected).
  Glib::ustring get_encoding() const;

  // Returns the newline value.
  // Windows or Unix.
  Glib::ustring get_newline() const;

  // Returns whether subtitles should be separated with blank lines
  bool get_blank_line_mode() const;

  // Create a instance of the dialog.
  static unique_ptr create();

 protected:
  ComboBoxEncoding* m_comboEncodings;
  ComboBoxNewLine* m_comboNewLine;
  Gtk::CheckButton* m_checkBlankLines;
};

class DialogOpenVideo : public Gtk::FileChooserDialog {
 public:
  DialogOpenVideo();
  ~DialogOpenVideo();
};

// Waveform or Video/Audio
class DialogOpenWaveform : public Gtk::FileChooserDialog {
 public:
  DialogOpenWaveform();
  ~DialogOpenWaveform();
};

// Keyframe or Video
class DialogOpenKeyframe : public Gtk::FileChooserDialog {
 public:
  DialogOpenKeyframe();
  ~DialogOpenKeyframe();
};
