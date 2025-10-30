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

#include "styleeditor.h"

#include <color.h>
#include <documents.h>
#include <extension/action.h>
#include <gtkmm_utility.h>
#include <utility.h>

#include <memory>
//#include "gtkmm/messagedialog.h"

class ColumnNameRecorder : public Gtk::TreeModel::ColumnRecord {
 public:
  ColumnNameRecorder() {
    add(name);
  }
  Gtk::TreeModelColumn<Glib::ustring> name;
};

// During dialog initialization, we need to block signals that the document has changed,
// which triggered changing document's label by adding an asterisk to the filename to indicate that the document
// has changed
bool m_block_signals = false;

DialogStyleEditor::DialogStyleEditor(BaseObjectType *cobject,
                                     const Glib::RefPtr<Gtk::Builder> &builder)
    : Gtk::Dialog(cobject) {
  utility::set_transient_parent(*this);

#define init_widget(WidgetClass, widget_name, signal, callback, key)      \
  {                                                                       \
    builder->get_widget(widget_name, m_widgets[widget_name]);             \
    WidgetClass *w = dynamic_cast<WidgetClass *>(m_widgets[widget_name]); \
    w->signal().connect(                                                  \
        sigc::bind(sigc::mem_fun(*this, &DialogStyleEditor::callback), w, \
                   Glib::ustring(key)));                                  \
  }
  builder->get_widget("vbox-style", m_widgets["vbox-style"]);

  init_widget(Gtk::Button, "button-new-style", signal_clicked,
              callback_button_clicked, "new-style");
  init_widget(Gtk::Button, "button-delete-style", signal_clicked,
              callback_button_clicked, "delete-style");
  init_widget(Gtk::Button, "button-copy-style", signal_clicked,
              callback_button_clicked, "copy-style");
  init_widget(Gtk::Button, "button-set-default", signal_clicked,
              callback_button_clicked, "set-default");

  init_widget(Gtk::SpinButton, "spin-playres-x", signal_value_changed,
            callback_playres_changed, "playres-x");
  init_widget(Gtk::SpinButton, "spin-playres-y", signal_value_changed,
            callback_playres_changed, "playres-y");

  init_widget(Gtk::FontButton, "button-font", signal_font_set,
              callback_font_button_changed, "font");
  init_widget(Gtk::ToggleButton, "button-bold", signal_toggled,
              callback_button_toggled, "bold");
  init_widget(Gtk::ToggleButton, "button-italic", signal_toggled,
              callback_button_toggled, "italic");
  init_widget(Gtk::ToggleButton, "button-underline", signal_toggled,
              callback_button_toggled, "underline");
  init_widget(Gtk::ToggleButton, "button-strikeout", signal_toggled,
              callback_button_toggled, "strikeout");

  init_widget(Gtk::ColorButton, "button-primary-color", signal_color_set,
              callback_color_button, "primary-color");
  init_widget(Gtk::ColorButton, "button-secondary-color", signal_color_set,
              callback_color_button, "secondary-color");
  init_widget(Gtk::ColorButton, "button-outline-color", signal_color_set,
              callback_color_button, "outline-color");
  init_widget(Gtk::ColorButton, "button-shadow-color", signal_color_set,
              callback_color_button, "shadow-color");

  init_widget(Gtk::SpinButton, "spin-margin-l", signal_value_changed,
              callback_spin_value_changed, "margin-l");
  init_widget(Gtk::SpinButton, "spin-margin-r", signal_value_changed,
              callback_spin_value_changed, "margin-r");
  init_widget(Gtk::SpinButton, "spin-margin-v", signal_value_changed,
              callback_spin_value_changed, "margin-v");

  init_widget(Gtk::SpinButton, "spin-angle", signal_value_changed,
              callback_spin_value_changed, "angle");
  init_widget(Gtk::SpinButton, "spin-scale-x", signal_value_changed,
              callback_spin_value_changed, "scale-x");
  init_widget(Gtk::SpinButton, "spin-scale-y", signal_value_changed,
              callback_spin_value_changed, "scale-y");
  init_widget(Gtk::SpinButton, "spin-spacing", signal_value_changed,
              callback_spin_value_changed, "spacing");

  init_widget(Gtk::SpinButton, "spin-outline", signal_value_changed,
              callback_spin_value_changed, "outline");
  init_widget(Gtk::SpinButton, "spin-shadow", signal_value_changed,
              callback_spin_value_changed, "shadow");

  init_widget(Gtk::RadioButton, "radio-outline", signal_toggled,
              callback_radio_toggled, "outline");
  init_widget(Gtk::RadioButton, "radio-box-per-line", signal_toggled,
              callback_radio_toggled, "box-per-line");
  init_widget(Gtk::RadioButton, "radio-rectangular-box", signal_toggled,
              callback_radio_toggled, "rectangular-box");

  for (unsigned int i = 0; i < 9; ++i) {
    Glib::ustring b = build_message("button-alignment-%d", i + 1);
    builder->get_widget(b, m_widgets[b]);

    Gtk::RadioButton *w = dynamic_cast<Gtk::RadioButton *>(m_widgets[b]);
    w->signal_toggled().connect(sigc::bind(
        sigc::mem_fun(*this, &DialogStyleEditor::callback_alignment_changed), w,
        i + 1));
  }

  // create treeview (this is in braces to destroy the pointers after we do not need them)
  {
    Gtk::TreeViewColumn *column = NULL;
    Gtk::CellRendererText *renderer = NULL;
    ColumnNameRecorder column_name;

    builder->get_widget("treeview-style", m_widgets["treeview-style"]);

    m_liststore = Gtk::ListStore::create(column_name);

    m_treeview = dynamic_cast<Gtk::TreeView *>(m_widgets["treeview-style"]);
    m_treeview->set_model(m_liststore);

    column = manage(new Gtk::TreeViewColumn(_("Styles")));
    renderer = manage(new Gtk::CellRendererText);
    renderer->property_editable() = true;
    renderer->signal_edited().connect(
        sigc::mem_fun(*this, &DialogStyleEditor::on_style_name_edited));

    column->pack_start(*renderer, false);
    column->add_attribute(renderer->property_text(), column_name.name);

    m_treeview->append_column(*column);

    m_treeview->get_selection()->signal_changed().connect(sigc::mem_fun(
        *this, &DialogStyleEditor::callback_style_selection_changed));
    if (m_liststore->children().empty()) {
      m_widgets["vbox-style"]->set_sensitive(false);
    } else {
    m_treeview->get_selection()->select(m_liststore->children().begin());
    }
  }
}

