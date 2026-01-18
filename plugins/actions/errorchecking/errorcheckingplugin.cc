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

#include <extension/action.h>
#include <gtkmm_utility.h>
#include <utility.h>

#include <memory>

#include "errorchecking.h"
#include "errorcheckingpreferences.h"
#include "maxcharactersperline.h"
#include "maxcharacterspersecond.h"
#include "maxlinepersubtitle.h"
#include "mincharacterspersecond.h"
#include "mindisplaytime.h"
#include "mingapbetweensubtitles.h"
#include "overlapping.h"

class ErrorCheckingGroup : public std::vector<ErrorChecking*> {
  public:
   ErrorCheckingGroup() {
      push_back(new Overlapping);
      push_back(new MinGapBetweenSubtitles);
      push_back(new MaxCharactersPerSecond);
      push_back(new MinCharactersPerSecond);
      push_back(new MinDisplayTime);
      push_back(new MaxCharactersPerLine);
      push_back(new MaxLinePerSubtitle);

      init_settings();
   }

   ~ErrorCheckingGroup() {
      for (auto it = begin(); it != end(); ++it) {
         delete *it;
      }
      clear();
   }

   void init_settings() {
      for (auto it = begin(); it != end(); ++it) {
         (*it)->init();
      }
   }

   ErrorChecking* get_by_name(const Glib::ustring& name) {
      for (auto it = begin(); it != end(); ++it) {
         if ((*it)->get_name() == name)
            return *it;
      }
      return nullptr;
   }
};

class DialogErrorChecking : public Gtk::Dialog {
   enum SortType { BY_CATEGORIES = 0, BY_SUBTITLES = 1 };

   class Column : public Gtk::TreeModel::ColumnRecord {
     public:
      Column() {
         add(text);
         add(solution);
         add(checker);
         add(num);
      }
      Gtk::TreeModelColumn<Glib::ustring> text;
      Gtk::TreeModelColumn<Glib::ustring> solution;

      Gtk::TreeModelColumn<Glib::ustring> num;  // Only used when the view is sort by subtitle.
      Gtk::TreeModelColumn<ErrorChecking*> checker;
   };

   static DialogErrorChecking* m_static_instance;

  public:
   static void create() {
      se_dbg(SE_DBG_PLUGINS);

      if (m_static_instance == NULL)
         m_static_instance = gtkmm_utility::get_widget_derived<DialogErrorChecking>(
            SE_DEV_VALUE(SE_PLUGIN_PATH_UI, SE_PLUGIN_PATH_DEV), "dialog-error-checking.ui", "dialog-error-checking");

      g_return_if_fail(m_static_instance);

      m_static_instance->show();
      m_static_instance->present();
   }

   static DialogErrorChecking* get_instance() {
      return m_static_instance;
   }

   DialogErrorChecking(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) : Gtk::Dialog(cobject) {
      se_dbg(SE_DBG_PLUGINS);

      m_sort_type = BY_CATEGORIES;

      create_menubar(builder);

      builder->get_widget("treeview-errors", m_treeview);
      builder->get_widget("statusbar", m_statusbar);

      create_treeview();
      refresh();
   }

   void on_quit() {
      delete m_static_instance;
      m_static_instance = nullptr;
   }

   bool on_delete_event(GdkEventAny* ev) {
      se_dbg(SE_DBG_PLUGINS);

      m_static_instance = nullptr;

      return Gtk::Window::on_delete_event(ev);
   }

   // Create the menubar and actions
   void create_menubar(const Glib::RefPtr<Gtk::Builder>& builder) {
      Gtk::Box* vbox;
      builder->get_widget("box", vbox);

      // ui
      Glib::ustring ui_info = R"(
      <ui>
        <menubar name='MenuBar'>
          <menu action='MenuError'>
            <menuitem action='Refresh'/>
            <menuitem action='TryToFixAll'/>
            <separator/>
            <menuitem action='Quit'/>
          </menu>
          <menu action='MenuView'>
            <menuitem action='SortByCategories'/>
            <menuitem action='SortBySubtitles'/>
            <separator/>
            <menuitem action='CollapseAll'/>
            <menuitem action='ExpandAll'/>
          </menu>
          <menu action='MenuOptions'>
            <menuitem action='Preferences'/>
          </menu>
        </menubar>
      </ui>
    )";

