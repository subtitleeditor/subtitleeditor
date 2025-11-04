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
#include <gui/comboboxtextcolumns.h>
#include <gui/dialogutility.h>
#include <utility.h>
#include <memory>

// TODO Add FPS finder
// (new_sub_time x fps_video) / old_sub_time = fps_src

class DialogChangeFramerate : public DialogActionMultiDoc {
  class ComboBoxEntryText : public Gtk::ComboBoxText {
   public:
    ComboBoxEntryText(BaseObjectType *cobject,
                      const Glib::RefPtr<Gtk::Builder> &)
        : Gtk::ComboBoxText(cobject) {
      get_entry()->signal_focus_out_event().connect(
          sigc::mem_fun(*this, &ComboBoxEntryText::on_focus_out));
    }

    // Add new value to the combobox, only if it doesn't already there
    void append_text(const Glib::ustring &text) {
      Gtk::TreeNodeChildren rows = get_model()->children();

      ComboBoxTextColumns m_columns;

      for (auto it = rows.begin(); it; ++it) {
        if ((*it)[m_columns.m_col_name] == text) {
          set_active(it);
          return;
        }
      }
      Gtk::ComboBoxText::append(text);
    }

    // Add the value if it's a number when the focus out
    bool on_focus_out(GdkEventFocus *) {
      Glib::ustring text = get_entry()->get_text();

      double value = 0;
      if (from_string(text, value)) {
        if (value > 0)
          append_text(to_string(value));
        else
          set_active(0);
      } else {
        set_active(0);
      }
      return true;
    }
  };

 public:
  DialogChangeFramerate(BaseObjectType *cobject,
                        const Glib::RefPtr<Gtk::Builder> &xml)
      : DialogActionMultiDoc(cobject, xml) {
    utility::set_transient_parent(*this);

    xml->get_widget_derived("combo-src", m_comboSrc);
    xml->get_widget_derived("combo-dest", m_comboDest);

    m_comboSrc->get_entry()->signal_activate().connect(
        sigc::bind<ComboBoxEntryText *>(
            sigc::mem_fun(*this, &DialogChangeFramerate::combo_activate),
            m_comboSrc));
    m_comboDest->get_entry()->signal_activate().connect(
        sigc::bind<ComboBoxEntryText *>(
            sigc::mem_fun(*this, &DialogChangeFramerate::combo_activate),
            m_comboDest));

    m_comboSrc->append_text(to_string(23.976));
    m_comboSrc->append_text(to_string(24.000));
    m_comboSrc->append_text(to_string(25.000));
    m_comboSrc->append_text(to_string(29.970));

    m_comboDest->append_text(to_string(23.976));
    m_comboDest->append_text(to_string(24.000));
    m_comboDest->append_text(to_string(25.000));
    m_comboDest->append_text(to_string(29.970));

    m_comboSrc->set_active(0);
    m_comboDest->set_active(1);

    set_default_response(Gtk::RESPONSE_OK);
  }

  void execute() {
    show();

    if (run() == Gtk::RESPONSE_OK) {
      DocumentList docs;

      if (apply_to_all_documents()) {
        docs = SubtitleEditorWindow::get_instance()->get_documents();
      } else {
        Document *doc =
            SubtitleEditorWindow::get_instance()->get_current_document();
        docs.push_back(doc);
      }

      double src = get_value(m_comboSrc);
      double dest = get_value(m_comboDest);

      if (src != 0 && dest != 0) {
        for (const auto &d : docs) {
          signal_change_framerate(d, src, dest);
        }
      }
    }

    hide();
  }

  sigc::signal<void, Document *, double, double> signal_change_framerate;

 protected:
  double get_value(ComboBoxEntryText *combo) {
    Glib::ustring text = combo->get_entry()->get_text();

    double value = 0;

    if (from_string(text, value))
      return value;
    return 0;
  }

  void combo_activate(ComboBoxEntryText *combo) {
    Glib::ustring text = combo->get_entry()->get_text();

    double value = 0;
    if (from_string(text, value)) {
      if (value > 0) {
        combo->append_text(to_string(value));
        combo->set_active_text(to_string(value));
        return;
      }
    }

    combo->set_active(0);
  }

 protected:
  ComboBoxEntryText *m_comboSrc;
  ComboBoxEntryText *m_comboDest;
};

class ChangeFrameratePlugin : public Action {
 public:
  ChangeFrameratePlugin() {
    activate();
    update_ui();
  }

  ~ChangeFrameratePlugin() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("ChangeFrameratePlugin");

    action_group->add(
          Gtk::Action::create(
            "change-framerate", Gtk::Stock::CONVERT, _("Change _Framerate"),
            _("Convert subtitles synced to video that has one framerate to a video with a different framerate")),
        sigc::mem_fun(*this, &ChangeFrameratePlugin::on_execute));

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui_id = ui->new_merge_id();

    ui->insert_action_group(action_group);

    ui->add_ui(ui_id, "/menubar/menu-timings/change-framerate",
               "change-framerate", "change-framerate");
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

    action_group->get_action("change-framerate")->set_sensitive(visible);
  }

 protected:
  void on_execute() {
    se_dbg(SE_DBG_PLUGINS);

    Document *doc = get_current_document();
    g_return_if_fail(doc);

    // create dialog
    std::unique_ptr<DialogChangeFramerate> dialog(
        gtkmm_utility::get_widget_derived<DialogChangeFramerate>(
            SE_DEV_VALUE(SE_PLUGIN_PATH_UI, SE_PLUGIN_PATH_DEV),
            "dialog-change-framerate.ui", "dialog-change-framerate"));

    dialog->signal_change_framerate.connect(
        sigc::mem_fun(*this, &ChangeFrameratePlugin::change_framerate));

    dialog->execute();
  }

  void change_framerate(Document *doc, double src_fps, double dest_fps) {
    se_dbg(SE_DBG_PLUGINS);

    g_return_if_fail(doc);

    doc->start_command(_("Change Framerate"));

    Subtitles subtitles = doc->subtitles();

    Subtitle subtitle = subtitles.get_first();

    while (subtitle) {
      SubtitleTime start = change_fps(subtitle.get_start(), src_fps, dest_fps);
      SubtitleTime end = change_fps(subtitle.get_end(), src_fps, dest_fps);

      subtitle.set_start_and_end(start, end);

      ++subtitle;
    }

    doc->emit_signal("subtitle-time-changed");
    doc->finish_command();

    doc->flash_message(_("The new framerate was applied. (%s to %s)"),
                       to_string(src_fps).c_str(), to_string(dest_fps).c_str());
  }

  SubtitleTime change_fps(const SubtitleTime &time, double src, double dest) {
    se_dbg(SE_DBG_PLUGINS);

    double frame = time.totalmsecs * src;
    double tot_sec = frame / dest;

    return SubtitleTime((long)tot_sec);
  }

 protected:
  Gtk::UIManager::ui_merge_id ui_id;
  Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(ChangeFrameratePlugin)