void DialogStyleEditor::on_style_name_edited(const Glib::ustring &path,
                                             const Glib::ustring &text) {
  unsigned int num = utility::string_to_int(path);

  Style style = m_current_document->styles().get(num);
  if (style) {
    Gtk::TreeIter iter = m_treeview->get_model()->get_iter(path);
    ColumnNameRecorder column_name;

    (*iter)[column_name.name] = text;

    style.set("name", text);
    m_current_document->make_document_changed();
  }
}

void DialogStyleEditor::callback_button_clicked(Gtk::Button *,
                                                const Glib::ustring &action) {
  ScriptInfo &script_info = m_current_document->get_script_info();
  if (action == "new-style") {
    // If There are already styles and PlayRes is not set, ask user what to do as just adding it might corrupt their styles
    if (m_current_document->styles().size() > 0 &&  (!script_info.data.count("PlayResX") || !script_info.data.count("PlayResX"))) {
       m_current_document->flash_message(_("Problems"));
       const int RESPONSE_APPLY_DEFAULT = 1;
       Gtk::MessageDialog dialog(*this,
                                _("PlayResX or PlayResY not set"),
                                false,
                                Gtk::MESSAGE_INFO,
                                Gtk::BUTTONS_CANCEL);
       dialog.set_title(_("Missing PlayRes Information"));
       dialog.set_secondary_text("You tried to add a new style, but this document does not have target screen size set and already has styles.\n\n Either cancel (no style will be added then) and fix the situation manually.\n\nOr apply a default screen size (1920x1080). A new style will be added then, but it might make subtitles in the current styles look too big or too small");
       dialog.add_button(_("Apply default PlayRes"), RESPONSE_APPLY_DEFAULT);

       int result = dialog.run();
       if (result != RESPONSE_APPLY_DEFAULT)
           return;
       else {
           ASS::set_default_playres(script_info);
           dynamic_cast<Gtk::SpinButton *>(m_widgets["spin-playres-x"])
        ->set_sensitive(true);
           dynamic_cast<Gtk::SpinButton *>(m_widgets["spin-playres-y"])
        ->set_sensitive(true);
       }
    }
    // If There are no styles and PlayRes is not set, just add Playres
    if (m_current_document->styles().size() == 0 &&  (!script_info.data.count("PlayResX") || !script_info.data.count("PlayResX"))) {
        ASS::set_default_playres(script_info);
           dynamic_cast<Gtk::SpinButton *>(m_widgets["spin-playres-x"])
        ->set_sensitive(true);
           dynamic_cast<Gtk::SpinButton *>(m_widgets["spin-playres-y"])
        ->set_sensitive(true);
    }
    ColumnNameRecorder column;
    Gtk::TreeIter iter = m_liststore->append();

    (*iter)[column.name] = "Undefined";

    Style style = m_current_document->styles().append();
    ASS::set_default_style(style);
    style.set("name", "Undefined");

    // Select the new style and start editing
    m_treeview->get_selection()->select(iter);
    Gtk::TreePath path = m_liststore->get_path(iter);
    Gtk::TreeViewColumn* col = m_treeview->get_column(0);
    m_treeview->set_cursor(path, *col, true);
    m_current_document->make_document_changed();

  } else if (action == "delete-style") {
    if (m_current_style) {
      m_current_document->styles().remove(m_current_style);

      Gtk::TreeIter iter = m_treeview->get_selection()->get_selected();
      m_liststore->erase(iter);
      m_current_document->make_document_changed();
    }
  } else if (action == "copy-style") {
    if (m_current_style) {
      Style new_style = m_current_document->styles().append();

      m_current_style.copy_to(new_style);
      new_style.set("name", new_style.get("name") + " (Copy)");
      //
      ColumnNameRecorder column;
      Gtk::TreeIter iter = m_liststore->append();
      (*iter)[column.name] = new_style.get("name");

      m_treeview->get_selection()->select(iter);

      // Start editing the name immediately
      Gtk::TreePath path = m_liststore->get_path(iter);
      Gtk::TreeViewColumn* col = m_treeview->get_column(0);
      m_treeview->set_cursor(path, *col, true);
      m_current_document->make_document_changed();
    }
  } else if (action == "set-default") {
    if (m_current_style) {
      Glib::ustring default_style = ASS::style_to_string(m_current_style);
      cfg::set_string("AdvancedSubStationAlpha", "default-style", default_style);
      cfg::set_comment("AdvancedSubStationAlpha", "default-style",
                         "Default style to be used");
      // We save PlayRes only if it is set (it can only be unset when a file without it is opened)
      bool playres_saved_to_config = false;
      if (script_info.data.count("PlayResX")) {
        Glib::ustring playres_x = script_info.data["PlayResX"];
        cfg::set_string("AdvancedSubStationAlpha", "default-playres-x",playres_x);
        playres_saved_to_config = true;
      }
      if (script_info.data.count("PlayResY")) {
        Glib::ustring playres_y = script_info.data["PlayResY"];
        cfg::set_string("AdvancedSubStationAlpha", "default-playres-y",playres_y);
        playres_saved_to_config = true;
      }
      if (playres_saved_to_config)
        m_current_document->flash_message(_("Default Style and PlayRes saved to configuration file"));
      else
        m_current_document->flash_message(_("Default Style saved to configuration file"));
    }
  }
}

