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

#include "document.h"

#include <gtkmm.h>

#include <ctime>
#include <iostream>
#include <memory>

#include "cfg.h"
#include "documents.h"
#include "encodings.h"
#include "error.h"
#include "gui/comboboxencoding.h"
#include "gui/dialogutility.h"
#include "subtitleformatsystem.h"
#include "utility.h"

// Constructor
// The default values of the document are sets from the user config.
Document::Document(bool create_new)
    : CommandSystem(*this), m_subtitles(*this), m_styles(*this) {
  // sets default encoding value
  Glib::ustring default_encoding = cfg::get_string("encodings", "default");

  m_charset = (default_encoding.empty()) ? "UTF-8" : default_encoding;

  // sets default document format
  Glib::ustring default_format = cfg::get_string("document", "format");

  m_format = (SubtitleFormatSystem::instance().is_supported(default_format))
                 ? default_format
                 : "SubRip";

  // sets default newline
  Glib::ustring default_newline = cfg::get_string("document", "newline");

  m_newline = (default_newline.empty()) ? "Unix" : default_newline;

  // create models
  m_subtitleModel = Glib::RefPtr<SubtitleModel>(new SubtitleModel(this));
  m_styleModel = Glib::RefPtr<StyleModel>(new StyleModel);

  // Add default style for formats that support styles (ASS, SSA) for new files
  // FIXME Ideally individual documents would indicate they support it
  // so they would not need to be listed here
  if (create_new && (m_format == "Advanced Sub Station Alpha" ||
                     m_format == "Sub Station Alpha" ||
                     m_format == "Subtitle Editor Project")) {

    Style style = m_styles.append();
    style.set("name", "Default");
  }
  // m_nameModel = Glib::RefPtr<NameModel>(new NameModel);
  CommandSystem::signal_changed().connect(
      sigc::mem_fun(*this, &Document::make_document_changed));
}

// Constructor by copy
Document::Document(Document &src, bool copy_subtitles)
    : CommandSystem(*this), m_subtitles(*this), m_styles(*this) {
  m_timing_mode = src.m_timing_mode;
  m_edit_timing_mode = src.m_edit_timing_mode;
  m_framerate = src.m_framerate;
  m_subtitleModel = Glib::RefPtr<SubtitleModel>(new SubtitleModel(this));
  m_styleModel = Glib::RefPtr<StyleModel>(new StyleModel);

  g_return_if_fail(m_subtitleModel);
  g_return_if_fail(m_styleModel);

  m_format = src.m_format;
  m_charset = src.m_charset;
  m_newline = src.m_newline;

  m_scriptInfo = src.m_scriptInfo;

  m_name = src.m_name;
  m_filename = src.m_filename;

  // model
  if (copy_subtitles) {
    m_subtitleModel->copy(src.get_subtitle_model());
    m_styleModel->copy(src.get_style_model());
  }

  CommandSystem::signal_changed().connect(
      sigc::mem_fun(*this, &Document::make_document_changed));
}

// Destructor
Document::~Document() {
}

// Return the subtitle view widget (Gtk::TreeView)
Gtk::Widget *Document::widget() {
  return get_subtitle_view();
}

// Define the full filename of the document.
// ex: /home/toto/subtitle05.ass
// A signal "document-property-changed" is emitted.
void Document::setFilename(const Glib::ustring &filename) {
  m_filename = utility::create_full_path(filename);

  m_name = Glib::path_get_basename(m_filename);

  emit_signal("document-property-changed");
}

// Return the full filename of the document.
Glib::ustring Document::getFilename() {
  return m_filename;
}

// Define the name of the document.
// A signal "document-property-changed" is emitted.
void Document::setName(const Glib::ustring &name) {
  m_name = name;

  emit_signal("document-property-changed");
}

// Return the name of the document.
// If the fullname is "/home/toto/subtitle05.ass"
// then return "subtitle05.ass"
Glib::ustring Document::getName() {
  return m_name;
}

// Define the subtitle format of the document.
// (SubRip, MicroDVD...)
void Document::setFormat(const Glib::ustring &format) {
  m_format = format;
}

// Return the subtitle format of the document.
// (SubRip, MicroDVD...)
Glib::ustring Document::getFormat() {
  return m_format;
}

// Define the charset of the document.
void Document::setCharset(const Glib::ustring &charset) {
  m_charset = charset;
}

// Return the charset of the document.
Glib::ustring Document::getCharset() {
  return m_charset;
}

// Define the newline type of the document.
// Value can be "Unix", "Windows" or "Macintosh"
void Document::setNewLine(const Glib::ustring &name) {
  m_newline = name;
}

// Return the newline type of the document.
// Value can be "Unix", "Windows" or "Macintosh"
Glib::ustring Document::getNewLine() {
  return m_newline;
}

