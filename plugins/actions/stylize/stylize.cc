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

class StylizeSelectedSubtitlesPlugin : public Action {
 public:
  StylizeSelectedSubtitlesPlugin() {
    activate();
    update_ui();
  }

  ~StylizeSelectedSubtitlesPlugin() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("StylizeSelectedSubtitlesPlugin");

    action_group->add(
        Gtk::Action::create("stylize-selected-subtitles", _("_Stylize"),
                            _("Apply style to the selected subtitles (note that this will get saved only when using style-supporting format such as ASS)")));

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui->insert_action_group(action_group);

    Glib::ustring submenu = R"(
      <ui>
        <menubar name='menubar'>
          <menu name='menu-edit' action='menu-edit'>
            <placeholder name='text-formatting'>
              <menu name='stylize-selected-subtitles' action='stylize-selected-subtitles'>
                <separator/>
                <placeholder name='stylize-selected-subtitles-placeholder'/>
              </menu>
            </placeholder>
          </menu>
        </menubar>
      </ui>
    )";

    ui_id = ui->add_ui_from_string(submenu);

    se::documents::signal_active_changed().connect(sigc::mem_fun(
        *this, &StylizeSelectedSubtitlesPlugin::on_active_changed));

    se::documents::signal_modified().connect(sigc::mem_fun(
        *this, &StylizeSelectedSubtitlesPlugin::on_document_signals));

    rebuild_styles_menu();
  }

  void deactivate() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui->remove_ui(ui_id_styles);
    ui->remove_action_group(action_group_styles);

    ui->remove_ui(ui_id);
    ui->remove_action_group(action_group);
  }

  void update_ui() {
    se_dbg(SE_DBG_PLUGINS);

    bool visible = (get_current_document() != NULL);

    action_group->get_action("stylize-selected-subtitles")
        ->set_sensitive(visible);
  }

 protected:
  void on_document_signals(Document *, const std::string &signal) {
    if (signal == "style-changed")
      rebuild_styles_menu();
    else if (signal == "style-inserted")
      rebuild_styles_menu();
    else if (signal == "style-removed")
      rebuild_styles_menu();
  }

  void on_active_changed(Document *doc) {
    rebuild_styles_menu();
  }

  void rebuild_styles_menu() {
    if (action_group_styles) {
      get_ui_manager()->remove_ui(ui_id_styles);
      get_ui_manager()->remove_action_group(action_group_styles);
    }

    action_group_styles =
        Gtk::ActionGroup::create("StylizeSelectedSubtitlesPluginStyles");
    get_ui_manager()->insert_action_group(action_group_styles);

    ui_id_styles = get_ui_manager()->new_merge_id();

    build_styles_menu();

    get_ui_manager()->ensure_update();
  }

  void build_styles_menu() {
    Document *doc = get_current_document();
    if (doc == NULL)
      return;

    guint count = 0;
    for (Style style = doc->styles().first(); style; ++style, ++count) {
      Glib::ustring action_name =
          Glib::ustring::compose("stylize-selected-subtitles-style-%1", count);
      Glib::ustring action_label = style.get("name");

      action_group_styles->add(
          Gtk::Action::create(action_name, action_label),
          sigc::bind(
              sigc::mem_fun(
                  *this,
                  &StylizeSelectedSubtitlesPlugin::apply_style_to_selection),
              style.get("name")));

      get_ui_manager()->add_ui(
          ui_id_styles,
          "/menubar/menu-edit/text-formatting/stylize-selected-subtitles/"
          "stylize-selected-subtitles-placeholder/",
          action_name, action_name, Gtk::UI_MANAGER_MENUITEM, false);
    }
  }

  void apply_style_to_selection(const Glib::ustring &name) {
    Document *doc = get_current_document();
    std::vector<Subtitle> selection = doc->subtitles().get_selection();
    if (selection.empty())
      return;

    doc->start_command(_("Set style to selection"));
    for (guint i = 0; i < selection.size(); ++i)
      selection[i].set("style", name);
    doc->finish_command();
  }

 protected:
  Gtk::UIManager::ui_merge_id ui_id;
  Glib::RefPtr<Gtk::ActionGroup> action_group;

  Gtk::UIManager::ui_merge_id ui_id_styles;
  Glib::RefPtr<Gtk::ActionGroup> action_group_styles;
};

REGISTER_EXTENSION(StylizeSelectedSubtitlesPlugin)