void DialogStyleEditor::callback_font_button_changed(Gtk::FontButton *w,
                                                     const Glib::ustring &) {
  if (!m_current_style || m_block_signals)
    return;

  Pango::FontDescription description(w->get_font_name());

  Glib::ustring font_name = description.get_family();
  Glib::ustring font_size = to_string(description.get_size() / 1000);

  m_current_style.set("font-name", font_name);
  m_current_style.set("font-size", font_size);
  m_current_document->make_document_changed();
}

void DialogStyleEditor::callback_button_toggled(Gtk::ToggleButton *w,
                                                const Glib::ustring &key) {
  if (!m_current_style || m_block_signals)
    return;

  m_current_style.set(key, to_string(w->get_active()));
  m_current_document->make_document_changed();
}

void DialogStyleEditor::callback_spin_value_changed(Gtk::SpinButton *w,
                                                    const Glib::ustring &key) {
  if (!m_current_style || m_block_signals)
    return;

  m_current_style.set(key, to_string(w->get_value()));
  m_current_document->make_document_changed();
}

void DialogStyleEditor::callback_radio_toggled(Gtk::RadioButton *w,
                                               const Glib::ustring &key) {
  if (!m_current_style || m_block_signals)
    return;

  if (w->get_active()) {
    if (key == "outline")
      m_current_style.set("border-style", "1");
    else if (key == "box-per-line")
      m_current_style.set("border-style", "3");
    else if (key == "rectangular-box")
      m_current_style.set("border-style", "4");
    m_current_document->make_document_changed();
  }
}
void DialogStyleEditor::callback_color_button(Gtk::ColorButton *w,
                                              const Glib::ustring &key) {
  if (!m_current_style)
    return;

  Color color;
  color.getFromColorButton(*w);

  m_current_style.set(key, color.to_string());
  m_current_document->make_document_changed();
}

