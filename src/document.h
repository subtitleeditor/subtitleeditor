#pragma once

// subtitleeditor -- a tool to create or edit subtitle
//
// https://kitone.github.io/subtitleeditor/
// https://github.com/kitone/subtitleeditor/
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

#include <sigc++/sigc++.h>

#include <map>
#include <string>
#include <vector>

#include "commandsystem.h"
#include "scriptinfo.h"
#include "stylemodel.h"
#include "styles.h"
#include "subtitles.h"
#include "subtitleview.h"
#include "timeutility.h"

typedef Glib::RefPtr<SubtitleModel> SubtitleModelPtr;
typedef SubtitleView *SubtitleViewPtr;
typedef std::vector<Document *> DocumentList;

// A Document is the base of all, it represent the model (SubtitleModel) and the
// view (SubtitleView), the metadata like the subtitle format of the document,
// the character coding, the timing mode... all is there. Every  action on
// subtitles begin from this class.
class Document : protected CommandSystem {
 public:
  // Create a new document from an uri, if the charset is empty then it will try
  // to auto detect the good value. This function display a dialog ask or error
  // if needed. Return a new document or NULL.
  static Document *create_from_file(
      const Glib::ustring &uri, const Glib::ustring &charset = Glib::ustring());

  // Constructor
  // The default values of the document are set from the user config.
  Document(bool create_new = true);

  // Constructor by copy
  Document(Document &copy, bool copy_subtitles);

  // Destructor
  ~Document();

  // Try to open a file from an uri.
  // The document charset is used to open the file.
  // Prefer the function create_from_file for create a new document.
  // Launch an Exception if it fails.
  // Exceptions: UnrecognizeFormatError, EncodingConvertError, IOFileError,
  // Glib::Error...
  void open(const Glib::ustring &uri);

  // Try to save the document to the file.
  // The format, charset and newline used are the document values.
  // The document name will be renamed from the uri.
  // An error dialog will be display if needed.
  // Return true if it succeeds or false.
  bool save(const Glib::ustring &filename);

  // Define the subtitle format of the document.
  // (SubRip, MicroDVD...)
  void setFormat(const Glib::ustring &format);

  // Return the subtitle format of the document.
  // (SubRip, MicroDVD...)
  Glib::ustring getFormat();

  // Define the charset of the document.
  void setCharset(const Glib::ustring &charset);

  // Return the charset of the document.
  Glib::ustring getCharset();

  // Define the newline type of the document.
  // Value can be "Unix", "Windows" or "Macintosh"
  void setNewLine(const Glib::ustring &name);

  // Return the newline type of the document.
  // Value can be "Unix", "Windows" or "Macintosh"
  Glib::ustring getNewLine();

  // Define the full filename of the document.
  // ex: /home/toto/subtitle05.ass
  // A signal "document-property-changed" is emitted.
  void setFilename(const Glib::ustring &filename);

  // Return the full filename of the document.
  Glib::ustring getFilename();

  // Define the name of the document.
  // A signal "document-property-changed" is emitted.
  void setName(const Glib::ustring &name);

  // Return the name of the document.
  // If the fullname is "/home/toto/subtitle05.ass"
  // then return "subtitle05.ass"
  Glib::ustring getName();

  // Return the StyleModel of the document.
  Glib::RefPtr<StyleModel> get_style_model();

  // FIXME (Need to be fixed)
  // Return the ScriptInfo of the document.
  ScriptInfo &get_script_info();

  // Display a message to the user. (statusbar)
  void message(const gchar *format, ...);

  // Signal connector to received message from the document.
  sigc::signal<void, Glib::ustring> &get_signal_message();

  // Display a flash message (3 seconds) to the user. (statusbar)
  void flash_message(const gchar *format, ...);

  // Signal connector to received flash message from the document.
  sigc::signal<void, Glib::ustring> &get_signal_flash_message();

  // Return a Subtitles manager of the document.
  Subtitles subtitles();

  // Return a Styles manager of the document.
  Styles styles();

  // Command System

  // The document has changed (start_command and finish_command are used)
  // after save the document toggle state of false
  // the signal "document-changed" is used after any change
  bool get_document_changed();

  // Turn m_document_changed to true and emit a signal "document-changed"
  void make_document_changed();

  // Turn m_document_changed to false and emit a signal "document-changed"
  void make_document_unchanged();

  // Command System (Undo/Redo)
  // start_command(_("XXX"));
  // change subtitle...
  // finish_command();
  void start_command(const Glib::ustring &description);
  void add_command(Command *cmd);
  void finish_command();

