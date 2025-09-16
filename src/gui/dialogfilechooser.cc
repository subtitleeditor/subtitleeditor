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

#include "comboboxencoding.h"
#include "comboboxnewline.h"
#include "comboboxsubtitleformat.h"
#include "comboboxvideo.h"
#include "dialogcharactercodings.h"
#include "dialogfilechooser.h"
#include "gtkmm_utility.h"
#include "subtitleformatsystem.h"
#include "utility.h"

// Init dialog filter with from SubtitleFormatSystem.
void init_dialog_subtitle_filters(Gtk::FileChooserDialog *dialog) {
  g_return_if_fail(dialog);

  auto subtitle_format_infos = SubtitleFormatSystem::instance().get_infos();

  Glib::RefPtr<Gtk::FileFilter> all = Gtk::FileFilter::create();
  Glib::RefPtr<Gtk::FileFilter> supported = Gtk::FileFilter::create();
  // all files
  {
    all->set_name(_("All files (*.*)"));
    all->add_pattern("*");
    dialog->add_filter(all);
  }

  // all supported formats
  supported->set_name(_("All supported formats (*.ass, *.ssa, *.srt, ...)"));
  for (const auto &sf_info : subtitle_format_infos) {
    supported->add_pattern("*." + sf_info.extension);
    supported->add_pattern("*." + sf_info.extension.uppercase());
  }
  dialog->add_filter(supported);

  // by format
  for (const auto &sf_info : subtitle_format_infos) {
    Glib::ustring name = sf_info.name;
    Glib::ustring ext = sf_info.extension;

    Glib::RefPtr<Gtk::FileFilter> filter = Gtk::FileFilter::create();
    filter->set_name(name + " (" + ext + ")");
    filter->add_pattern("*." + ext);
    filter->add_pattern("*." + ext.uppercase());
    dialog->add_filter(filter);
  }

  // select by default
  dialog->set_filter(supported);
}

// DialogFileChooser

DialogFileChooser::DialogFileChooser(BaseObjectType *cobject,
                                     const Glib::ustring &name)
    : Gtk::FileChooserDialog(cobject), m_name(name) {
  if (cfg::has_key("dialog-last-folder", m_name)) {
    set_current_folder_uri(cfg::get_string("dialog-last-folder", m_name));
  }
  utility::set_transient_parent(*this);
}

DialogFileChooser::DialogFileChooser(const Glib::ustring &title,
                                     Gtk::FileChooserAction action,
                                     const Glib::ustring &name)
    : Gtk::FileChooserDialog(title, action), m_name(name) {
  if (cfg::has_key("dialog-last-folder", m_name)) {
    set_current_folder_uri(cfg::get_string("dialog-last-folder", m_name));
  }
  utility::set_transient_parent(*this);
}

DialogFileChooser::~DialogFileChooser() {
  cfg::set_string("dialog-last-folder", m_name, get_current_folder_uri());
}

// Define the current file filter.
// ex: 'Subtitle Editor Project', 'SubRip', 'MicroDVD' ...
void DialogFileChooser::set_current_filter(const Glib::ustring &sf_name) {
  for (const auto &filter : list_filters()) {
    if (filter->get_name().find(sf_name) == Glib::ustring::npos)
      continue;
    set_filter(filter);
    return;
  }
}

// This can be use to setup the document name based on video uri
void DialogFileChooser::set_filename_from_another_uri(
    const Glib::ustring &another_uri, const Glib::ustring &ext) {
  try {
    Glib::ustring filename = Glib::filename_from_uri(another_uri);
    Glib::ustring pathname = Glib::path_get_dirname(filename);
    Glib::ustring basename = Glib::path_get_basename(filename);

    basename = utility::add_or_replace_extension(basename, ext);

    set_current_folder(pathname);  // set_current_folder_uri ?
    set_current_name(basename);
  } catch (const Glib::Exception &ex) {
    std::cerr << "set_filename_from_another_uri failed : " << ex.what()
              << std::endl;
  }
}

