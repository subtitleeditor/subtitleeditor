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

#include <debug.h>
#include <documents.h>
#include <extension/action.h>
#include <i18n.h>

#include <algorithm>

class DocumentsNavigationPlugin : public Action {
  public:
   DocumentsNavigationPlugin() {
      activate();
      update_ui();
   }

   ~DocumentsNavigationPlugin() {
      deactivate();
   }

   void activate() {
      se_dbg(SE_DBG_PLUGINS);

      // actions
      action_group = Gtk::ActionGroup::create("DocumentsNavigationPlugin");

      action_group->add(Gtk::Action::create("documentsnavigation", _("_Tabs"), _("Select a tab")));

      action_group->add(
         Gtk::Action::create("documentsnavigation-first-document", Gtk::Stock::GOTO_FIRST, _("_First Tab"), _("Select the first (left-most) tab")),
         sigc::bind<int>(sigc::mem_fun(*this, &DocumentsNavigationPlugin::on_select_document), FIRST));

      action_group->add(
         Gtk::Action::create("documentsnavigation-last-document", Gtk::Stock::GOTO_LAST, _("_Last Tab"), _("Select the last (right-most) tab")),
         sigc::bind<int>(sigc::mem_fun(*this, &DocumentsNavigationPlugin::on_select_document), LAST));

      action_group->add(Gtk::Action::create("documentsnavigation-previous-document",
                                            Gtk::Stock::GO_BACK,
                                            _("_Previous Tab"),
                                            _("Select the tab on the left (or cycle to the right-most one)")),
                        sigc::bind<int>(sigc::mem_fun(*this, &DocumentsNavigationPlugin::on_select_document), PREVIOUS));

      action_group->add(Gtk::Action::create("documentsnavigation-next-document",
                                            Gtk::Stock::GO_FORWARD,
                                            _("_Next Tab"),
                                            _("Select the tab on the right (or cycle to the left-most one)")),
                        sigc::bind<int>(sigc::mem_fun(*this, &DocumentsNavigationPlugin::on_select_document), NEXT));

      // ui
      Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

      ui->insert_action_group(action_group);

      Glib::ustring submenu = R"(
      <ui>
        <menubar name='menubar'>
          <menu name='menu-extensions' action='menu-extensions'>
            <placeholder name='placeholder'>
              <menu name='documentsnavigation' action='documentsnavigation'>
                <menuitem action='documentsnavigation-first-document'/>
                <menuitem action='documentsnavigation-last-document'/>
                <separator/>
                <menuitem action='documentsnavigation-previous-document'/>
                <menuitem action='documentsnavigation-next-document'/>
                <separator/>
                <placeholder name='documentsnavigation-documents'/>
              </menu>
            </placeholder>
          </menu>
        </menubar>
      </ui>
    )";

      ui_id = ui->add_ui_from_string(submenu);

      // Update the documents menu when a document is created, deleted or changed
      m_create_document_connection =
         se::documents::signal_created().connect(sigc::mem_fun(*this, &DocumentsNavigationPlugin::on_document_create_or_delete));

      m_delete_document_connection =
         se::documents::signal_deleted().connect(sigc::mem_fun(*this, &DocumentsNavigationPlugin::on_document_create_or_delete));

      m_document_signals_connection = se::documents::signal_modified().connect(sigc::mem_fun(*this, &DocumentsNavigationPlugin::on_document_signals));

