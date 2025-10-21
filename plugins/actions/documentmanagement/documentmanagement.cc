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

#include <documents.h>
#include <extension/action.h>
#include <gui/dialogfilechooser.h>
#include <player.h>
#include <subtitleformatsystem.h>
#include <utility.h>

#include <vector>

class DialogAskToSaveOnExit : public Gtk::MessageDialog {
 public:
  DialogAskToSaveOnExit()
      : Gtk::MessageDialog("", false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_NONE) {
    utility::set_transient_parent(*this);

    add_button(_("Close _without Saving"), Gtk::RESPONSE_NO);
    add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_YES);
  }

  int run(Document *doc) {
    Glib::ustring document_name = doc->getName();
    Glib::ustring primary_text =
        build_message(_("Save the changes to document \"%s\" before closing?"),
                      document_name.c_str());
    Glib::ustring secondary_text =
        _("If you don't save, the last changes will be permanently lost.");

    set_message(primary_text);
    set_secondary_text(secondary_text);

    return Gtk::Dialog::run();
  }
};

class DocumentManagementPlugin : public Action {
 public:
  DocumentManagementPlugin() {
    activate();
    update_ui();
  }

  ~DocumentManagementPlugin() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("DocumentManagementPlugin");

    // new document
    action_group->add(Gtk::Action::create("new-document", Gtk::Stock::NEW, "",
                                          _("Create a new document")),
                      sigc::mem_fun(*this, &DocumentManagementPlugin::on_new));

    // open & save document
    action_group->add(Gtk::Action::create("open-document", Gtk::Stock::OPEN, "",
                                          _("Open a file")),
                      sigc::mem_fun(*this, &DocumentManagementPlugin::on_open));

    action_group->add(
        Gtk::Action::create("open-project", _("Open Project"),
                            _("Open a Subtitle Editor Project")),
        sigc::mem_fun(*this, &DocumentManagementPlugin::on_open_project));
#if GTKMM_CHECK_VERSION(2, 16, 0)
    action_group->get_action("open-project")->set_stock_id(Gtk::Stock::OPEN);
#endif  // GTKMM_CHECK_VERSION(2,16,0)

    action_group->add(Gtk::Action::create("save-document", Gtk::Stock::SAVE, "",
                                          _("Save the current file")),
                      Gtk::AccelKey("<Control>S"),
                      sigc::mem_fun(*this, &DocumentManagementPlugin::on_save));

    action_group->add(
        Gtk::Action::create(
            "save-project", _("Save Project"),
            _("Save the current file as Subtitle Editor Project")),
        sigc::mem_fun(*this, &DocumentManagementPlugin::on_save_project));
#if GTKMM_CHECK_VERSION(2, 16, 0)
    action_group->get_action("save-project")->set_stock_id(Gtk::Stock::SAVE);
#endif  // GTKMM_CHECK_VERSION(2,16,0)

    action_group->add(
        Gtk::Action::create("save-as-document", Gtk::Stock::SAVE_AS, "",
                            _("Save the current file with a different name")),
        Gtk::AccelKey("<Shift><Control>S"),
        sigc::mem_fun(*this, &DocumentManagementPlugin::on_save_as));

    action_group->add(
        Gtk::Action::create("save-all-documents", Gtk::Stock::SAVE_AS,
                            _("Save _All"), _("Save all open files")),
        sigc::mem_fun(*this, &DocumentManagementPlugin::on_save_all_documents));

    // open & save translation
    action_group->add(
        Gtk::Action::create("open-translation", Gtk::Stock::OPEN,
                            _("Open _Translation"),
                            _("Open translation from file")),
        Gtk::AccelKey("<Control>T"),
        sigc::mem_fun(*this, &DocumentManagementPlugin::on_open_translation));

    action_group->add(
        Gtk::Action::create("save-translation", Gtk::Stock::SAVE,
                            _("Save Trans_lation"),
                            _("Save translation to file")),
        Gtk::AccelKey("<Shift><Control>T"),
        sigc::mem_fun(*this, &DocumentManagementPlugin::on_save_translation));

    // recent files
    Glib::RefPtr<Gtk::RecentAction> recentAction = Gtk::RecentAction::create(
        "menu-recent-open-document", _("Open _Recent"));

    Glib::RefPtr<Gtk::RecentFilter> filter = Gtk::RecentFilter::create();
    filter->set_name("subtitleeditor");
    filter->add_group("subtitleeditor");
    recentAction->set_filter(filter);