// Internally call set_current_folder and set_current_name with dirname and
// basename
void DialogFileChooser::set_current_folder_and_name(
    const Glib::ustring &filename) {
  set_current_folder(Glib::path_get_dirname(filename));
  set_current_name(Glib::path_get_basename(filename));
}

// DialogOpenDocument
// Dialog open file chooser with Encoding and Video options.

// Constructor
DialogOpenDocument::DialogOpenDocument(
    BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &builder)
    : DialogFileChooser(cobject, "dialog-open-document") {
  builder->get_widget_derived("combobox-encodings", m_comboEncodings);
  builder->get_widget("label-video", m_labelVideo);
  builder->get_widget_derived("combobox-video", m_comboVideo);

  signal_current_folder_changed().connect(
      sigc::mem_fun(*this, &DialogOpenDocument::on_current_folder_changed));

  signal_selection_changed().connect(
      sigc::mem_fun(*this, &DialogOpenDocument::on_selection_changed));

  init_dialog_subtitle_filters(this);

  add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

  set_default_response(Gtk::RESPONSE_OK);
}

// Returns the encoding value.
// Charset or empty string (Auto Detected)
Glib::ustring DialogOpenDocument::get_encoding() const {
  return m_comboEncodings->get_value();
}

// Returns the video uri or empty string.
Glib::ustring DialogOpenDocument::get_video_uri() const {
  Glib::ustring video = m_comboVideo->get_value();
  if (video.empty())
    return Glib::ustring();

  return Glib::build_filename(get_current_folder_uri(), video);
}

void DialogOpenDocument::show_video(bool state) {
  if (state) {
    m_labelVideo->show();
    m_comboVideo->show();
  } else {
    m_labelVideo->hide();
    m_comboVideo->hide();
  }
}

// Create a instance of the dialog.
DialogOpenDocument::unique_ptr DialogOpenDocument::create() {
  unique_ptr ptr(gtkmm_utility::get_widget_derived<DialogOpenDocument>(
      SE_DEV_VALUE(PACKAGE_UI_DIR, PACKAGE_UI_DIR_DEV),
      "dialog-open-document.ui", "dialog-open-document"));

  return ptr;
}

// The current folder has changed, need to update the ComboBox Video
void DialogOpenDocument::on_current_folder_changed() {
  m_comboVideo->set_current_folder(get_current_folder());
}

// The file selection has changed, need to update the ComboBox Video
void DialogOpenDocument::on_selection_changed() {
  std::vector<std::string> selected = get_filenames();

  if (selected.size() == 1)
    m_comboVideo->auto_select_video(selected.front());
  else
    m_comboVideo->auto_select_video("");
}

// DialogSaveDocument
// Dialog save file chooser with Format, Encoding and NewLine options.

// Constructor
DialogSaveDocument::DialogSaveDocument(
    BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &builder)
    : DialogFileChooser(cobject, "dialog-save-document") {
  builder->get_widget_derived("combobox-format", m_comboFormat);
  builder->get_widget_derived("combobox-encodings", m_comboEncodings);
  builder->get_widget_derived("combobox-newline", m_comboNewLine);

  init_dialog_subtitle_filters(this);

  m_comboEncodings->show_auto_detected(false);

  add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

  set_default_response(Gtk::RESPONSE_OK);

  m_comboFormat->signal_changed().connect(
      sigc::mem_fun(*this, &DialogSaveDocument::on_combo_format_changed));
}

void DialogSaveDocument::set_format(const Glib::ustring &format) {
  m_comboFormat->set_value(format);
}

// Returns the subtitle format value.
Glib::ustring DialogSaveDocument::get_format() const {
  return m_comboFormat->get_value();
}

void DialogSaveDocument::set_encoding(const Glib::ustring &encoding) {
  m_comboEncodings->set_value(encoding);
}

// Returns the encoding value or empty string (Auto Detected).
Glib::ustring DialogSaveDocument::get_encoding() const {
  return m_comboEncodings->get_value();
}

void DialogSaveDocument::set_newline(const Glib::ustring &newline) {
  m_comboNewLine->set_value(newline);
}

// Return the newline value.
// Windows or Unix.
Glib::ustring DialogSaveDocument::get_newline() const {
  return m_comboNewLine->get_value();
}