  CommandSystem &get_command_system();

  // Return the subtitle view widget (SubtitleView -> Gtk::TreeView)
  Gtk::Widget *widget();

  // Define the timing mode of the document.
  // This is the internal timing mode (frame or time) used
  // to represent subtitle.
  // A signal "timing-mode-changed" is emitted.
  void set_timing_mode(TIMING_MODE mode);

  // Return the timing mode of the document.
  // This is the internal timing mode (frame or time) used
  // to represent subtitle.
  TIMING_MODE get_timing_mode();

  // Define the editing timing mode of the document.
  // A signal "edit-timing-mode-changed" is emitted.
  void set_edit_timing_mode(TIMING_MODE mode);

  // Return the editing timing mode of the document.
  TIMING_MODE get_edit_timing_mode();

  // Define the framerate of the document.
  // A signal "framerate-changed" is emitted.
  void set_framerate(FRAMERATE framerate);

  // Return the framerate of the document.
  FRAMERATE get_framerate();

  // Return a signal connector from its name.
  // The list of signals available:
  // "document-property-changed"
  //  filename, format, charset...
  // "subtitle-selection-changed"
  //  the selection of subtitle changed.
  // "subtitle-time-changed"
  //  time of subtitle(s) changed.
  // "subtitle-deleted"
  //  one or more subtitles have been removed.
  // "subtitle-insered"
  //  one or more subtitles have been created.
  // "framerate-changed"
  //  the framerate value of document changed.
  // "timing-mode-changed"
  //  the internal timing mode of the document changed.
  // "edit-timing-mode-changed"
  //  the (external) timing mode of the document editing changed.
  // "document-changed"
  //  the document has changed after the editing of something like
  //  subtitle, metadata... "style-changed" one or
  //  more styles have been changed.
  // "style-deleted"
  //  one or more styles have been removed.
  // "style-inserted"
  //  one or more styles have been created.
  sigc::signal<void> &get_signal(const std::string &name);

  // Emit a signal from its name.
  void emit_signal(const std::string &name);

  // Return the name of the current column focus.
  // (start, end, duration, text, translation ...)
  Glib::ustring get_current_column_name();

 protected:
  friend class Command;
  friend class Subtitle;
  friend class Subtitles;
  friend class SubtitleView;

  // Return the subtitle model.
  // A Gtk Model is used internally to avoid duplicate data.
  SubtitleModelPtr get_subtitle_model();

  // Return the (Gtk) subtitle view of the document.
  SubtitleViewPtr get_subtitle_view();

  // Create an attach the subtitle view of the document.
  void create_subtitle_view();

 protected:
  // Name of the document (ex: "toto.srt")
  Glib::ustring m_name;
  // Filename of the document (ex: "/home/john/toto.srt")
  Glib::ustring m_filename;
  // Subtitle format of the document
  Glib::ustring m_format;
  // Charset of the document (default "UTF-8")
  Glib::ustring m_charset;
  // Internally we always use the '\n'
  // this is only use for the export
  Glib::ustring m_newline;
  // (Internal/Model) timing mode of the subtitle
  TIMING_MODE m_timing_mode{TIME};
  // (External/View) timing mode used for the editing of the subtitle
  TIMING_MODE m_edit_timing_mode{TIME};
  // Framerate of the document
  FRAMERATE m_framerate{FRAMERATE_25};
  // Subtitles interface to modify the subtitle model (do not used directly the
  // SubtitleModel)
  Subtitles m_subtitles;
  // Styles interface to modify the style model (do not used directly the
  // StyleModel)
  Styles m_styles;
  // ScriptInfo attached to the document
  ScriptInfo m_scriptInfo;
  // StyleModel attached to the document
  Glib::RefPtr<StyleModel> m_styleModel;
  // SubtitleView attached to the document
  SubtitleView *m_subtitleView{nullptr};
  // SubtitleModel attached to the document
  Glib::RefPtr<SubtitleModel> m_subtitleModel;
  //
  bool m_document_changed{false};
  // list of signals ('document-changed', 'timing-mode-changed' ...)
  std::map<std::string, sigc::signal<void> > m_signal;
  // signal connector to display a message to the ui
  sigc::signal<void, Glib::ustring> m_signal_message;
  // signal connector to display a flash message (~3s) to the ui
  sigc::signal<void, Glib::ustring> m_signal_flash_message;
};