      // actions
      m_action_group = Gtk::ActionGroup::create();

      // File
      m_action_group->add(Gtk::Action::create("MenuError", _("_Error")));
      m_action_group->add(
         Gtk::Action::create("Refresh", Gtk::Stock::REFRESH), Gtk::AccelKey("F5"), sigc::mem_fun(*this, &DialogErrorChecking::refresh));
      m_action_group->add(Gtk::Action::create("TryToFixAll", Gtk::Stock::APPLY, _("Try To _Fix All")),
                          Gtk::AccelKey("F3"),
                          sigc::mem_fun(*this, &DialogErrorChecking::try_to_fix_all));
      m_action_group->add(Gtk::Action::create("Quit", Gtk::Stock::QUIT), sigc::mem_fun(*this, &DialogErrorChecking::on_quit));

      // menu view
      Gtk::RadioAction::Group sort_group;
      m_action_group->add(Gtk::Action::create("MenuView", _("_View")));
      m_action_group->add(Gtk::RadioAction::create(sort_group, "SortByCategories", _("By _Categories")),
                          Gtk::AccelKey("<Control>C"),
                          sigc::bind(sigc::mem_fun(*this, &DialogErrorChecking::set_sort_type), BY_CATEGORIES));
      m_action_group->add(Gtk::RadioAction::create(sort_group, "SortBySubtitles", _("By _Subtitles")),
                          Gtk::AccelKey("<Control>S"),
                          sigc::bind(sigc::mem_fun(*this, &DialogErrorChecking::set_sort_type), BY_SUBTITLES));

      m_action_group->add(
         Gtk::Action::create("CollapseAll", _("_Collapse All")), Gtk::AccelKey("C"), sigc::mem_fun(*this, &DialogErrorChecking::on_collapse_all));
      m_action_group->add(
         Gtk::Action::create("ExpandAll", _("_Expand All")), Gtk::AccelKey("E"), sigc::mem_fun(*this, &DialogErrorChecking::on_expand_all));

      // menu option
      m_action_group->add(Gtk::Action::create("MenuOptions", _("_Options")));
      m_action_group->add(Gtk::Action::create("Preferences", Gtk::Stock::PREFERENCES),
                          Gtk::AccelKey("<Control>P"),
                          sigc::mem_fun(*this, &DialogErrorChecking::on_preferences));

      // generate ui
      Glib::RefPtr<Gtk::UIManager> ui = Gtk::UIManager::create();
      ui->insert_action_group(m_action_group);
      add_accel_group(ui->get_accel_group());

      ui->add_ui_from_string(ui_info);