// Update the extension of the current filename.
void DialogSaveDocument::on_combo_format_changed() {
  Glib::ustring basename = get_current_name();

  if (basename.empty())
    return;

  // Try to get the extension from the format
  SubtitleFormatInfo sfinfo;
  if (SubtitleFormatSystem::instance().get_info(get_format(), sfinfo) == false)
    return;
  // Change the extension according to the format selected
  basename = utility::add_or_replace_extension(basename, sfinfo.extension);
  // Update only the current name
  set_current_name(basename);
}

// Create a instance of the dialog.
DialogSaveDocument::unique_ptr DialogSaveDocument::create() {
  unique_ptr ptr(gtkmm_utility::get_widget_derived<DialogSaveDocument>(
      SE_DEV_VALUE(PACKAGE_UI_DIR, PACKAGE_UI_DIR_DEV),
      "dialog-save-document.ui", "dialog-save-document"));

  return ptr;
}

// DialogImportText
// Dialog open file chooser with Encoding option.

// Constructor
DialogImportText::DialogImportText(BaseObjectType *cobject,
                                   const Glib::RefPtr<Gtk::Builder> &builder)
    : DialogFileChooser(cobject, "dialog-import-text") {
  builder->get_widget_derived("combobox-encodings", m_comboEncodings);
  builder->get_widget("checkbutton-blank-lines", m_checkBlankLines);
  widget_config::read_config_and_connect(m_checkBlankLines, "plain-text",
                                         "import-bl-between-subtitles");

  add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

  set_default_response(Gtk::RESPONSE_OK);
}

// Returns the encoding value.
// Charset or empty string (Auto Detected)
Glib::ustring DialogImportText::get_encoding() const {
  return m_comboEncodings->get_value();
}

// Returns whether blank lines separate subtitles
bool DialogImportText::get_blank_line_mode() const {
  return m_checkBlankLines->get_active();
}

// Create a instance of the dialog.
DialogImportText::unique_ptr DialogImportText::create() {
  unique_ptr ptr(gtkmm_utility::get_widget_derived<DialogImportText>(
      SE_DEV_VALUE(PACKAGE_UI_DIR, PACKAGE_UI_DIR_DEV), "dialog-import-text.ui",
      "dialog-import-text"));

  return ptr;
}

// DialogExportText
// Dialog save file chooser with Encoding and NewLine options.

// Constructor
DialogExportText::DialogExportText(BaseObjectType *cobject,
                                   const Glib::RefPtr<Gtk::Builder> &builder)
    : DialogFileChooser(cobject, "dialog-export-text") {
  builder->get_widget_derived("combobox-encodings", m_comboEncodings);
  builder->get_widget_derived("combobox-newline", m_comboNewLine);
  builder->get_widget("checkbutton-blank-lines", m_checkBlankLines);
  widget_config::read_config_and_connect(m_checkBlankLines, "plain-text",
                                         "export-bl-between-subtitles");

  m_comboEncodings->show_auto_detected(false);

  add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

  set_default_response(Gtk::RESPONSE_OK);
}

// Returns the encoding value or empty string (Auto Detected).
Glib::ustring DialogExportText::get_encoding() const {
  return m_comboEncodings->get_value();
}

// Return the newline value.
// Windows or Unix.
Glib::ustring DialogExportText::get_newline() const {
  return m_comboNewLine->get_value();
}

// Returns whether subtitles should be separated by blank lines
bool DialogExportText::get_blank_line_mode() const {
  return m_checkBlankLines->get_active();
}

// Create a instance of the dialog.
DialogExportText::unique_ptr DialogExportText::create() {
  unique_ptr ptr(gtkmm_utility::get_widget_derived<DialogExportText>(
      SE_DEV_VALUE(PACKAGE_UI_DIR, PACKAGE_UI_DIR_DEV), "dialog-export-text.ui",
      "dialog-export-text"));

  return ptr;
}