    recentAction->set_show_icons(false);
    recentAction->set_show_numbers(true);
    recentAction->set_show_tips(true);
    // recentAction->set_show_not_found(false);
    recentAction->set_sort_type(Gtk::RECENT_SORT_MRU);

    recentAction->signal_item_activated().connect(sigc::mem_fun(
        *this, &DocumentManagementPlugin::on_recent_item_activated));
    action_group->add(recentAction);

    // close
    action_group->add(
        Gtk::Action::create("close-document", Gtk::Stock::CLOSE, "",
                            _("Close the current file")),
        sigc::mem_fun(*this, &DocumentManagementPlugin::on_close));

    // quit the program
    action_group->add(Gtk::Action::create("exit", Gtk::Stock::QUIT, _("E_xit"),
                                          _("Quit the program")),
                      sigc::mem_fun(*this, &DocumentManagementPlugin::on_exit));

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui->insert_action_group(action_group);

    se::documents::signal_created().connect(
        sigc::mem_fun(*this, &DocumentManagementPlugin::on_document_create));

    Gtk::Window *window =
        dynamic_cast<Gtk::Window *>(get_subtitleeditor_window());
    if (window)
      window->signal_delete_event().connect(sigc::mem_fun(
          *this,
          &DocumentManagementPlugin::on_subtitleeditor_window_delete_event));

    // config connection
    m_config_interface_connection =
        cfg::signal_changed("interface")
            .connect(sigc::mem_fun(
                *this, &DocumentManagementPlugin::on_config_interface_changed));

    init_autosave();

    ui_id = ui->new_merge_id();

#define ADD_UI(name)                                                      \
  ui->add_ui(ui_id,                                                       \
             Glib::ustring::compose("%1/%2", "/menubar/menu-file", name), \
             name, name);
#define ADD_OPEN_UI(name)                                                    \
  ui->add_ui(                                                                \
      ui_id,                                                                 \
      Glib::ustring::compose("%1/%2", "/menubar/menu-file/menu-open", name), \
      name, name);
#define ADD_SAVE_UI(name)                                                    \
  ui->add_ui(                                                                \
      ui_id,                                                                 \
      Glib::ustring::compose("%1/%2", "/menubar/menu-file/menu-save", name), \
      name, name);