      Gtk::Widget* menubar = ui->get_widget("/MenuBar");
      vbox->pack_start(*menubar, Gtk::PACK_SHRINK);
      vbox->reorder_child(*menubar, 0);
      menubar->show_all();
   }

   // Update the state of the ui from the new document (or empty).
   void on_current_document_changed(Document* doc) {
      bool state = (doc != NULL);

      m_action_group->get_action("Refresh")->set_sensitive(state);
      m_action_group->get_action("TryToFixAll")->set_sensitive(state);
      m_action_group->get_action("ExpandAll")->set_sensitive(state);
      m_action_group->get_action("CollapseAll")->set_sensitive(state);

      refresh();
   }

   // Return the current document.
   Document* get_document() {
      return SubtitleEditorWindow::get_instance()->get_current_document();
   }

   // Set the sort method.
   void set_sort_type(SortType type) {
      m_sort_type = type;

      refresh();
   }

   // Return the sort type.
   SortType get_sort_type() {
      return m_sort_type;
   }

   // Create the treeview and connect signals.
   void create_treeview() {
      // create the model
      m_model = Gtk::TreeStore::create(m_column);
      m_treeview->set_model(m_model);

      Gtk::TreeViewColumn* column = NULL;
      Gtk::CellRendererText* renderer = NULL;

      // create one column for both
      column = manage(new Gtk::TreeViewColumn);
      m_treeview->append_column(*column);

      // add error msg renderer
      renderer = manage(new Gtk::CellRendererText);
      // renderer->property_wrap_mode() = Pango::WRAP_WORD;
      // renderer->property_wrap_width() = 300;
      column->pack_start(*renderer, false);
      column->add_attribute(renderer->property_markup(), m_column.text);

      // set treeview property
      m_treeview->set_rules_hint(true);

      // signals
      m_treeview->get_selection()->signal_changed().connect(sigc::mem_fun(*this, &DialogErrorChecking::on_selection_changed));

      m_treeview->signal_row_activated().connect(sigc::mem_fun(*this, &DialogErrorChecking::on_row_activated));
      // tooltip
      m_treeview->set_has_tooltip(true);

      m_treeview->signal_query_tooltip().connect(sigc::mem_fun(*this, &DialogErrorChecking::on_query_tooltip));

      m_treeview->show_all();
   }

   void on_collapse_all() {
      m_treeview->collapse_all();
   }

   void on_expand_all() {
      m_treeview->expand_all();
   }

   // Get the subtitle num of the error and select it
   // in the subtitle view editor.
   void on_selection_changed() {
      Document* doc = get_document();
      if (doc == NULL)
         return;

      Gtk::TreeIter it = m_treeview->get_selection()->get_selected();
      if (!it)
         return;

      unsigned int num = utility::string_to_int(Glib::ustring((*it)[m_column.num]));

      Subtitle sub = doc->subtitles().get(num);
      if (sub)
         doc->subtitles().select(sub);
   }

   // Update the statusbar message by the number of error.
   void set_statusbar_error(unsigned int count) {
      if (count == 0)
         m_statusbar->push(_("No error was found."));
      else
         m_statusbar->push(build_message(ngettext("1 error was found.", "%d errors were found.", count), count));
   }

   // Add an error in the node.
   // The label depend of the sort type.
   void add_error(Gtk::TreeModel::Row& node, ErrorChecking::Info& info, ErrorChecking* checker) {
      Glib::ustring text;

      if (get_sort_type() == BY_CATEGORIES) {
         Glib::ustring subtitle = build_message(_("Subtitle n°<b>%d</b>"), info.currentSub.get_num());
         Glib::ustring error = info.error;

         text = build_message("%s\n%s", subtitle.c_str(), error.c_str());
      } else if (get_sort_type() == BY_SUBTITLES) {
         Glib::ustring checker_label = checker->get_label();
         Glib::ustring error = info.error;

         text = build_message("%s\n%s", checker_label.c_str(), error.c_str());
      }

      Gtk::TreeIter it = m_model->append(node.children());

      (*it)[m_column.num] = to_string(info.currentSub.get_num());
      (*it)[m_column.checker] = checker;
      (*it)[m_column.text] = text;
      (*it)[m_column.solution] = info.solution;
   }

   // Rebuild the model.
   // Check errors.
   void refresh() {
      m_model->clear();
      m_statusbar->push("");

      Document* doc = get_document();
      if (doc == NULL)
         return;

      if (get_sort_type() == BY_CATEGORIES)
         check_by_categories(doc, m_checker_list);
      else  // BY_SUBTITLES
         check_by_subtitle(doc, m_checker_list);
   }

   // Check errors by organizing them by type of error.
   void check_by_categories(Document* doc, const std::vector<ErrorChecking*>& checkers) {
      unsigned int count_error = 0;

      Subtitles subtitles = doc->subtitles();

      for (const auto& checker : checkers) {
         if (checker->get_active() == false)
            continue;

         // Check all subtitles with the current checker
         Gtk::TreeModel::Row row = *(m_model->append());

         Subtitle current, previous, next;
         for (current = subtitles.get_first(); current; ++current) {
            // get next
            next = current;
            ++next;

            // info
            ErrorChecking::Info info;
            info.document = doc;
            info.currentSub = current;
            info.nextSub = next;
            info.previousSub = previous;
            info.tryToFix = false;

            if (checker->execute(info)) {
               add_error(row, info, checker);
               ++count_error;
            }

            previous = current;
         }

         // Update the node label or delete if it empty
         if (row.children().empty()) {
            m_model->erase(row);
         } else {
            row[m_column.checker] = checker;

            update_node_label(row);
         }
      }

      set_statusbar_error(count_error);
   }

   // Check errors by organizing them by subtitle.
   void check_by_subtitle(Document* doc, const std::vector<ErrorChecking*>& checkers) {
      std::vector<ErrorChecking*>::const_iterator checker_it;

      Subtitles subtitles = doc->subtitles();

      unsigned int count_error = 0;
      Subtitle current, previous, next;
      for (current = subtitles.get_first(); current; ++current) {
         // get next
         next = current;
         ++next;

         Gtk::TreeModel::Row row = *(m_model->append());

         for (const auto& checker : checkers) {
            if (checker->get_active() == false)
               continue;

            // info
            ErrorChecking::Info info;
            info.document = doc;
            info.currentSub = current;
            info.nextSub = next;
            info.previousSub = previous;
            info.tryToFix = false;

            if (checker->execute(info) == false)
               continue;

            add_error(row, info, checker);

            ++count_error;
         }

         // Update the row label or remove the node if it's empty
         if (row.children().empty()) {
            m_model->erase(row);
         } else {
            row[m_column.checker] = NULL;  // do not needs because is sort by subtitles
            row[m_column.num] = to_string(current.get_num());

            update_node_label(row);
         }

         previous = current;
      }

      set_statusbar_error(count_error);
   }

   // FIXME
   void try_to_fix_all() {
      Document* doc = get_document();
      if (doc == NULL)
         return;

      ErrorCheckingGroup group;
      for (const auto& c : group) {
         if (c->get_active())
            fix_error(c, doc);
      }
      refresh();
   }

   // FIXME
   void fix_row(Gtk::TreeModel::Row& row) {
      Gtk::TreeIter it = row.children().begin();
      while (it) {
         if (fix_selected(it))
            it = m_model->erase(it);
         else
            ++it;
      }

      // All children have been corrected, the node is no longer useful.
      if (row.children().empty())
         m_model->erase(row);
   }

   // Fix the error with the support of the Command Recorder.
   bool error_checking_fix(ErrorChecking* checker, ErrorChecking::Info& info) {
      info.document->start_command(checker->get_label());

      bool res = checker->execute(info);

      info.document->finish_command();

      return res;
   }

   // FIXME
   bool fix_selected(Gtk::TreeIter& iter) {
      ErrorChecking* checker = (*iter)[m_column.checker];

      if (checker == nullptr)
         return false;

      Document* doc = get_document();

      Glib::ustring num = (*iter)[m_column.num];

      Subtitle current = doc->subtitles().get(utility::string_to_int(num));
      Subtitle previous = doc->subtitles().get_previous(current);
      Subtitle next = doc->subtitles().get_next(current);

      // info
      ErrorChecking::Info info;
      info.document = doc;
      info.currentSub = current;
      info.nextSub = next;
      info.previousSub = previous;
      info.tryToFix = true;

      return error_checking_fix(checker, info);
   }

   // FIXME
   unsigned int fix_error(ErrorChecking* checker, Document* doc) {
      Subtitles subtitles = doc->subtitles();

      unsigned int count = 0;
      Subtitle current, previous, next;
      for (current = subtitles.get_first(); current; ++current) {
         // get next
         next = current;
         ++next;

         // info
         ErrorChecking::Info info;
         info.document = doc;
         info.currentSub = current;
         info.nextSub = next;
         info.previousSub = previous;
         info.tryToFix = true;

         if (error_checking_fix(checker, info))
            ++count;

         previous = current;
      }

      return count;
   }

   // FIXME
   void on_row_activated(const Gtk::TreePath& path, Gtk::TreeViewColumn*) {
      Gtk::TreeIter it = m_model->get_iter(path);

      Gtk::TreeModel::Row row = *it;

      // check if it's not a node
      if (row.children().empty()) {  // this is an error
         // If the fix succeeds, we need to remove it.
         if (fix_selected(it)) {
            Gtk::TreeModel::Row parent = *row.parent();

            m_model->erase(it);

            if (parent.children().empty())
               m_model->erase(parent);
            else
               update_node_label(parent);
         }
      } else {  // this is a node
         fix_row(row);
      }
   }

   // Update the label of the node.
   void update_node_label(const Gtk::TreeModel::Row row) {
      if (!row)
         return;

      unsigned int size = row.children().size();

      if (get_sort_type() == BY_CATEGORIES) {
         Glib::ustring label;

         ErrorChecking* checker = row[m_column.checker];

         if (checker)
            label = checker->get_label();

         row[m_column.text] = build_message(ngettext("%s (<b>1 error</b>)", "%s (<b>%d errors</b>)", size), label.c_str(), size);
      } else if (get_sort_type() == BY_SUBTITLES) {
         unsigned int num = utility::string_to_int(Glib::ustring(row[m_column.num]));
         row[m_column.text] =
            build_message(ngettext("Subtitle n°<b>%d</b> (<b>1 error</b>)", "Subtitle n°<b>%d</b> (<b>%d errors</b>)", size), num, size);
      }
   }

   // Show tooltip solution.
   bool on_query_tooltip(int x, int y, bool keyboard_tooltip, const Glib::RefPtr<Gtk::Tooltip>& tooltip) {
      Gtk::TreeIter iter;
      if (m_treeview->get_tooltip_context_iter(x, y, keyboard_tooltip, iter) == false)
         return false;

      Glib::ustring solution = (*iter)[m_column.solution];
      if (solution.empty())
         return false;

      tooltip->set_markup(solution);

      Gtk::TreePath path = m_model->get_path(iter);

      m_treeview->set_tooltip_row(tooltip, path);
      return true;
   }

   // Display the dialog preferences.
   void on_preferences() {
      ErrorCheckingGroup group;
      DialogErrorCheckingPreferences::create(*this, group);

      // Maybe some values have been changed, reset the config.
      m_checker_list.init_settings();

      refresh();
   }

  protected:
   SortType m_sort_type;
   Gtk::TreeView* m_treeview;
   Glib::RefPtr<Gtk::TreeStore> m_model;
   Column m_column;
   Gtk::Statusbar* m_statusbar;

   ErrorCheckingGroup m_checker_list;

   Glib::RefPtr<Gtk::ActionGroup> m_action_group;
};