// Open Movie
DialogOpenVideo::DialogOpenVideo()
    : Gtk::FileChooserDialog(_("Open Video"), Gtk::FILE_CHOOSER_ACTION_OPEN) {
  utility::set_transient_parent(*this);

  // audio or video filter
  Glib::RefPtr<Gtk::FileFilter> m_filterAudioOrVideo = Gtk::FileFilter::create();
  m_filterAudioOrVideo->set_name(_("Audio or Video"));
  m_filterAudioOrVideo->add_pattern("*.mp3");
  m_filterAudioOrVideo->add_pattern("*.ogg");
  m_filterAudioOrVideo->add_pattern("*.wav");
  m_filterAudioOrVideo->add_pattern("*.avi");
  m_filterAudioOrVideo->add_pattern("*.wma");
  m_filterAudioOrVideo->add_pattern("*.mkv");
  m_filterAudioOrVideo->add_pattern("*.mpg");
  m_filterAudioOrVideo->add_pattern("*.mpeg");
  m_filterAudioOrVideo->add_mime_type("video/*");
  m_filterAudioOrVideo->add_mime_type("audio/*");
  add_filter(m_filterAudioOrVideo);

  // audio filter
  Glib::RefPtr<Gtk::FileFilter> m_filterAudio = Gtk::FileFilter::create();
  m_filterAudio->set_name(_("Audio"));
  m_filterAudio->add_pattern("*.mp3");
  m_filterAudio->add_pattern("*.ogg");
  m_filterAudio->add_pattern("*.wav");
  m_filterAudio->add_mime_type("audio/*");
  add_filter(m_filterAudio);

  // video filter
  Glib::RefPtr<Gtk::FileFilter> m_filterVideo = Gtk::FileFilter::create();
  m_filterVideo->set_name(_("Video"));
  m_filterVideo->add_pattern("*.avi");
  m_filterVideo->add_pattern("*.wma");
  m_filterVideo->add_pattern("*.mkv");
  m_filterVideo->add_pattern("*.mpg");
  m_filterVideo->add_pattern("*.mpeg");
  m_filterVideo->add_mime_type("video/*");
  add_filter(m_filterVideo);


  Glib::RefPtr<Gtk::FileFilter> m_filterAll = Gtk::FileFilter::create();
  m_filterAll->set_name(_("ALL"));
  m_filterAll->add_pattern("*.*");
  add_filter(m_filterAll);

  add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
  set_default_response(Gtk::RESPONSE_OK);

  if (cfg::has_key("dialog-last-folder", "dialog-open-video")) {
    set_current_folder_uri(
        cfg::get_string("dialog-last-folder", "dialog-open-video"));
  }
}

DialogOpenVideo::~DialogOpenVideo() {
  Glib::ustring floder = get_current_folder_uri();

  cfg::set_string("dialog-last-folder", "dialog-open-video", floder);
}

// Waveform or Audio/Video
DialogOpenWaveform::DialogOpenWaveform()
    : Gtk::FileChooserDialog(_("Open Waveform"),
                             Gtk::FILE_CHOOSER_ACTION_OPEN) {
  utility::set_transient_parent(*this);

  // waveform, video and audio filter
  Glib::RefPtr<Gtk::FileFilter> m_filterSupported = Gtk::FileFilter::create();
  m_filterSupported->set_name(_("Waveform & Media"));
  m_filterSupported->add_pattern("*.wf");
  m_filterSupported->add_mime_type("video/*");
  m_filterSupported->add_pattern("*.avi");
  m_filterSupported->add_pattern("*.wma");
  m_filterSupported->add_pattern("*.mkv");
  m_filterSupported->add_pattern("*.mpg");
  m_filterSupported->add_pattern("*.mpeg");
  m_filterSupported->add_mime_type("audio/*");
  m_filterSupported->add_pattern("*.mp3");
  m_filterSupported->add_pattern("*.ogg");
  m_filterSupported->add_pattern("*.wav");
  add_filter(m_filterSupported);

  // waveform filter
  Glib::RefPtr<Gtk::FileFilter> m_filterWaveform = Gtk::FileFilter::create();
  m_filterWaveform->set_name(_("Waveform (*.wf)"));
  m_filterWaveform->add_pattern("*.wf");
  add_filter(m_filterWaveform);

  // movies filter
  Glib::RefPtr<Gtk::FileFilter> m_filterMovie = Gtk::FileFilter::create();
  m_filterMovie->set_name(_("Video"));
  m_filterMovie->add_pattern("*.avi");
  m_filterMovie->add_pattern("*.wma");
  m_filterMovie->add_pattern("*.mkv");
  m_filterMovie->add_pattern("*.mpg");
  m_filterMovie->add_pattern("*.mpeg");
  m_filterMovie->add_mime_type("video/*");
  add_filter(m_filterMovie);

  // audio filter
  Glib::RefPtr<Gtk::FileFilter> m_filterAudio = Gtk::FileFilter::create();
  m_filterAudio->set_name(_("Audio"));
  m_filterAudio->add_pattern("*.mp3");
  m_filterAudio->add_pattern("*.ogg");
  m_filterAudio->add_pattern("*.wav");
  m_filterAudio->add_mime_type("audio/*");
  add_filter(m_filterAudio);

  // all filter
  Glib::RefPtr<Gtk::FileFilter> m_filterAll = Gtk::FileFilter::create();
  m_filterAll->set_name(_("ALL"));
  m_filterAll->add_pattern("*.*");
  add_filter(m_filterAll);

  add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
  set_default_response(Gtk::RESPONSE_OK);

  if (cfg::has_key("dialog-last-folder", "dialog-open-waveform")) {
    set_current_folder_uri(
        cfg::get_string("dialog-last-folder", "dialog-open-waveform"));
  }
}

