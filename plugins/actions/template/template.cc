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

#include <documents.h>
#include <extension/action.h>
#include <gtkmm_utility.h>
#include <gui/comboboxencoding.h>
#include <gui/comboboxnewline.h>
#include <gui/comboboxsubtitleformat.h>
#include <utility.h>

#include <memory>

class DialogTemplate : public Gtk::Dialog {
  public:
   DialogTemplate(BaseObjectType* cobj, const Glib::RefPtr<Gtk::Builder>& xml) : Gtk::Dialog(cobj) {
      utility::set_transient_parent(*this);

      xml->get_widget("entry-name", m_entryName);
      xml->get_widget_derived("combo-format", m_comboFormat);
      xml->get_widget_derived("combo-encoding", m_comboEncoding);
      xml->get_widget_derived("combo-newline", m_comboNewLine);

      m_comboEncoding->show_auto_detected(false);

      add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
      add_button(Gtk::Stock::SAVE_AS, Gtk::RESPONSE_OK);

      set_default_response(Gtk::RESPONSE_OK);
   }

   void set_name(const Glib::ustring& name) {
      m_entryName->set_text(name);
   }

   Glib::ustring get_name() {
      return m_entryName->get_text();
   }

   void set_charset(const Glib::ustring& charset) {
      m_comboEncoding->set_value(charset);
   }

   Glib::ustring get_charset() {
      return m_comboEncoding->get_value();
   }

   void set_format(const Glib::ustring& format) {
      m_comboFormat->set_value(format);
   }

   Glib::ustring get_format() {
      return m_comboFormat->get_value();
   }

   void set_newline(const Glib::ustring& newline) {
      m_comboNewLine->set_value(newline);
   }

   Glib::ustring get_newline() {
      return m_comboNewLine->get_value();
   }

  public:
   Gtk::Entry* m_entryName;
   ComboBoxEncoding* m_comboEncoding;
   ComboBoxSubtitleFormat* m_comboFormat;
   ComboBoxNewLine* m_comboNewLine;
};

class TemplatePlugin : public Action {
  public:
   TemplatePlugin() {
      activate();
      update_ui();
   }

   ~TemplatePlugin() {
      deactivate();
   }

   void activate() {
      se_dbg(SE_DBG_PLUGINS);

      // actions
      action_group = Gtk::ActionGroup::create("TemplatePlugin");

      action_group->add(Gtk::Action::create("template", _("_Template"), _("Save a template or use a template")));

      action_group->add(
         Gtk::Action::create("save-as-template", Gtk::Stock::SAVE_AS, _("_Save As Template"), _("Save the current document as template")),
         sigc::mem_fun(*this, &TemplatePlugin::on_save_as_template));

      // ui
      Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

      ui->insert_action_group(action_group);

      Glib::ustring submenu = R"(
      <ui>
        <menubar name='menubar'>
          <menu name='menu-extensions' action='menu-extensions'>
            <placeholder name='placeholder'>
              <menu name='template' action='template'>
                <menuitem action='save-as-template'/>
                <separator/>
                <placeholder name='template-files'/>
              </menu>
            </placeholder>
          </menu>
        </menubar>
      </ui>
    )";

      ui_id = ui->add_ui_from_string(submenu);