void DialogStyleEditor::callback_style_selection_changed() {
  Gtk::TreeIter iter = m_treeview->get_selection()->get_selected();
  if (iter) {
    unsigned int num =
        utility::string_to_int(m_treeview->get_model()->get_string(iter));

    Style style = m_current_document->styles().get(num);

    init_style(style);
  } else {
    // null
    init_style(Style());
  }
}

void DialogStyleEditor::callback_alignment_changed(Gtk::RadioButton *w,
                                                   unsigned int num) {
  if (!m_current_style || m_block_signals)
    return;

  if (w->get_active())
    m_current_style.set("alignment", to_string(num));
  m_current_document->make_document_changed();
}

void DialogStyleEditor::callback_playres_changed(Gtk::SpinButton *w,
const Glib::ustring &key) {
  if (!m_current_document || m_block_signals)
    return;

  ScriptInfo &script_info = m_current_document->get_script_info();

  if (key == "playres-x") {
    script_info.data["PlayResX"] = to_string(static_cast<int>(w->get_value()));
  } else if (key == "playres-y") {
    script_info.data["PlayResY"] = to_string(static_cast<int>(w->get_value()));
  }
  m_current_document->make_document_changed();
}


void DialogStyleEditor::init_style(const Style &style) {
  se_dbg_msg(SE_DBG_PLUGINS, ("init_style: " + (style ? style.get("name") : "null")).c_str());
  m_block_signals = true;
  m_current_style = style;
  // Load PlayRes always (not just when style is selected)
  if (m_current_document) {
    ScriptInfo &script_info = m_current_document->get_script_info();

    int playres_x = 0;
    int playres_y = 0;

    if (script_info.data.count("PlayResX")) {
        playres_x = utility::string_to_int(script_info.data["PlayResX"]);
        dynamic_cast<Gtk::SpinButton *>(m_widgets["spin-playres-x"])
          ->set_value(playres_x);
    } else {
        dynamic_cast<Gtk::SpinButton *>(m_widgets["spin-playres-x"])
        ->set_sensitive(false);
    }

    if (script_info.data.count("PlayResY")) {
        playres_y = utility::string_to_int(script_info.data["PlayResY"]);
        dynamic_cast<Gtk::SpinButton *>(m_widgets["spin-playres-y"])
          ->set_value(playres_y);
    } else {
        dynamic_cast<Gtk::SpinButton *>(m_widgets["spin-playres-y"])
          ->set_sensitive(false);
    }
  }

  m_widgets["vbox-style"]->set_sensitive((m_current_style));

  if (!m_current_style) {
    return;
  }

#define init_toggle_button(name, key)                \
  dynamic_cast<Gtk::ToggleButton *>(m_widgets[name]) \
      ->set_active(utility::string_to_bool(style.get(key)));
#define init_spin_button(name, key)                \
  dynamic_cast<Gtk::SpinButton *>(m_widgets[name]) \
      ->set_value(utility::string_to_double(style.get(key)));
#define init_color_button(name, key)                                           \
  {                                                                            \
    Color color(style.get(key));                                               \
    color.initColorButton(*dynamic_cast<Gtk::ColorButton *>(m_widgets[name])); \
  }

  // font
  {
    Glib::ustring font = m_current_style.get("font-name") + " " +
                         m_current_style.get("font-size");
    dynamic_cast<Gtk::FontButton *>(m_widgets["button-font"])
        ->set_font_name(font);
  }

  init_toggle_button("button-bold", "bold");
  init_toggle_button("button-italic", "italic");
  init_toggle_button("button-underline", "underline");
  init_toggle_button("button-strikeout", "strikeout");

  init_color_button("button-primary-color", "primary-color");
  init_color_button("button-secondary-color", "secondary-color");
  init_color_button("button-outline-color", "outline-color");
  init_color_button("button-shadow-color", "shadow-color");

  init_spin_button("spin-margin-l", "margin-l");
  init_spin_button("spin-margin-r", "margin-r");
  init_spin_button("spin-margin-v", "margin-v");

  init_spin_button("spin-angle", "angle");
  init_spin_button("spin-scale-x", "scale-x");
  init_spin_button("spin-scale-y", "scale-y");
  init_spin_button("spin-spacing", "spacing");

  init_spin_button("spin-outline", "outline");
  init_spin_button("spin-shadow", "shadow");

  // border style
  {
    Glib::ustring border_style = m_current_style.get("border-style");
    if (border_style == "1")
      dynamic_cast<Gtk::RadioButton *>(m_widgets["radio-outline"])
          ->set_active(true);
    else if (border_style == "4")
      dynamic_cast<Gtk::RadioButton *>(m_widgets["radio-rectangular-box"])
          ->set_active(true);
    else
      dynamic_cast<Gtk::RadioButton *>(m_widgets["radio-box-per-line"])
          ->set_active(true);
  }
  // alignment
  {
    Glib::ustring num = m_current_style.get("alignment");

    dynamic_cast<Gtk::RadioButton *>(m_widgets["button-alignment-" + num])
        ->set_active(true);
  }
  m_block_signals = false;
}

