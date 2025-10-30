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
#include <filereader.h>
#include <gtkmm.h>
#include <gtkmm_utility.h>
#include <utility.h>
#include "capitalizationpage.h"
#include "commonerrorpage.h"
#include "confirmationpage.h"
#include "hearingimpairedpage.h"
#include "patternmanager.h"
#include "taskspage.h"

class AssistantTextCorrection : public Gtk::Assistant {
 public:
  AssistantTextCorrection(BaseObjectType* cobject,
                          const Glib::RefPtr<Gtk::Builder>& builder)
      : Gtk::Assistant(cobject) {
    se_dbg(SE_DBG_PLUGINS);

    doc = SubtitleEditorWindow::get_instance()->get_current_document();

    builder->get_widget_derived("vbox-tasks", m_tasksPage);
    builder->get_widget_derived("vbox-comfirmation", m_comfirmationPage);

    add_tasks();

    se_dbg_msg(SE_DBG_PLUGINS, "Init tasks pages");
    // Init tasks pages
    for (int i = 0; i < get_n_pages(); ++i) {
      PatternsPage* page = dynamic_cast<PatternsPage*>(get_nth_page(i));
      if (page)
        m_tasksPage->add_task(page);
    }

    set_page_type(*get_nth_page(0), Gtk::ASSISTANT_PAGE_INTRO);
    set_page_type(*get_nth_page(get_n_pages() - 1),
                  Gtk::ASSISTANT_PAGE_CONFIRM);
  }

  ~AssistantTextCorrection() {
    se_dbg(SE_DBG_PLUGINS);
  }

  void add_tasks() {
    se_dbg(SE_DBG_PLUGINS);

    add_page(manage(new HearingImpairedPage), 1);
    add_page(manage(new CommonErrorPage), 2);
    add_page(manage(new CapitalizationPage), 3);
  }

  void add_page(PatternsPage* page, unsigned int pos) {
    se_dbg_msg(SE_DBG_PLUGINS, "new task page '%s' to the position '%d'",
               page->get_page_title().c_str(), pos);

    insert_page(*page, pos);
    set_page_title(*page, page->get_page_title());
    set_page_type(*page, Gtk::ASSISTANT_PAGE_CONTENT);
  }

  // Catch the comfirmation page and initialize with the current document
  // and patterns available.
  void on_prepare(Gtk::Widget* page) {
    se_dbg(SE_DBG_PLUGINS);

    AssistantPage* ap = dynamic_cast<AssistantPage*>(page);
    if (ap && ap == m_comfirmationPage) {
      bool res = m_comfirmationPage->comfirme(doc, get_patterns());
      set_page_complete(*page, true);
      set_page_title(*page, m_comfirmationPage->get_page_title());
      // No change, only display the close button
      if (!res)
        set_page_type(*m_comfirmationPage, Gtk::ASSISTANT_PAGE_SUMMARY);
    } else {
      set_page_complete(*page, true);
    }
  }

  // Return all patterns activated.
  std::list<Pattern*> get_patterns() {
    se_dbg(SE_DBG_PLUGINS);

    std::list<Pattern*> patterns;

    for (int i = 0; i < get_n_pages(); ++i) {
      PatternsPage* page = dynamic_cast<PatternsPage*>(get_nth_page(i));
      if (page == NULL)
        continue;
      if (page->is_enable() == false)
        continue;

      std::list<Pattern*> p = page->get_patterns();
      patterns.merge(p);
    }
    return patterns;
  }

  // Apply the change and destroy the window.
  void on_apply() {
    se_dbg(SE_DBG_PLUGINS);

    m_comfirmationPage->apply(doc);

    save_cfg();
  }

  // Destroy the window.
  void on_cancel() {
    se_dbg(SE_DBG_PLUGINS);

    save_cfg();

    // destroy_();
    delete this;
  }

  // Close signal, destroy the window.
  void on_close() {
    se_dbg(SE_DBG_PLUGINS);

    save_cfg();

    // destroy_();
    delete this;
  }

  // Save the configuration for each pages.
  void save_cfg() {
    se_dbg(SE_DBG_PLUGINS);

    for (int i = 0; i < get_n_pages(); ++i) {
      PatternsPage* page = dynamic_cast<PatternsPage*>(get_nth_page(i));
      if (page != NULL)
        page->save_cfg();
    }
  }

 protected:
  TasksPage* m_tasksPage;
  ComfirmationPage* m_comfirmationPage;
  Document* doc;
};

class TextCorrectionPlugin : public Action {
 public:
  TextCorrectionPlugin() {
    activate();
    update_ui();
  }

  ~TextCorrectionPlugin() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("TextCorrectionPlugin");
    action_group->add(
        Gtk::Action::create("text-correction", _("Text _Correction")),
        sigc::mem_fun(*this, &TextCorrectionPlugin::on_execute));

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui_id = ui->new_merge_id();
    ui->insert_action_group(action_group);
    ui->add_ui(ui_id, "/menubar/menu-tools/checking", "text-correction",
               "text-correction");
  }

  void deactivate() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui->remove_ui(ui_id);
    ui->remove_action_group(action_group);
  }

  void update_ui() {
    se_dbg(SE_DBG_PLUGINS);

    bool visible = (get_current_document() != NULL);

    action_group->get_action("text-correction")->set_sensitive(visible);
  }

  void on_execute() {
    // create dialog
    AssistantTextCorrection* assistant =
        gtkmm_utility::get_widget_derived<AssistantTextCorrection>(
            SE_DEV_VALUE(SE_PLUGIN_PATH_UI, SE_PLUGIN_PATH_DEV),
            "assistant-text-correction.ui", "assistant");
    assistant->show();
  }

 protected:
  Gtk::UIManager::ui_merge_id ui_id;
  Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(TextCorrectionPlugin)