// Try to open a file from an uri.
// The document charset is used to open the file.
// Prefer the function create_from_file for create a new document.
// Launch an Exception if it fails.
// Exceptions: UnrecognizeFormatError, EncodingConvertError, IOFileError,
// Glib::Error...
// FIXME: Remove this function
void Document::open(const Glib::ustring &uri) {
  SubtitleFormatSystem::instance().open_from_uri(this, uri, getCharset());
}

// Try to save the document to the file.
// The format, charset and newline used are the document values.
// The document name will be renamed from the uri.
// An error dialog will be display if needed.
// Return true if it succeeds or false.
bool Document::save(const Glib::ustring &uri) {
  Glib::ustring basename =
      Glib::path_get_basename(Glib::filename_from_uri(uri));
  Glib::ustring format = getFormat();
  Glib::ustring charset = getCharset();
  Glib::ustring newline = getNewLine();

  try {
    SubtitleFormatSystem::instance().save_to_uri(this, uri, format, charset,
                                                 newline);
    return true;
  } catch (const EncodingConvertError &ex) {
    Glib::ustring title = build_message(
        _("Could not save the file \"%s\" using the character coding %s."),
        basename.c_str(), Encodings::get_label_from_charset(charset).c_str());
    Glib::ustring msg =
        _("The document contains one or more characters "
          "that cannot be encoded using the specified character coding.");

    ErrorDialog dialog(title, msg);
    dialog.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
    dialog.run();
  } catch (const std::exception &ex) {
    dialog_error(_("Save Document Failed."), ex.what());
  } catch (const Glib::Exception &ex) {
    dialog_error(_("Save Document Failed."), ex.what());
  }
  return false;
}

// Return the subtitle model.
// A Gtk Model is used internally to avoid duplicate data.
Glib::RefPtr<SubtitleModel> Document::get_subtitle_model() {
  return m_subtitleModel;
}

// Return the StyleModel of the document.
Glib::RefPtr<StyleModel> Document::get_style_model() {
  return m_styleModel;
}

// FIXME (Need to be fixed)
// Return the ScriptInfo of the document.
ScriptInfo &Document::get_script_info() {
  return m_scriptInfo;
}

// Return the (Gtk) subtitle view of the document.
SubtitleView *Document::get_subtitle_view() {
  if (m_subtitleView == nullptr)
    create_subtitle_view();

  return m_subtitleView;
}

// Create an attach the subtitle view of the document.
void Document::create_subtitle_view() {
  se_dbg(SE_DBG_APP);

  m_subtitleView = manage(new SubtitleView(*this));
  m_subtitleView->show();
}

// Display a message to the user. (statusbar)
void Document::message(const gchar *format, ...) {
  va_list args;
  gchar *formatted = NULL;

  va_start(args, format);
  formatted = g_strdup_vprintf(format, args);
  va_end(args);

  se_dbg_msg(SE_DBG_APP, formatted);

  m_signal_message(formatted);

  g_free(formatted);
}

// Signal connector to received message from the document.
sigc::signal<void, Glib::ustring> &Document::get_signal_message() {
  return m_signal_message;
}

// Display a flash message (3 seconds) to the user. (statusbar)
void Document::flash_message(const gchar *format, ...) {
  va_list args;
  gchar *formatted = NULL;

  va_start(args, format);
  formatted = g_strdup_vprintf(format, args);
  va_end(args);

  se_dbg_msg(SE_DBG_APP, formatted);

  m_signal_flash_message(formatted);

  g_free(formatted);
}

// Signal connector to received flash message from the document.
sigc::signal<void, Glib::ustring> &Document::get_signal_flash_message() {
  return m_signal_flash_message;
}

// Return a Subtitles manager of the document.
Subtitles Document::subtitles() {
  return m_subtitles;
}

// Return a Styles manager of the document.
Styles Document::styles() {
  return m_styles;
}

// Command System

void Document::start_command(const Glib::ustring &description) {
  CommandSystem::start(description);
}

void Document::add_command(Command *cmd) {
  CommandSystem::add(cmd);
}

void Document::finish_command() {
  if (CommandSystem::is_recording()) {
    CommandSystem::finish();

    make_document_changed();
  }
}

CommandSystem &Document::get_command_system() {
  return *this;
}

// The document has changed (start_command and finish_command are used)
// after save the document toggle state of false
// the signal "document-changed" is used after any change
bool Document::get_document_changed() {
  return m_document_changed;
}

// Turn m_document_changed to true and emit a signal "document-changed"
void Document::make_document_changed() {
  m_document_changed = true;

  emit_signal("document-changed");
}

// Turn m_document_changed to false and emit a signal "document-changed"
void Document::make_document_unchanged() {
  m_document_changed = false;

  emit_signal("document-changed");
}