// static instance of the dialog
DialogErrorChecking* DialogErrorChecking::m_static_instance = nullptr;

// Error Checking Plugin
class ErrorCheckingPlugin : public Action {
  public:
   ErrorCheckingPlugin() {
      activate();
      update_ui();
   }

   ~ErrorCheckingPlugin() {
      deactivate();
   }

   void activate() {
      se_dbg(SE_DBG_PLUGINS);

      // actions
      action_group = Gtk::ActionGroup::create("ErrorCheckingPlugin");

      action_group->add(Gtk::Action::create("error-checking", _("_Error Checking"), _("Launch the error-checking dialog")),
                        sigc::mem_fun(*this, &ErrorCheckingPlugin::on_error_checker));

      // ui
      Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

      ui_id = ui->new_merge_id();

      ui->insert_action_group(action_group);

      ui->add_ui(ui_id, "/menubar/menu-tools/checking", "error-checking", "error-checking");
   }

   void deactivate() {
      se_dbg(SE_DBG_PLUGINS);

      Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

      ui->remove_ui(ui_id);
      ui->remove_action_group(action_group);

      DialogErrorChecking* dialog = DialogErrorChecking::get_instance();
      if (dialog != nullptr)
         dialog->on_quit();
   }

   void update_ui() {
      se_dbg(SE_DBG_PLUGINS);

      bool visible = (get_current_document() != NULL);

      action_group->get_action("error-checking")->set_sensitive(visible);

      DialogErrorChecking* dialog = DialogErrorChecking::get_instance();
      if (dialog != NULL)
         dialog->on_current_document_changed(get_current_document());
   }

  protected:
   void on_error_checker() {
      DialogErrorChecking::create();
   }

  protected:
   Gtk::UIManager::ui_merge_id ui_id;
   Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(ErrorCheckingPlugin)
