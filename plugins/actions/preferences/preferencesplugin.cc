
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
#include <extension/action.h>
#include <gtkmm_utility.h>
#include <i18n.h>
#include <memory>
#include "documentpage.h"
#include "extensionpage.h"
#include "interfacepage.h"
#include "timingpage.h"
#include "videoplayerpage.h"
#include "waveformpage.h"

class DialogPreferences : public Gtk::Dialog {
 public:
  DialogPreferences(BaseObjectType *cobject,
                    const Glib::RefPtr<Gtk::Builder> &xml)
      : Gtk::Dialog(cobject) {
    utility::set_transient_parent(*this);

    InterfacePage *interface = nullptr;
    DocumentPage *document = nullptr;
    WaveformPage *waveform = nullptr;
    VideoPlayerPage *videoplayer = nullptr;
    TimingPage *timing = nullptr;
    ExtensionPage *extension = nullptr;

    xml->get_widget_derived("box-interface", interface);
    xml->get_widget_derived("box-document", document);
    xml->get_widget_derived("box-waveform", waveform);
    xml->get_widget_derived("box-video-player", videoplayer);
    xml->get_widget_derived("box-timing", timing);
    xml->get_widget_derived("box-extension", extension);
  }

  static void create() {
    std::unique_ptr<DialogPreferences> dialog(
        gtkmm_utility::get_widget_derived<DialogPreferences>(
            SE_DEV_VALUE(SE_PLUGIN_PATH_UI, SE_PLUGIN_PATH_DEV),
            "dialog-preferences.ui", "dialog-preferences"));

    dialog->run();
  }
};

// Error Checking Plugin
class PreferencesPlugin : public Action {
 public:
  PreferencesPlugin() {
    activate();
    update_ui();
  }

  ~PreferencesPlugin() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("PreferencesPlugin");

    action_group->add(
        Gtk::Action::create("preferences", Gtk::Stock::PREFERENCES, "",
                            _("Configure Subtitle Editor")),
        sigc::mem_fun(*this, &PreferencesPlugin::on_preferences));

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui_id = ui->new_merge_id();

    ui->insert_action_group(action_group);

    ui->add_ui(ui_id, "/menubar/menu-options/preferences", "preferences",
               "preferences");
  }

  void deactivate() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui->remove_ui(ui_id);
    ui->remove_action_group(action_group);
  }

 protected:
  void on_preferences() {
    DialogPreferences::create();
  }

 protected:
  Gtk::UIManager::ui_merge_id ui_id;
  Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(PreferencesPlugin)