// Define the timing mode of the document.
// This is the internal timing mode (frame or time) used
// to represent subtitle.
// A signal "timing-mode-changed" is emitted.
void Document::set_timing_mode(TIMING_MODE mode) {
  m_timing_mode = mode;

  emit_signal("timing-mode-changed");
}

// Return the timing mode of the document.
// This is the internal timing mode (frame or time) used
// to represent subtitle.
TIMING_MODE Document::get_timing_mode() {
  return m_timing_mode;
}

// Define the editing timing mode of the document.
// A signal "edit-timing-mode-changed" is emitted.
void Document::set_edit_timing_mode(TIMING_MODE mode) {
  m_edit_timing_mode = mode;
  emit_signal("edit-timing-mode-changed");
}

// Return the editing timing mode of the document.
TIMING_MODE Document::get_edit_timing_mode() {
  return m_edit_timing_mode;
}

// Define the framerate of the document.
// A signal "framerate-changed" is emitted.
void Document::set_framerate(FRAMERATE framerate) {
  m_framerate = framerate;
  emit_signal("framerate-changed");
}

// Return the framerate of the document.
FRAMERATE Document::get_framerate() {
  return m_framerate;
}

// Create a new document from an uri, if the charset is empty then it will try
// to auto detect the good value. This function display a dialog ask or error
// if needed. Return a new document or NULL.
Document *Document::create_from_file(const Glib::ustring &uri,
                                     const Glib::ustring &charset) {
  se_dbg_msg(SE_DBG_APP, "uri=%s charset=%s", uri.c_str(), charset.c_str());

  Glib::ustring filename = Glib::filename_from_uri(uri);
  Glib::ustring basename = Glib::path_get_basename(filename);
  Glib::ustring label_charset = Encodings::get_label_from_charset(charset);

  try {
    Document *doc = new Document(false);
    doc->setCharset(charset);
    doc->open(uri);
    return doc;
  } catch (const UnrecognizeFormatError &ex) {
    Glib::ustring title = build_message(
        _("Could not recognize the subtitle format for the file \"%s\"."),
        basename.c_str());
    Glib::ustring msg =
        _("Please check that the file contains subtitles in a supported "
          "format.");

    ErrorDialog dialog(title, msg);
    dialog.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
    dialog.run();
  } catch (const EncodingConvertError &ex) {
    Glib::ustring title, msg;

    if (charset.empty()) {
      title = build_message(_("Could not open automatically the file \"%s\"."),
                            basename.c_str());
      msg =
          _("Subtitle Editor was not able to automatically determine the file "
            "encoding. "
            "Select a different character coding from the menu and try again.");
    } else {
      title = build_message(
          _("Could not open the file \"%s\" using the character coding %s."),
          basename.c_str(), Encodings::get_label_from_charset(charset).c_str());
      msg =
          _("Select a different character coding from the menu and try again.");
    }

    ErrorDialog dialog(title, msg);
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

    Gtk::Label labelEncoding(_("Character Coding:"), 1.0, 0.5);
    ComboBoxEncoding comboEncoding(false);

    Gtk::HBox hbox(false, 6);
    dialog.get_vbox()->pack_start(hbox, false, false);
    hbox.pack_start(labelEncoding);
    hbox.pack_start(comboEncoding);

    dialog.show_all();
    if (dialog.run() == Gtk::RESPONSE_OK) {
      dialog.hide();

      return Document::create_from_file(uri, comboEncoding.get_value());
    }
  } catch (const std::exception &ex) {
    Glib::ustring title =
        build_message(_("Could not open the file \"%s\""), basename.c_str());
    Glib::ustring msg = ex.what();

    ErrorDialog dialog(title, msg);
    dialog.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
    dialog.run();
  } catch (const Glib::Exception &ex) {
    Glib::ustring title =
        build_message(_("Could not open the file \"%s\""), basename.c_str());
    Glib::ustring msg = ex.what();

    ErrorDialog dialog(title, msg);
    dialog.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
    dialog.run();
  } catch (...) {
    Glib::ustring title =
        build_message(_("Could not open the file \"%s\""), basename.c_str());
    Glib::ustring msg = _("An unknown error occurred while opening the file.");

    ErrorDialog dialog(title, msg);
    dialog.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
    dialog.run();
  }
  return nullptr;
}

// Return a signal connector from his name.
sigc::signal<void> &Document::get_signal(const std::string &name) {
  return m_signal[name];
}

// Emit a signal from his name.
void Document::emit_signal(const std::string &name) {
  se_dbg_msg(SE_DBG_APP, "signal named '%s'", name.c_str());

  m_signal[name].emit();

  se::documents::signal_modified().emit(this, name);
}

// Return the name of the current column focus.
// (start, end, duration, text, translation ...)
Glib::ustring Document::get_current_column_name() {
  return m_subtitleView->get_current_column_name();
}
