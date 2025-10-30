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

#include "styleeditor.h"

#include <color.h>
#include <documents.h>
#include <extension/action.h>
#include <gtkmm_utility.h>
#include <utility.h>

#include <memory>

class ColumnNameRecorder : public Gtk::TreeModel::ColumnRecord {
 public:
  ColumnNameRecorder() {
    add(name);
  }
  Gtk::TreeModelColumn<Glib::ustring> name;
};

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
  init_widget(Gtk::RadioButton, "radio-opaque-box", signal_toggled,
              callback_radio_toggled, "opaque-box");

  for (unsigned int i = 0; i < 9; ++i) {
    Glib::ustring b = build_message("button-alignment-%d", i + 1);
    builder->get_widget(b, m_widgets[b]);

    Gtk::RadioButton *w = dynamic_cast<Gtk::RadioButton *>(m_widgets[b]);
    w->signal_toggled().connect(sigc::bind(
        sigc::mem_fun(*this, &DialogStyleEditor::callback_alignment_changed), w,
        i + 1));
  }

  // create treeview
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
  }
}

void DialogStyleEditor::callback_button_clicked(Gtk::Button *,
                                                const Glib::ustring &action) {
  if (action == "new-style") {
    ColumnNameRecorder column;
    Gtk::TreeIter iter = m_liststore->append();

    (*iter)[column.name] = "Undefined";

    Style style = m_current_document->styles().append();
    style.set("name", "Undefined");
  } else if (action == "delete-style") {
    if (m_current_style) {
      m_current_document->styles().remove(m_current_style);

      Gtk::TreeIter iter = m_treeview->get_selection()->get_selected();
      m_liststore->erase(iter);
    }
  } else if (action == "copy-style") {
    if (m_current_style) {
      Style new_style = m_current_document->styles().append();

      m_current_style.copy_to(new_style);
      new_style.set("name", new_style.get("name") + "#");
      //
      ColumnNameRecorder column;
      Gtk::TreeIter iter = m_liststore->append();
      (*iter)[column.name] = new_style.get("name");

      m_treeview->get_selection()->select(iter);
    }
  }
}

void DialogStyleEditor::callback_font_button_changed(Gtk::FontButton *w,
                                                     const Glib::ustring &) {
  if (!m_current_style)
    return;

  Pango::FontDescription description(w->get_font_name());

  Glib::ustring font_name = description.get_family();
  Glib::ustring font_size = to_string(description.get_size() / 1000);

  m_current_style.set("font-name", font_name);
  m_current_style.set("font-size", font_size);
}

void DialogStyleEditor::callback_button_toggled(Gtk::ToggleButton *w,
                                                const Glib::ustring &key) {
  if (!m_current_style)
    return;

  m_current_style.set(key, to_string(w->get_active()));
}

void DialogStyleEditor::callback_spin_value_changed(Gtk::SpinButton *w,
                                                    const Glib::ustring &key) {
  if (!m_current_style)
    return;

  m_current_style.set(key, to_string(w->get_value()));
}

void DialogStyleEditor::callback_radio_toggled(Gtk::RadioButton *w,
                                               const Glib::ustring &key) {
  if (!m_current_style)
    return;

  if (w->get_active()) {
    if (key == "outline")
      m_current_style.set("border-style", "1");
    else if (key == "opaque-box")
      m_current_style.set("border-style", "3");
  }
}
void DialogStyleEditor::callback_color_button(Gtk::ColorButton *w,
                                              const Glib::ustring &key) {
  if (!m_current_style)
    return;

  Color color;
  color.getFromColorButton(*w);

  m_current_style.set(key, color.to_string());
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
  if (!m_current_style)
    return;

  if (w->get_active())
    m_current_style.set("alignment", to_string(num));
}

void DialogStyleEditor::init_style(const Style &style) {
  std::cout << "init_style: " << ((style) ? style.get("name") : "null")
            << std::endl;
  m_current_style = style;

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
    else
      dynamic_cast<Gtk::RadioButton *>(m_widgets["radio-opaque-box"])
          ->set_active(true);
  }
  // alignment
  {
    Glib::ustring num = m_current_style.get("alignment");

    dynamic_cast<Gtk::RadioButton *>(m_widgets["button-alignment-" + num])
        ->set_active(true);
  }
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