void DialogStyleEditor::execute(Document *doc) {
  g_return_if_fail(doc);

  m_current_document = doc;

  {
    ColumnNameRecorder column_name;

    // add styles
    m_current_document = se::documents::active();

    for (Style style = m_current_document->styles().first(); style; ++style) {
      Gtk::TreeIter iter = m_liststore->append();

      (*iter)[column_name.name] = style.get("name");
    }

    if (m_liststore->children().empty()) {
      m_widgets["vbox-style"]->set_sensitive(false);
    } else {
      m_treeview->get_selection()->select(m_liststore->children().begin());
    }
  }

  run();
}

// Register Plugin
class StyleEditorPlugin : public Action {
 public:
  StyleEditorPlugin() {
    activate();
    update_ui();
  }

  ~StyleEditorPlugin() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("StyleEditorPlugin");

    action_group->add(
        Gtk::Action::create("style-editor", Gtk::Stock::SELECT_COLOR,
                            _("_Style Editor"), _("Launch the style editor")),
        sigc::mem_fun(*this, &StyleEditorPlugin::on_execute));

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui_id = ui->new_merge_id();

    ui->insert_action_group(action_group);

    ui->add_ui(ui_id, "/menubar/menu-tools/style-editor", "style-editor",
               "style-editor");
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

    action_group->get_action("style-editor")->set_sensitive(visible);
  }

 protected:
   void on_execute() {
     se_dbg(SE_DBG_PLUGINS);

     Document *doc = get_current_document();
     g_return_if_fail(doc);

     // create dialog
     std::unique_ptr<DialogStyleEditor> dialog(
         gtkmm_utility::get_widget_derived<DialogStyleEditor>(
             SE_DEV_VALUE(SE_PLUGIN_PATH_UI, SE_PLUGIN_PATH_DEV),
             "dialog-style-editor.ui", "dialog-style-editor"));

     dialog->execute(doc);
   }

 protected:
   Gtk::UIManager::ui_merge_id ui_id;
   Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(StyleEditorPlugin)