      rebuild_templates_menu();
   }

   void deactivate() {
      se_dbg(SE_DBG_PLUGINS);

      Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

      if (action_group_templates) {
         ui->remove_ui(ui_id_templates);
         ui->remove_action_group(action_group_templates);
      }
      ui->remove_ui(ui_id);
      ui->remove_action_group(action_group);
   }

   void update_ui() {
      se_dbg(SE_DBG_PLUGINS);

      bool visible = (get_current_document() != NULL);

      action_group->get_action("save-as-template")->set_sensitive(visible);
   }

   void rebuild_templates_menu() {
      Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();
      // We clean the old ui and action_group
      if (action_group_templates) {
         ui->remove_ui(ui_id_templates);
         ui->remove_action_group(action_group_templates);
      }
      // Create new ui and action_group
      action_group_templates = Gtk::ActionGroup::create("TemplatePluginFiles");
      ui->insert_action_group(action_group_templates);

      ui_id_templates = ui->new_merge_id();

      // Read the template directory
      if (initialize_template_directory() == false)
         return;

      // For each files we create an entry
      Glib::Dir dir(get_config_dir("plugins/template"));
      std::vector<Glib::ustring> files(dir.begin(), dir.end());

      for (unsigned int i = 0; i < files.size(); ++i) {
         add_ui_from_file(i, files[i]);
      }
      // Update the ui
      ui->ensure_update();
   }

   void add_ui_from_file(const guint number, const Glib::ustring& filename) {
      // We extract from the filename the label and the charset
      Glib::RefPtr<Glib::Regex> re = Glib::Regex::create("^\\[(.*)\\]\\[(.*)\\]$");
      if (re->match(filename) == false) {
         // Something wrong...
         return;
      }
      std::vector<Glib::ustring> group = re->split(filename);

      Glib::ustring label = group[1];
      Glib::ustring charset = group[2];
      Glib::ustring fullname = Glib::build_filename(get_config_dir("plugins/template"), filename);

      Glib::ustring action_name = Glib::ustring::compose("template-file-%1", number);
      Glib::ustring action_accel = "";

      action_group_templates->add(Gtk::Action::create(action_name, label),
                                  Gtk::AccelKey(action_accel),
                                  sigc::bind(sigc::mem_fun(*this, &TemplatePlugin::on_template_activate), fullname, charset));

      get_ui_manager()->add_ui(
         ui_id_templates, "/menubar/menu-extensions/placeholder/template/template-files", action_name, action_name, Gtk::UI_MANAGER_MENUITEM, false);
   }

   void on_template_activate(const Glib::ustring& path, const Glib::ustring& charset) {
      Glib::ustring uri = Glib::filename_to_uri(path);

      Document* doc = Document::create_from_file(uri, charset);
      if (doc == NULL)
         return;

      doc->setFilename(se::documents::generate_untitled_name());
      doc->setCharset(charset);
      se::documents::append(doc);
   }

   void on_save_as_template() {
      Document* current = get_current_document();
      g_return_if_fail(current);

      // create dialog
      std::unique_ptr<DialogTemplate> dialog(gtkmm_utility::get_widget_derived<DialogTemplate>(
         SE_DEV_VALUE(SE_PLUGIN_PATH_UI, SE_PLUGIN_PATH_DEV), "dialog-template-save-as.ui", "dialog-template-save-as"));

      dialog->set_name(current->getName());
      dialog->set_format(current->getFormat());
      dialog->set_newline(current->getNewLine());
      dialog->set_charset(current->getCharset());

      if (dialog->run() != Gtk::RESPONSE_OK)
         return;

      std::unique_ptr<Document> newdoc(new Document(*current));
      newdoc->setName(dialog->get_name());
      newdoc->setFormat(dialog->get_format());
      newdoc->setNewLine(dialog->get_newline());
      newdoc->setCharset(dialog->get_charset());

      Glib::ustring uri = Glib::filename_to_uri(
         Glib::build_filename(get_config_dir("plugins/template"), Glib::ustring::compose("[%1][%2]", newdoc->getName(), newdoc->getCharset())));

      if (newdoc->save(uri) == false)
         return;

      rebuild_templates_menu();
   }

   // Check if the template directory exists
   // Create if needed
   bool initialize_template_directory() {
      se_dbg(SE_DBG_PLUGINS);

      // Read the template directory
      Glib::ustring path = get_config_dir("plugins/template");
      // Already there ?
      if (Glib::file_test(path, Glib::FILE_TEST_EXISTS | Glib::FILE_TEST_IS_DIR))
         return true;
      // Needs to be created
      try {
         Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(path);
         if (file && file->make_directory_with_parents())
            return true;
      } catch (...) {
      }
      return false;
   }

  protected:
   Gtk::UIManager::ui_merge_id ui_id;
   Glib::RefPtr<Gtk::ActionGroup> action_group;

   Gtk::UIManager::ui_merge_id ui_id_templates;
   Glib::RefPtr<Gtk::ActionGroup> action_group_templates;
};

REGISTER_EXTENSION(TemplatePlugin)