    ADD_UI("new-document");
    ADD_OPEN_UI("open-document");
    ADD_OPEN_UI("open-project");
    ADD_OPEN_UI("open-translation");
    ADD_UI("menu-recent-open-document");
    ADD_SAVE_UI("save-document");
    ADD_SAVE_UI("save-project");
    ADD_SAVE_UI("save-as-document");
    ADD_SAVE_UI("save-all-documents");
    ADD_SAVE_UI("save-translation");
    ADD_UI("close-document");
    ADD_UI("exit");

#undef ADD_UI
#undef ADD_OPEN_UI
#undef ADD_SAVE_UI
  }

  void deactivate() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui->remove_ui(ui_id);
    ui->remove_action_group(action_group);

    m_config_interface_connection.disconnect();
    m_autosave_timeout.disconnect();
  }

  void update_ui() {
    se_dbg(SE_DBG_PLUGINS);

    bool visible = (get_current_document() != NULL);

    action_group->get_action("open-translation")->set_sensitive(visible);
    action_group->get_action("save-document")->set_sensitive(visible);
    action_group->get_action("save-project")->set_sensitive(visible);
    action_group->get_action("save-as-document")->set_sensitive(visible);
    action_group->get_action("save-all-documents")->set_sensitive(visible);
    action_group->get_action("save-translation")->set_sensitive(visible);
    action_group->get_action("close-document")->set_sensitive(visible);
  }

 protected:
  // Create a new document with unique name
  void on_new() {
    se_dbg(SE_DBG_PLUGINS);

    Document *doc = new Document();

    g_return_if_fail(doc);

    Glib::ustring ext =
        SubtitleFormatSystem::instance().get_extension_of_format(
            doc->getFormat());
    doc->setFilename(se::documents::generate_untitled_name(ext));

    se::documents::append(doc);
  }

  void on_open() {
    open_filechooser();
  }

  void on_open_project() {
    open_filechooser("Subtitle Editor Project");
  }

  // Launch a filechooser dialog and open a document
  void open_filechooser(const Glib::ustring &filterformat = Glib::ustring()) {
    se_dbg(SE_DBG_PLUGINS);

    DialogOpenDocument::unique_ptr dialog = DialogOpenDocument::create();

    if (!filterformat.empty())
      dialog->set_current_filter(filterformat);
    dialog->show();

    if (dialog->run() != Gtk::RESPONSE_OK)
      return;

    dialog->hide();

    Glib::ustring charset = dialog->get_encoding();

    std::vector<Glib::ustring> uris = dialog->get_uris();

    for (const auto &uri : uris) {
      open_document(uri, charset);
    }

    Glib::ustring video_uri = dialog->get_video_uri();
    if (video_uri.empty() == false) {
      // TODO : check and ask if already exist ?
      SubtitleEditorWindow::get_instance()->get_player()->open(video_uri);
    }
  }

  bool open_document(const Glib::ustring &uri, const Glib::ustring &charset) {
    se_dbg_msg(SE_DBG_PLUGINS, "uri=%s charset=%s", uri.c_str(),
               charset.c_str());

    Glib::ustring filename = Glib::filename_from_uri(uri);

    // check if is not already open
    Document *already = se::documents::find_by_name(filename);

    if (already) {
      already->flash_message(_("I am already open"));
      return false;
    }
    // Don't need to catch error, create_from_file catch and
    // display errors messages if needs.
    Document *doc = Document::create_from_file(uri, charset);
    if (!doc)
      return false;

    se::documents::append(doc);
    return true;
  }

  // Save a document. If file doesn't exist use save_as
  bool save_document(Document *doc) {
    se_dbg(SE_DBG_PLUGINS);

    g_return_val_if_fail(doc, false);

    if (Glib::file_test(doc->getFilename(), Glib::FILE_TEST_EXISTS) == false)
      return save_as_document(doc);

    Glib::ustring filename = doc->getFilename();
    Glib::ustring uri = Glib::filename_to_uri(filename);
    Glib::ustring format = doc->getFormat();
    Glib::ustring charset = doc->getCharset();
    Glib::ustring newline = doc->getNewLine();

    if (doc->save(uri) == false) {
      // "The file FILENAME (FORMAT, CHARSET, NEWLINE) has not been saved."
      doc->message(_("The file %s (%s, %s, %s) has not been saved."),
                   filename.c_str(), format.c_str(), charset.c_str(),
                   newline.c_str());
      return false;
    }
    // "Saving file FILENAME (FORMAT, CHARSET, NEWLINE)."
    doc->flash_message(_("Saving file %s (%s, %s, %s)."), filename.c_str(),
                       format.c_str(), charset.c_str(), newline.c_str());
    return true;
  }

  bool save_as_document(Document *doc,
                        const Glib::ustring &default_format = Glib::ustring()) {
    se_dbg(SE_DBG_PLUGINS);

    g_return_val_if_fail(doc, false);

    Glib::ustring format =
        default_format.empty() ? doc->getFormat() : default_format;

    DialogSaveDocument::unique_ptr dialog = DialogSaveDocument::create();

    if (Glib::file_test(doc->getFilename(), Glib::FILE_TEST_EXISTS)) {
      dialog->set_current_folder_and_name(doc->getFilename());
    } else if (SubtitleEditorWindow::get_instance()
                   ->get_player()
                   ->get_state() != Player::NONE) {
      dialog->set_filename_from_another_uri(
          SubtitleEditorWindow::get_instance()->get_player()->get_uri(),
          SubtitleFormatSystem::instance().get_extension_of_format(format));
    } else {
      dialog->set_current_name(doc->getName());
    }

    dialog->set_format(format);
    dialog->set_encoding(doc->getCharset());
    dialog->set_newline(doc->getNewLine());
    dialog->set_do_overwrite_confirmation(true);
    dialog->show();

    int response = dialog->run();

    dialog->hide();
    if (response != Gtk::RESPONSE_OK)
      return false;

    Glib::ustring filename = dialog->get_filename();
    Glib::ustring uri = dialog->get_uri();
    format = dialog->get_format();
    Glib::ustring encoding = dialog->get_encoding();
    Glib::ustring newline = dialog->get_newline();

    doc->setFormat(format);
    doc->setCharset(encoding);
    doc->setNewLine(newline);

    if (doc->save(uri) == false) {
      doc->message(_("The file %s (%s, %s, %s) has not been saved."),
                   filename.c_str(), format.c_str(), encoding.c_str(),
                   newline.c_str());
      return false;
    }
    doc->flash_message(_("Saving file %s (%s, %s, %s)."), filename.c_str(),
                       format.c_str(), encoding.c_str(), newline.c_str());
    // update in recent manager
    add_document_in_recent_manager(doc);
    return true;
  }

  // Save a document. If file doesn't exist use on_save_as
  void on_save() {
    se_dbg(SE_DBG_PLUGINS);

    Document *doc = get_current_document();
    g_return_if_fail(doc);

    save_document(doc);
  }

  void on_save_project() {
    se_dbg(SE_DBG_PLUGINS);

    Document *doc = get_current_document();
    g_return_if_fail(doc);

    save_as_document(doc, "Subtitle Editor Project");
  }

  // Save document with new name
  void on_save_as() {
    se_dbg(SE_DBG_PLUGINS);

    Document *doc = get_current_document();
    g_return_if_fail(doc);

    save_as_document(doc);
  }

  // Save all open files
  void on_save_all_documents() {
    se_dbg(SE_DBG_PLUGINS);

    auto documents = get_subtitleeditor_window()->get_documents();

    for (const auto &doc : documents) {
      save_document(doc);
    }
  }

  // Open translation from file.
  // Create a new document with a translation
  // and move the text of this new document (trans) to the current document
  // at the column "translation". After that delete the new document (trans)
  void on_open_translation() {
    se_dbg(SE_DBG_PLUGINS);

    Document *current = get_current_document();
    g_return_if_fail(current);

    DialogOpenDocument::unique_ptr ui = DialogOpenDocument::create();

    ui->show_video(false);
    ui->set_select_multiple(false);
    ui->show();

    if (ui->run() != Gtk::RESPONSE_OK)
      return;

    ui->hide();

    Glib::ustring encoding = ui->get_encoding();
    Glib::ustring uri = ui->get_uri();

    // We don't need to catch error because Document::create_from_file already
    // catch and show a error dialog if needs.
    Document *doc = Document::create_from_file(uri, encoding);
    if (doc == NULL)
      return;

    current->start_command(_("Open translation"));

    Subtitle s1 = current->subtitles().get_first();
    Subtitle s2 = doc->subtitles().get_first();

    while (s1 && s2) {
      s1.set_translation(s2.get_text());

      ++s1;
      ++s2;
    }

    // create new subtitle
    if (s2) {
      int size = doc->subtitles().size() - current->subtitles().size();

      while (s2) {
        s1 = current->subtitles().append();

        s1.set_translation(s2.get_text());
        s1.set_start_and_end(s2.get_start(), s2.get_end());
        ++s2;
      }

      current->flash_message(
          ngettext("1 subtitle was added with the translation",
                   "%d subtitles were added with the translation", size),
          size);
    }
    current->finish_command();
    delete doc;
  }

  // Save the current translation in a new document
  void on_save_translation() {
    se_dbg(SE_DBG_PLUGINS);

    Document *current = get_current_document();
    g_return_if_fail(current);

    DialogSaveDocument::unique_ptr dialog = DialogSaveDocument::create();

    // default to the format of the original document
    dialog->set_format(current->getFormat());
    dialog->set_encoding(current->getCharset());
    dialog->set_newline(current->getNewLine());
    dialog->show();

    if (dialog->run() != Gtk::RESPONSE_OK)
      return;
    dialog->hide();

    Glib::ustring filename = dialog->get_filename();
    Glib::ustring uri = dialog->get_uri();
    Glib::ustring format = dialog->get_format();
    Glib::ustring encoding = dialog->get_encoding();
    Glib::ustring newline = dialog->get_newline();

    Document doc_translation(*current);

    doc_translation.setFilename(filename);
    doc_translation.setFormat(format);
    doc_translation.setCharset(encoding);
    doc_translation.setNewLine(newline);

    // apply translation
    Subtitle sub = doc_translation.subtitles().get_first();
    while (sub) {
      sub.set_text(sub.get_translation());
      ++sub;
    }
    // display message for user
    if (doc_translation.save(uri) == false)
      current->message(
          _("The translation file %s (%s, %s, %s) has not been saved."),
          filename.c_str(), format.c_str(), encoding.c_str(), newline.c_str());
    else
      current->flash_message(_("Saving translation file %s (%s, %s, %s)."),
                             filename.c_str(), format.c_str(), encoding.c_str(),
                             newline.c_str());
  }

  bool on_subtitleeditor_window_delete_event(GdkEventAny * /*ev*/) {
    while (get_current_document() != NULL) {
      if (close_current_document() == false)
        return true;  // true to stop the closing
    }
    return false;
  }

  bool close_current_document() {
    Document *doc = get_current_document();
    g_return_val_if_fail(doc, false);

    if (cfg::get_boolean("interface", "ask-to-save-on-exit") == false) {
      se::documents::remove(doc);
    } else if (!doc->get_document_changed()) {
      se::documents::remove(doc);
    } else {
      DialogAskToSaveOnExit dialog;

      int response = dialog.run(doc);

      if (response == Gtk::RESPONSE_YES) {
        on_save();

        // if(doc->get_document_changed() == false)
        se::documents::remove(doc);
      } else if (response == Gtk::RESPONSE_NO) {
        se::documents::remove(doc);
      } else if (response == Gtk::RESPONSE_CANCEL) {
        // nothing
        return false;
      }
    }
    return true;
  }

  // Remove the current document
  void on_close() {
    se_dbg(SE_DBG_PLUGINS);

    close_current_document();
  }

  // Quit the program
  // Close all document with support to ask to save if is enable
  void on_exit() {
    while (get_current_document() != NULL) {
      if (!close_current_document())
        return;
    }
    Gtk::Main::quit();
  }

  // A new document has been create, update the recent manager
  void on_document_create(Document *doc) {
    se_dbg(SE_DBG_PLUGINS);

    add_document_in_recent_manager(doc);
  }

  void add_document_in_recent_manager(Document *doc) {
    se_dbg(SE_DBG_PLUGINS);

    if (doc == NULL)
      return;

    Glib::ustring filename = doc->getFilename();

    if (!Glib::file_test(filename, Glib::FILE_TEST_EXISTS))
      return;

    Glib::ustring uri = Glib::filename_to_uri(filename);

    se_dbg_msg(SE_DBG_PLUGINS, "uri=%s", uri.c_str());

    Gtk::RecentManager::Data data;
    // data.mime_type = "subtitle/";
    data.app_name = Glib::get_application_name();
    data.app_exec = Glib::get_prgname();
    data.groups.push_back("subtitleeditor");
    data.is_private = false;
    Gtk::RecentManager::get_default()->add_item(uri, data);
  }

  // Open a recent document
  void on_recent_item_activated() {
    Glib::RefPtr<Gtk::Action> action =
        action_group->get_action("menu-recent-open-document");

    Glib::RefPtr<Gtk::RecentAction> recentAction =
        Glib::RefPtr<Gtk::RecentAction>::cast_static(action);

    Glib::RefPtr<Gtk::RecentInfo> cur = recentAction->get_current_item();

    if (cur) {
      se_dbg_msg(SE_DBG_PLUGINS, "uri=%s", cur->get_uri().c_str());

      open_document(cur->get_uri(), "");
    }
  }

  // Only for "used-autosave" and "autosave-minutes".
  void on_config_interface_changed(const Glib::ustring &key,
                                   const Glib::ustring & /*value*/) {
    if (key == "used-autosave" || key == "autosave-minutes")
      init_autosave();
  }

  void init_autosave() {
    se_dbg(SE_DBG_PLUGINS);

    m_autosave_timeout.disconnect();

    if (cfg::get_boolean("interface", "used-autosave") == false)
      return;

    int autosave_minutes = cfg::get_int("interface", "autosave-minutes");

    long mseconds = SubtitleTime(0, autosave_minutes, 0, 0).totalmsecs;

    m_autosave_timeout = Glib::signal_timeout().connect(
        sigc::mem_fun(*this, &DocumentManagementPlugin::on_autosave_files),
        mseconds);

    se_dbg_msg(SE_DBG_PLUGINS, "save files every %d minutes", autosave_minutes);
  }

  // Save files every "auto-save-minutes" value.
  bool on_autosave_files() {
    se_dbg(SE_DBG_PLUGINS);

    on_save_all_documents();
    return true;
  }

 protected:
  Gtk::UIManager::ui_merge_id ui_id;
  Glib::RefPtr<Gtk::ActionGroup> action_group;
  sigc::connection m_config_interface_connection;
  sigc::connection m_autosave_timeout;
};

REGISTER_EXTENSION(DocumentManagementPlugin)