DialogOpenWaveform::~DialogOpenWaveform() {
  auto floder = get_current_folder_uri();
  cfg::set_string("dialog-last-folder", "dialog-open-waveform", floder);
}

// Keyframes or Video
DialogOpenKeyframe::DialogOpenKeyframe()
    : Gtk::FileChooserDialog(_("Open Keyframe"),
                             Gtk::FILE_CHOOSER_ACTION_OPEN) {
  utility::set_transient_parent(*this);

  // keyframes and video filter
  Glib::RefPtr<Gtk::FileFilter> m_filterSupported = Gtk::FileFilter::create();
  m_filterSupported->set_name(_("Keyframe & Media"));
  m_filterSupported->add_pattern("*.kf");
  m_filterSupported->add_mime_type("video/*");
  m_filterSupported->add_pattern("*.avi");
  m_filterSupported->add_pattern("*.wma");
  m_filterSupported->add_pattern("*.mkv");
  m_filterSupported->add_pattern("*.mpg");
  m_filterSupported->add_pattern("*.mpeg");
  add_filter(m_filterSupported);

  // keyframe filter
  Glib::RefPtr<Gtk::FileFilter> m_filterKeyframe = Gtk::FileFilter::create();
  m_filterKeyframe->set_name(_("Keyframe (*.kf)"));
  m_filterKeyframe->add_pattern("*.kf");
  add_filter(m_filterKeyframe);

  // movies filter
  Glib::RefPtr<Gtk::FileFilter> m_filterMovie = Gtk::FileFilter::create();
  m_filterMovie->set_name(_("Video"));
  m_filterMovie->add_pattern("*.avi");
  m_filterMovie->add_pattern("*.wma");
  m_filterMovie->add_pattern("*.mkv");
  m_filterMovie->add_pattern("*.mpg");
  m_filterMovie->add_pattern("*.mpeg");
  m_filterMovie->add_mime_type("video/*");
  add_filter(m_filterMovie);

  // all filter
  Glib::RefPtr<Gtk::FileFilter> m_filterAll = Gtk::FileFilter::create();
  m_filterAll->set_name(_("ALL"));
  m_filterAll->add_pattern("*.*");
  add_filter(m_filterAll);

  add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
  set_default_response(Gtk::RESPONSE_OK);

  if (cfg::has_key("dialog-last-folder", "dialog-open-keyframe")) {
    set_current_folder_uri(
        cfg::get_string("dialog-last-folder", "dialog-open-keyframe"));
  }
}

DialogOpenKeyframe::~DialogOpenKeyframe() {
  auto floder = get_current_folder_uri();
  cfg::set_string("dialog-last-folder", "dialog-open-keyframe", floder);
}