      rebuild_documents_menu();
   }

   void deactivate() {
      se_dbg(SE_DBG_PLUGINS);

      Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

      m_create_document_connection.disconnect();
      m_delete_document_connection.disconnect();
      m_document_signals_connection.disconnect();

      if (action_group_documents) {
         get_ui_manager()->remove_ui(ui_id_documents);
         get_ui_manager()->remove_action_group(action_group_documents);
      }
      ui->remove_ui(ui_id);
      ui->remove_action_group(action_group);
   }

   void update_ui() {
      se_dbg(SE_DBG_PLUGINS);

      bool state = (get_current_document() != NULL);

      action_group->get_action("documentsnavigation-first-document")->set_sensitive(state);
      action_group->get_action("documentsnavigation-last-document")->set_sensitive(state);
      action_group->get_action("documentsnavigation-previous-document")->set_sensitive(state);
      action_group->get_action("documentsnavigation-next-document")->set_sensitive(state);
   }

   // Remove old ui_id and action_group and regenerate menu items.
   void rebuild_documents_menu() {
      if (action_group_documents) {
         get_ui_manager()->remove_ui(ui_id_documents);
         get_ui_manager()->remove_action_group(action_group_documents);
      }

      action_group_documents = Gtk::ActionGroup::create("DocumentsNavigationPluginDocuments");
      get_ui_manager()->insert_action_group(action_group_documents);

      ui_id_documents = get_ui_manager()->new_merge_id();

      guint count = 0;

      auto documents = se::documents::all();

      for (auto it = documents.begin(); it != documents.end(); ++it, ++count) {
         Glib::ustring action_name = Glib::ustring::compose("documentsnavigation-document-%1", count);
         Glib::ustring action_accel = (count < 10) ? Glib::ustring::compose("<alt>%1", (count + 1) % 10) : "";

         action_group_documents->add(Gtk::Action::create(action_name, (*it)->getName()),
                                     Gtk::AccelKey(action_accel),
                                     sigc::bind(sigc::mem_fun(*this, &DocumentsNavigationPlugin::on_documents_menu_activate), count));

         get_ui_manager()->add_ui(ui_id_documents,
                                  "/menubar/menu-extensions/placeholder/documentsnavigation/"
                                  "documentsnavigation-documents",
                                  action_name,
                                  action_name,
                                  Gtk::UI_MANAGER_MENUITEM,
                                  false);
      }
      get_ui_manager()->ensure_update();
   }

   enum { FIRST = 0, LAST = 1, PREVIOUS = 2, NEXT = 3 };

   void on_select_document(int value) {
      se_dbg_msg(SE_DBG_PLUGINS, "select %d", value);

      auto documents = se::documents::all();
      g_return_if_fail(!documents.empty());

      Document* doc = NULL;

      if (value == FIRST)
         doc = documents.front();
      else if (value == LAST)
         doc = documents.back();
      else if (value == PREVIOUS)
         doc = get_document(PREVIOUS);
      else
         doc = get_document(NEXT);

      g_return_if_fail(doc);
      se::documents::active(doc);
   }

   // We want to rebuild the documents menu each time a document is created or
   // delete
   void on_document_create_or_delete(Document* doc) {
      g_return_if_fail(doc);

      rebuild_documents_menu();
   }

   // We want to rebuild the documents menu each time the document property
   // change, like name of the document
   void on_document_signals(Document*, const std::string& signal) {
      if (signal == "document-property-changed")
         rebuild_documents_menu();
   }

   // PREVIOUS or NEXT
   Document* get_document(int value) {
      se_dbg(SE_DBG_PLUGINS);

      Document* current = get_current_document();
      g_return_val_if_fail(current, NULL);

      auto docs = se::documents::all();

      if (value == PREVIOUS)
         std::reverse(docs.begin(), docs.end());

      for (auto it = docs.begin(); it != docs.end(); ++it) {
         if (*it == current) {
            ++it;
            if (it == docs.end())
               return docs.front();
            else
               return *it;
         }
      }
      return NULL;
   }

   void on_documents_menu_activate(gint count) {
      se_dbg_msg(SE_DBG_PLUGINS, "activate document %d", count);

      auto docs = se::documents::all();
      g_return_if_fail(!docs.empty());

      auto it = docs.begin();

      std::advance(it, count);
      g_return_if_fail(it != docs.end());

      se::documents::active(*it);
   }

  protected:
   Gtk::UIManager::ui_merge_id ui_id;
   Glib::RefPtr<Gtk::ActionGroup> action_group;

   Gtk::UIManager::ui_merge_id ui_id_documents;
   Glib::RefPtr<Gtk::ActionGroup> action_group_documents;

   sigc::connection m_create_document_connection;
   sigc::connection m_delete_document_connection;
   sigc::connection m_document_signals_connection;
};

REGISTER_EXTENSION(DocumentsNavigationPlugin)
