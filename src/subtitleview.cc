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

#include <gdk/gdkkeysyms.h>
#include <iostream>

#include "cfg.h"
#include "document.h"
#include "subtitleeditorwindow.h"
#include "subtitlemodel.h"
#include "subtitles.h"
#include "subtitleview.h"
#include "utility.h"

#include "gui/cellrenderercustom.h"
#include "gui/textviewcell.h"

class TimeCell : public Gtk::CellEditable, public Gtk::TextView {
 public:
  TimeCell() : Glib::ObjectBase(typeid(TimeCell)), Gtk::CellEditable() {
    se_dbg(SE_DBG_VIEW);
  }

  Glib::ustring get_text() {
    se_dbg(SE_DBG_VIEW);

    Glib::RefPtr<Gtk::TextBuffer> buffer = get_buffer();

    Gtk::TextBuffer::iterator start, end;

    buffer->get_bounds(start, end);
    return buffer->get_text(start, end);
  }

  void set_text(const Glib::ustring &text) {
    se_dbg_msg(SE_DBG_VIEW, "text=<%s>", text.c_str());

    get_buffer()->set_text(text);
  }

  // bug #23569 : Cursor cannot be moved with mouse when editing subtitles
  bool on_button_press_event(GdkEventButton *event) {
    se_dbg(SE_DBG_VIEW);

    Gtk::TextView::on_button_press_event(event);
    return true;
  }

 protected:
  bool on_key_press_event(GdkEventKey *event) {
    se_dbg(SE_DBG_VIEW);

    if (event->keyval == GDK_KEY_Escape) {
      // m_canceled = true;
      remove_widget();
      return true;
    }

    bool st_enter =
        (event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter ||
         event->keyval == GDK_KEY_ISO_Enter ||
         event->keyval == GDK_KEY_3270_Enter);

    if (st_enter) {
      editing_done();
      remove_widget();
      return true;
    }

    Gtk::TextView::on_key_press_event(event);
    return true;
  }

  bool on_scroll_event(GdkEventScroll *ev) {
    se_dbg(SE_DBG_VIEW);

    Glib::ustring text = get_text();
    long frame;

    if (SubtitleTime::validate(text)) {  // TIME
      SubtitleTime time(get_text());

      long step = 100;

      if (ev->state & GDK_SHIFT_MASK && ev->state & GDK_CONTROL_MASK)
        step *= 100;
      else if (ev->state & GDK_CONTROL_MASK)
        step *= 10;

      SubtitleTime val(step);

      if (ev->direction == GDK_SCROLL_UP) {
        time = time + val;
        set_text(time.str());
        return true;
      } else if (ev->direction == GDK_SCROLL_DOWN) {
        time = time - val;
        set_text(time.str());
        return true;
      }
    } else if (from_string(text, frame)) {  // FRAME
      long step = 1;

      if (ev->state & GDK_SHIFT_MASK && ev->state & GDK_CONTROL_MASK)
        step *= 100;
      else if (ev->state & GDK_CONTROL_MASK)
        step *= 10;

      if (ev->direction == GDK_SCROLL_UP)
        frame += step;
      else if (ev->direction == GDK_SCROLL_DOWN)
        frame -= step;

      set_text(to_string(frame));

      return true;
    }
    return false;
  }
};

// Basic cell renderer template
// Disable all actions at the beginning of the editing and
// enable the actions when it's finished.
// Support also the flash message.
template <class T>
class SubtitleViewCellRendererCustom : public CellRendererCustom<T> {
 public:
  explicit SubtitleViewCellRendererCustom(Document *doc)
      : CellRendererCustom<T>(), m_document(doc) {
    se_dbg(SE_DBG_VIEW);
  }

  virtual Gtk::CellEditable *start_editing_vfunc(
      GdkEvent *event, Gtk::Widget &widget, const Glib::ustring &path,
      const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area,
      Gtk::CellRendererState flags) {
    Gtk::CellEditable *editable = CellRendererCustom<T>::start_editing_vfunc(
        event, widget, path, background_area, cell_area, flags);

    // display flash message
    if (m_document)
      on_flash_message();

    return editable;
  }

  // If need to display flash message.
  virtual void on_flash_message() {
  }

 protected:
  // Enable or disable all actions so as not to interfere with editing.
  // As a simple shorcuts.
  void set_action_groups_sensitives(bool state) {
    if (cfg::get_boolean("subtitle-view",
                         "do-not-disable-actions-during-editing"))
      return;

    auto actions = SubtitleEditorWindow::get_instance()
                       ->get_ui_manager()
                       ->get_action_groups();

    for (const auto &action : actions) {
      action->set_sensitive(state);
    }
  }

  // Disable all actions.
  void begin_editing() {
    set_action_groups_sensitives(false);
  }

  // Enable all actions.
  void finish_editing() {
    set_action_groups_sensitives(true);
  }

 protected:
  Document *m_document;
};

// Represents a cell time.
class CellRendererTime : public SubtitleViewCellRendererCustom<TimeCell> {
 public:
  explicit CellRendererTime(Document *doc)
      : SubtitleViewCellRendererCustom<TimeCell>(doc) {
    property_editable() = true;
    property_yalign() = 0.0;
    property_xalign() = 1.0;
    property_alignment() = Pango::ALIGN_RIGHT;
  }
};

class CellRendererCPS : public Gtk::CellRendererText {
 public:
  CellRendererCPS()
      : Glib::ObjectBase(typeid(CellRendererCPS)), Gtk::CellRendererText() {
    property_yalign() = 0;
    // property_weight() = Pango::WEIGHT_ULTRALIGHT;
    property_xalign() = 1.0;
    property_alignment() = Pango::ALIGN_RIGHT;
  }
};

class CellRendererTextMultiline
    : public SubtitleViewCellRendererCustom<TextViewCell> {
 public:
  explicit CellRendererTextMultiline(Document *doc)
      : SubtitleViewCellRendererCustom<TextViewCell>(doc) {
    property_editable() = true;
    property_yalign() = 0.0;

    if (cfg::get_boolean("subtitle-view", "property-alignment-center")) {
      property_xalign() = 0.5;
      property_alignment() = Pango::ALIGN_CENTER;
    }
  }

  // Need to display a flash message for the behavior of line-break and exit.
  void on_flash_message() {
    if (cfg::get_boolean("subtitle-view",
                         "used-ctrl-enter-to-confirm-change")) {
      m_document->flash_message(
          _("Use Ctrl+Return for exit and Return for line-break"));
    } else {
      m_document->flash_message(
          _("Use Return for exit and Ctrl+Return for line-break"));
    }
  }
};

// SubtitleView Constructor
SubtitleView::SubtitleView(Document &doc) {
  m_refDocument = &doc;

  m_subtitleModel = m_refDocument->get_subtitle_model();
  m_styleModel = m_refDocument->m_styleModel;

  set_model(m_subtitleModel);

  createColumns();

  set_rules_hint(true);
  set_enable_search(false);
  set_search_column(m_column.num);

  // config
  loadCfg();

  get_selection()->signal_changed().connect(
      sigc::mem_fun(*this, &SubtitleView::on_selection_changed));

  get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);

  cfg::signal_changed("subtitle-view")
      .connect(
          sigc::mem_fun(*this, &SubtitleView::on_config_subtitle_view_changed));

  // DnD
  set_reorderable(true);

  // We need to update the view if the framerate of the document changed
  m_refDocument->get_signal("framerate-changed")
      .connect(sigc::mem_fun(*this, &SubtitleView::update_visible_range));

  // Update the columns size
  m_refDocument->get_signal("edit-timing-mode-changed")
      .connect(sigc::mem_fun(*this, &Gtk::TreeView::columns_autosize));

  // Setup my own copy of needed timing variables
  min_duration = cfg::get_int("timing", "min-display");
  max_cpl = cfg::get_int("timing", "max-characters-per-line");
  min_gap = cfg::get_int("timing", "min-gap-between-subtitles");
  min_cps = cfg::get_double("timing", "min-characters-per-second");
  max_cps = cfg::get_double("timing", "max-characters-per-second");

  check_timing = cfg::get_boolean("timing", "do-auto-timing-check");

  // keep trace of timing settings
  cfg::signal_changed("timing").connect(
      sigc::mem_fun(*this, &SubtitleView::on_config_timing_changed));
}

void SubtitleView::on_config_timing_changed(const Glib::ustring &key,
                                            const Glib::ustring &value) {
  if (key == "min-gap-between-subtitles")
    min_gap = utility::string_to_long(value);
  else if (key == "max-characters-per-line")
    max_cpl = utility::string_to_int(value);
  else if (key == "do-auto-timing-check")
    check_timing = utility::string_to_bool(value);
  else if (key == "min-display")
    min_duration = cfg::get_int("timing", "min-display");
  else if (key == "min-characters-per-second")
    min_cps = utility::string_to_double(value);
  else if (key == "max-characters-per-second")
    max_cps = utility::string_to_double(value);

  update_visible_range();
}

// Update the visible range
// We need to update after timing change or framerate change
void SubtitleView::update_visible_range() {
  // tell Gtk it should update all visible rows in the subtitle list
  Gtk::TreePath cur_path, end_path;
  if (get_visible_range(cur_path, end_path)) {
    while (cur_path <= end_path) {
      m_subtitleModel->row_changed(cur_path,
                                   m_subtitleModel->get_iter(cur_path));
      cur_path.next();
    }
  }
}

SubtitleView::~SubtitleView() {
}

void SubtitleView::loadCfg() {
  se_dbg(SE_DBG_VIEW);

  auto ers = cfg::get_boolean("subtitle-view", "enable-rubberband-selection");
  set_rubber_banding(ers);
}

void SubtitleView::set_tooltips(Gtk::TreeViewColumn *column,
                                const Glib::ustring &text) {
  se_dbg_msg(SE_DBG_VIEW, "[%s]=%s", column->get_title().c_str(), text.c_str());

  Gtk::Widget *widget = column->get_widget();
  if (widget)
    widget->set_tooltip_text(text);
}

// Return a new column (already manage) with Gtk::Label in title.
Gtk::TreeViewColumn *SubtitleView::create_treeview_column(
    const Glib::ustring &name) {
  Glib::ustring title = get_column_label_by_name(name);

  Gtk::TreeViewColumn *column = manage(new Gtk::TreeViewColumn);

  Gtk::Label *label = manage(
      new Gtk::Label(title));  //, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false));
  label->show();
  column->set_widget(*label);

  m_columns[name] = column;

  return column;
}

void SubtitleView::createColumns() {
  createColumnNum();
  createColumnLayer();
  createColumnStart();
  createColumnEnd();
  createColumnDuration();
  createColumnStyle();
  createColumnName();
  createColumnMarginR();
  createColumnMarginL();
  createColumnMarginV();
  createColumnEffect();
  createColumnText();
  createColumnCPS();
  createColumnTranslation();
  createColumnNote();

  update_columns_displayed_from_config();
}

// create columns
void SubtitleView::createColumnNum() {
  se_dbg(SE_DBG_VIEW);

  Gtk::TreeViewColumn *column = nullptr;
  Gtk::CellRendererText *renderer = nullptr;

  column = create_treeview_column("number");
  renderer = manage(new Gtk::CellRendererText);
  renderer->property_editable() = false;
  renderer->property_yalign() = 0;
  renderer->property_xalign() = 1.0;
  renderer->property_alignment() = Pango::ALIGN_RIGHT;

  column->pack_start(*renderer);
  column->add_attribute(renderer->property_text(), m_column.num);

  append_column(*column);

  set_tooltips(column, _("The line number"));
}

void SubtitleView::createColumnLayer() {
  se_dbg(SE_DBG_VIEW);

  Gtk::TreeViewColumn *column = nullptr;
  Gtk::CellRendererText *renderer = nullptr;

  column = create_treeview_column("layer");
  renderer = manage(new Gtk::CellRendererText);

  column->pack_start(*renderer, false);
  column->add_attribute(renderer->property_text(), m_column.layer);

  renderer->property_editable() = true;
  renderer->property_yalign() = 0;

  renderer->signal_edited().connect(
      sigc::mem_fun(*this, &SubtitleView::on_edited_layer));

  append_column(*column);

  // set_tooltips(column, _("Layer number."));
}

void SubtitleView::cps_data_func(const Gtk::CellRenderer *renderer,
                                 const Gtk::TreeModel::iterator &iter) {
  CellRendererTime *trenderer = (CellRendererTime *)renderer;
  Subtitle cur_sub(m_refDocument, iter);

  Glib::ustring color("black");  // default
  Glib::ustring cps_text_string =
      cur_sub.get_characters_per_second_text_string();
  if (check_timing) {
    const int cmp = cur_sub.check_cps_text(min_cps, max_cps);
    if (cmp > 0) {
      color = "red";
      cps_text_string = "<b>" + cps_text_string + "</b>";
    } else if (cmp < 0)
      color = "blue";
  }
  trenderer->property_markup() = Glib::ustring::compose(
      "<span foreground=\"%1\">%2</span>", color, cps_text_string);
}

void SubtitleView::cpl_text_data_func(const Gtk::CellRenderer *renderer,
                                      const Gtk::TreeModel::iterator &iter) {
  Gtk::CellRendererText *trenderer = (Gtk::CellRendererText *)renderer;
  Subtitle cur_sub(m_refDocument, iter);
  Glib::ustring cpl_text = cur_sub.get_characters_per_line_text();

  if (check_timing) {
    // Parse each line and build markup with colors per line
    std::istringstream ss(cpl_text);
    std::string line;
    std::vector<std::string> lines;

    while (std::getline(ss, line)) {
      int cpl = utility::string_to_int(line);

      if (cpl > max_cpl) {
        // This line exceeds limit - make it red
        lines.push_back(Glib::ustring::compose(
            "<span foreground=\"red\"><b>%1</b></span>", line));
      } else {
        // This line is OK - keep it black
        lines.push_back(Glib::ustring::compose(
            "<span foreground=\"black\" font-weight=\"200\">%1</span>", line));
      }
    }

    // Join lines with newline
    Glib::ustring markup;
    for (size_t i = 0; i < lines.size(); ++i) {
      if (i > 0)
        markup += "\n";
      markup += lines[i];
    }

    trenderer->property_markup() = markup;
  } else {
    // When check_timing is off, just display plain text
    trenderer->property_text() = cpl_text;
  }
}

void SubtitleView::duration_data_func(const Gtk::CellRenderer *renderer,
                                      const Gtk::TreeModel::iterator &iter) {
  CellRendererTime *trenderer = (CellRendererTime *)renderer;
  Subtitle cur_sub(m_refDocument, iter);

  Glib::ustring color;

  // Display text in red if the check timing option is enabled and
  // if the current subtitle don't respect the minimum duration
  if (check_timing) {
    if (cur_sub.get_duration().totalmsecs < min_duration)  // duration in msec
      color = "red";
  }

  trenderer->property_markup() = cur_sub.convert_value_to_time_string(
      (*iter)[m_column.duration_value], color);
}

void SubtitleView::start_time_data_func(const Gtk::CellRenderer *renderer,
                                        const Gtk::TreeModel::iterator &iter) {
  CellRendererTime *trenderer = (CellRendererTime *)renderer;
  Subtitle cur_sub(m_refDocument, iter);

  Glib::ustring color;

  // Display text in red if the check timing option is enabled and
  // if the current subtitle don't respect gap before subtitle
  if (check_timing) {
    if (cur_sub.check_gap_before(min_gap) == false)
      color = "red";
  }

  trenderer->property_markup() = cur_sub.convert_value_to_time_string(
      (*iter)[m_column.start_value], color);
}

void SubtitleView::end_time_data_func(const Gtk::CellRenderer *renderer,
                                      const Gtk::TreeModel::iterator &iter) {
  CellRendererTime *trenderer = (CellRendererTime *)renderer;
  Subtitle cur_sub(m_refDocument, iter);

  Glib::ustring color;

  // Display text in red if the check timing option is enabled and
  // if the current subtitle don't respect gap before subtitle
  if (check_timing) {
    if (cur_sub.check_gap_after(min_gap) == false)
      color = "red";
  }

  trenderer->property_markup() =
      cur_sub.convert_value_to_time_string((*iter)[m_column.end_value], color);
}

void SubtitleView::create_column_time(
    const Glib::ustring &name,
    const Gtk::TreeModelColumnBase & /*column_attribute*/,
    const sigc::slot<void, const Glib::ustring &, const Glib::ustring &>
        &slot_edited,
    const sigc::slot<void, const Gtk::CellRenderer *,
                     const Gtk::TreeModel::iterator &> &slot_cell_data_func,
    const Glib::ustring &tooltips) {
  se_dbg_msg(SE_DBG_VIEW, "name=%s tooltips=%s", name.c_str(),
             tooltips.c_str());

  CellRendererTime *renderer = manage(new CellRendererTime(m_refDocument));

  Gtk::TreeViewColumn *column = create_treeview_column(name);
  column->pack_start(*renderer);

  column->set_cell_data_func(*renderer, slot_cell_data_func);

  renderer->signal_edited().connect(slot_edited);

  append_column(*column);

  set_tooltips(column, tooltips);
}

void SubtitleView::createColumnStart() {
  create_column_time("start", m_column.start_value,
                     sigc::mem_fun(*this, &SubtitleView::on_edited_start),
                     sigc::mem_fun(*this, &SubtitleView::start_time_data_func),
                     _("When a subtitle appears on the screen."));
}

void SubtitleView::createColumnEnd() {
  create_column_time("end", m_column.end_value,
                     sigc::mem_fun(*this, &SubtitleView::on_edited_end),
                     sigc::mem_fun(*this, &SubtitleView::end_time_data_func),
                     _("When a subtitle disappears from the screen."));
}

void SubtitleView::createColumnDuration() {
  create_column_time("duration", m_column.duration_value,
                     sigc::mem_fun(*this, &SubtitleView::on_edited_duration),
                     sigc::mem_fun(*this, &SubtitleView::duration_data_func),
                     _("The duration of the subtitle."));
}

void SubtitleView::createColumnStyle() {
  se_dbg(SE_DBG_VIEW);

  Gtk::TreeViewColumn *column = nullptr;
  Gtk::CellRendererCombo *renderer = nullptr;

  column = create_treeview_column("style");
  renderer = manage(new Gtk::CellRendererCombo);

  column->pack_start(*renderer, false);
  column->add_attribute(renderer->property_text(), m_column.style);

  renderer->property_model() = m_styleModel;
  renderer->property_text_column() = 0;
  renderer->property_editable() = true;
  renderer->property_has_entry() = false;
  renderer->property_yalign() = 0;

  renderer->signal_edited().connect(
      sigc::mem_fun(*this, &SubtitleView::on_edited_style));

  append_column(*column);
}

void SubtitleView::createColumnName() {
  se_dbg(SE_DBG_VIEW);

  Gtk::TreeViewColumn *column = create_treeview_column("name");

  SubtitleViewCellRendererCustom<TextViewCell> *renderer =
      manage(new SubtitleViewCellRendererCustom<TextViewCell>(m_refDocument));

  column->pack_start(*renderer, false);
  column->add_attribute(renderer->property_text(), m_column.name);

  renderer->property_editable() = true;
  renderer->property_yalign() = 0;

  renderer->signal_edited().connect(
      sigc::mem_fun(*this, &SubtitleView::on_edited_name));

  append_column(*column);
}

void SubtitleView::createColumnCPS() {
  se_dbg(SE_DBG_VIEW);

  Gtk::TreeViewColumn *column = create_treeview_column("cps");

  CellRendererCPS *renderer = manage(new CellRendererCPS);

  column->pack_start(*renderer);

  column->set_cell_data_func(
      *renderer, sigc::mem_fun(*this, &SubtitleView::cps_data_func));

  append_column(*column);

  set_tooltips(column, _("The number of characters per second"));
}

void SubtitleView::createColumnText() {
  se_dbg(SE_DBG_VIEW);

  Gtk::TreeViewColumn *column = create_treeview_column("text");

  append_column(*column);

  // text
  {
    CellRendererTextMultiline *renderer =
        manage(new CellRendererTextMultiline(m_refDocument));

    column->pack_start(*renderer, true);
    column->add_attribute(renderer->property_text(), m_column.text);
    column->property_expand() = true;

    renderer->property_ellipsize() = Pango::ELLIPSIZE_END;
    renderer->signal_edited().connect(
        sigc::mem_fun(*this, &SubtitleView::on_edited_text));
  }
  // cpl
  {
    Gtk::CellRendererText *renderer = nullptr;
    renderer = manage(new Gtk::CellRendererText);

    column->pack_start(*renderer, false);
    column->set_cell_data_func(
        *renderer, sigc::mem_fun(*this, &SubtitleView::cpl_text_data_func));
    renderer->property_yalign() = 0;
    renderer->property_xalign() = 1.0;
    renderer->property_alignment() = Pango::ALIGN_RIGHT;

    renderer->property_visible() =
        cfg::get_boolean("subtitle-view", "show-character-per-line");
  }

  column->set_resizable(true);
}

void SubtitleView::createColumnTranslation() {
  se_dbg(SE_DBG_VIEW);

  Gtk::TreeViewColumn *column = create_treeview_column("translation");

  // translation
  {
    CellRendererTextMultiline *renderer =
        manage(new CellRendererTextMultiline(m_refDocument));

    column->pack_start(*renderer, true);
    column->add_attribute(renderer->property_text(), m_column.translation);
    column->property_expand() = true;

    renderer->property_ellipsize() = Pango::ELLIPSIZE_END;
    append_column(*column);

    renderer->signal_edited().connect(
        sigc::mem_fun(*this, &SubtitleView::on_edited_translation));
  }
  // cpl
  {
    Gtk::CellRendererText *renderer = nullptr;
    renderer = manage(new Gtk::CellRendererText);

    column->pack_end(*renderer, false);
    column->add_attribute(renderer->property_text(),
                          m_column.characters_per_line_translation);
    renderer->property_yalign() = 0;
    renderer->property_weight() = Pango::WEIGHT_ULTRALIGHT;
    renderer->property_visible() =
        cfg::get_boolean("subtitle-view", "show-character-per-line");
  }

  column->set_resizable(true);
}

void SubtitleView::createColumnNote() {
  se_dbg(SE_DBG_VIEW);

  Gtk::TreeViewColumn *column = create_treeview_column("note");

  CellRendererTextMultiline *renderer =
      manage(new CellRendererTextMultiline(m_refDocument));

  column->pack_start(*renderer, false);
  column->add_attribute(renderer->property_text(), m_column.note);

  append_column(*column);

  renderer->signal_edited().connect(
      sigc::mem_fun(*this, &SubtitleView::on_edited_note));

  column->set_resizable(true);
}

void SubtitleView::createColumnEffect() {
  se_dbg(SE_DBG_VIEW);

  Gtk::TreeViewColumn *column = nullptr;
  Gtk::CellRendererText *renderer = nullptr;

  column = create_treeview_column("effect");
  renderer = manage(new Gtk::CellRendererText);

  column->pack_start(*renderer, false);
  column->add_attribute(renderer->property_text(), m_column.effect);

  append_column(*column);

  renderer->property_editable() = true;
  renderer->property_yalign() = 0;

  renderer->signal_edited().connect(
      sigc::mem_fun(*this, &SubtitleView::on_edited_effect));

  column->set_resizable(true);
}

void SubtitleView::createColumnMarginR() {
  se_dbg(SE_DBG_VIEW);

  Gtk::TreeViewColumn *column = nullptr;
  Gtk::CellRendererText *renderer = nullptr;

  column = create_treeview_column("margin-r");
  renderer = manage(new Gtk::CellRendererText);

  column->pack_start(*renderer, false);
  column->add_attribute(renderer->property_text(), m_column.marginR);

  renderer->property_editable() = true;
  renderer->property_yalign() = 0;

  renderer->signal_edited().connect(
      sigc::mem_fun(*this, &SubtitleView::on_edited_margin_r));

  append_column(*column);
}

void SubtitleView::createColumnMarginL() {
  se_dbg(SE_DBG_VIEW);

  Gtk::TreeViewColumn *column = nullptr;
  Gtk::CellRendererText *renderer = nullptr;

  column = create_treeview_column("margin-l");
  renderer = manage(new Gtk::CellRendererText);

  column->pack_start(*renderer, false);
  column->add_attribute(renderer->property_text(), m_column.marginL);

  renderer->property_editable() = true;
  renderer->property_yalign() = 0;

  renderer->signal_edited().connect(
      sigc::mem_fun(*this, &SubtitleView::on_edited_margin_l));

  append_column(*column);
}

void SubtitleView::createColumnMarginV() {
  se_dbg(SE_DBG_VIEW);

  Gtk::TreeViewColumn *column = nullptr;
  Gtk::CellRendererText *renderer = nullptr;

  column = create_treeview_column("margin-v");
  renderer = manage(new Gtk::CellRendererText);

  column->pack_start(*renderer, false);
  column->add_attribute(renderer->property_text(), m_column.marginV);

  renderer->property_editable() = true;
  renderer->property_yalign() = 0;

  renderer->signal_edited().connect(
      sigc::mem_fun(*this, &SubtitleView::on_edited_margin_v));

  append_column(*column);
}

// retourne l'item select ou NULL
Gtk::TreeIter SubtitleView::getSelected() {
  se_dbg(SE_DBG_VIEW);

  Glib::RefPtr<Gtk::TreeSelection> selection = get_selection();

  std::vector<Gtk::TreeModel::Path> rows = selection->get_selected_rows();

  if (rows.size() > 0) {
    return selection->get_model()->get_iter(rows[0]);
  }

  Gtk::TreeIter null;
  return null;
}

void SubtitleView::on_selection_changed() {
  se_dbg(SE_DBG_VIEW);

  m_refDocument->emit_signal("subtitle-selection-changed");
}

void SubtitleView::on_edited_layer(const Glib::ustring &path,
                                   const Glib::ustring &value) {
  se_dbg_msg(SE_DBG_VIEW, "%s %s", path.c_str(), value.c_str());

  Subtitle subtitle(m_refDocument, path);
  if (subtitle) {
    unsigned int val = 0;
    if (from_string(value, val)) {
      m_refDocument->start_command(_("Editing layer"));
      subtitle.set_layer(value);
      m_refDocument->finish_command();
    }
  }
}

// callback utiliser pour modifier le temps directement depuis la list
// (treeview)
void SubtitleView::on_edited_start(const Glib::ustring &path,
                                   const Glib::ustring &newtext) {
  se_dbg_msg(SE_DBG_VIEW, "%s %s", path.c_str(), newtext.c_str());

  Subtitle subtitle(m_refDocument, path);
  if (!subtitle)
    return;

  if (subtitle.get("start") == newtext)
    return;

  if (m_refDocument->get_edit_timing_mode() == TIME) {
    if (!SubtitleTime::validate(newtext))
      return;

    m_refDocument->start_command(_("Editing start"));
    subtitle.set_start(newtext);
    m_refDocument->emit_signal("subtitle-time-changed");
    m_refDocument->finish_command();
  } else {  // edit_mode == FRAME
    long frame = 0;
    if (!from_string(newtext, frame))
      return;

    m_refDocument->start_command(_("Editing start"));
    subtitle.set_start_frame(frame);
    m_refDocument->emit_signal("subtitle-time-changed");
    m_refDocument->finish_command();
  }
}

// callback utiliser pour modifier le temps directement depuis la list
// (treeview)
void SubtitleView::on_edited_end(const Glib::ustring &path,
                                 const Glib::ustring &newtext) {
  se_dbg_msg(SE_DBG_VIEW, "%s %s", path.c_str(), newtext.c_str());

  Subtitle subtitle(m_refDocument, path);
  if (!subtitle)
    return;

  if (subtitle.get("end") == newtext)
    return;

  if (m_refDocument->get_edit_timing_mode() == TIME) {
    if (!SubtitleTime::validate(newtext))
      return;

    m_refDocument->start_command(_("Editing end"));
    subtitle.set_end(newtext);
    m_refDocument->emit_signal("subtitle-time-changed");
    m_refDocument->finish_command();
  } else {  // edit_mode == FRAME
    long frame = 0;
    if (!from_string(newtext, frame))
      return;

    m_refDocument->start_command(_("Editing end"));
    subtitle.set_end_frame(frame);
    m_refDocument->emit_signal("subtitle-time-changed");
    m_refDocument->finish_command();
  }
}

// callback utiliser pour modifier le temps directement depuis la list
// (treeview)
void SubtitleView::on_edited_duration(const Glib::ustring &path,
                                      const Glib::ustring &newtext) {
  se_dbg_msg(SE_DBG_VIEW, "%s %s", path.c_str(), newtext.c_str());

  Subtitle subtitle(m_refDocument, path);
  if (!subtitle)
    return;

  if (subtitle.get("duration") == newtext)
    return;

  if (m_refDocument->get_edit_timing_mode() == TIME) {
    if (!SubtitleTime::validate(newtext))
      return;

    m_refDocument->start_command(_("Editing duration"));
    subtitle.set_duration(newtext);
    m_refDocument->emit_signal("subtitle-time-changed");
    m_refDocument->finish_command();
  } else {  // edit_mode == FRAME
    long frame = 0;
    if (!from_string(newtext, frame))
      return;

    m_refDocument->start_command(_("Editing duration"));
    subtitle.set_duration_frame(frame);
    m_refDocument->emit_signal("subtitle-time-changed");
    m_refDocument->finish_command();
  }
}

// callback utiliser pour modifier le texte
void SubtitleView::on_edited_text(const Glib::ustring &path,
                                  const Glib::ustring &newtext) {
  se_dbg_msg(SE_DBG_VIEW, "%s %s", path.c_str(), newtext.c_str());

  Subtitle subtitle(m_refDocument, path);
  if (subtitle) {
    if (subtitle.get("text") != newtext) {
      m_refDocument->start_command(_("Editing text"));

      subtitle.set_text(newtext);

      m_refDocument->finish_command();
    }
  }
}

// callback utiliser pour modifier le texte
void SubtitleView::on_edited_translation(const Glib::ustring &path,
                                         const Glib::ustring &newtext) {
  se_dbg_msg(SE_DBG_VIEW, "%s %s", path.c_str(), newtext.c_str());

  Subtitle subtitle(m_refDocument, path);
  if (subtitle) {
    if (subtitle.get("translation") != newtext) {
      m_refDocument->start_command(_("Editing translation"));
      subtitle.set_translation(newtext);
      m_refDocument->finish_command();
    }
  }
}

// callback utiliser pour modifier le texte
void SubtitleView::on_edited_note(const Glib::ustring &path,
                                  const Glib::ustring &newtext) {
  se_dbg_msg(SE_DBG_VIEW, "%s %s", path.c_str(), newtext.c_str());

  Subtitle subtitle(m_refDocument, path);
  if (subtitle) {
    if (subtitle.get("note") != newtext) {
      m_refDocument->start_command(_("Editing note"));
      subtitle.set_note(newtext);
      m_refDocument->finish_command();
    }
  }
}

void SubtitleView::on_edited_effect(const Glib::ustring &path,
                                    const Glib::ustring &newtext) {
  se_dbg_msg(SE_DBG_VIEW, "%s %s", path.c_str(), newtext.c_str());

  Subtitle subtitle(m_refDocument, path);
  if (subtitle) {
    if (subtitle.get("effect") != newtext) {
      m_refDocument->start_command(_("Editing effect"));
      subtitle.set_effect(newtext);
      m_refDocument->finish_command();
    }
  }
}

// callback utiliser pour modifier le style a partir d'un menu
void SubtitleView::on_edited_style(const Glib::ustring &path,
                                   const Glib::ustring &newstyle) {
  se_dbg_msg(SE_DBG_VIEW, "%s %s", path.c_str(), newstyle.c_str());

  Subtitle subtitle(m_refDocument, path);
  if (subtitle) {
    if (subtitle.get("style") != newstyle) {
      m_refDocument->start_command(_("Editing style"));
      subtitle.set_style(newstyle);
      m_refDocument->finish_command();
    }
  }
}

// callback utiliser pour modifier le nom
void SubtitleView::on_edited_name(const Glib::ustring &path,
                                  const Glib::ustring &newname) {
  se_dbg_msg(SE_DBG_VIEW, "%s %s", path.c_str(), newname.c_str());

  Subtitle subtitle(m_refDocument, path);
  if (subtitle) {
    if (subtitle.get("name") != newname) {
      m_refDocument->start_command(_("Editing name"));
      subtitle.set_name(newname);
      m_refDocument->finish_command();
    }
  }
}

void SubtitleView::on_edited_margin_l(const Glib::ustring &path,
                                      const Glib::ustring &value) {
  se_dbg_msg(SE_DBG_VIEW, "%s %s", path.c_str(), value.c_str());

  Subtitle subtitle(m_refDocument, path);
  if (subtitle) {
    unsigned int val = 0;
    if (from_string(value, val)) {
      m_refDocument->start_command(_("Editing margin-l"));
      subtitle.set_margin_l(value);
      m_refDocument->finish_command();
    }
  }
}

void SubtitleView::on_edited_margin_r(const Glib::ustring &path,
                                      const Glib::ustring &value) {
  se_dbg_msg(SE_DBG_VIEW, "%s %s", path.c_str(), value.c_str());

  Subtitle subtitle(m_refDocument, path);
  if (subtitle) {
    unsigned int val = 0;
    if (from_string(value, val)) {
      m_refDocument->start_command(_("Editing margin-r"));
      subtitle.set_margin_r(value);
      m_refDocument->finish_command();
    }
  }
}

void SubtitleView::on_edited_margin_v(const Glib::ustring &path,
                                      const Glib::ustring &value) {
  se_dbg_msg(SE_DBG_VIEW, "%s %s", path.c_str(), value.c_str());

  Subtitle subtitle(m_refDocument, path);
  if (subtitle) {
    unsigned int val = 0;
    if (from_string(value, val)) {
      m_refDocument->start_command(_("Editing margin-v"));
      subtitle.set_margin_v(value);
      m_refDocument->finish_command();
    }
  }
}

void SubtitleView::select_and_set_cursor(const Gtk::TreeIter &iter,
                                         bool start_editing) {
  se_dbg(SE_DBG_VIEW);

  Gtk::TreeViewColumn *column = nullptr;
  Gtk::TreeModel::Path path;
  get_cursor(path, column);

  if (column == nullptr)
    column = m_columns["text"];

  get_selection()->select(iter);

  const Gtk::TreeModel::Path path_from_iter = m_subtitleModel->get_path(iter);
  // Select the row and the column (cell)
  set_cursor(path_from_iter, *column, start_editing);
  // Scroll to center the current selection in the view
  scroll_to_row(path_from_iter, 0.5);
}

bool SubtitleView::on_key_press_event(GdkEventKey *event) {

  if ((event->string != nullptr)&& cfg::get_boolean("subtitle-view", "enable-goto-subtitle-number")) {
    int num;
    std::istringstream ss(event->string);
    bool is_num = static_cast<bool>(ss >> num) != 0;
    // Update only if it's different
    if (is_num != get_enable_search())
      set_enable_search(is_num);
  } else {
    set_enable_search(false);
  }

  return Gtk::TreeView::on_key_press_event(event);
}

void SubtitleView::on_config_subtitle_view_changed(const Glib::ustring &key,
                                                   const Glib::ustring &value) {
  if (key == "columns-displayed") {
    update_columns_displayed_from_config();
  } else if (key == "property-alignment-center") {
    bool state;
    if (from_string(value, state)) {
      Gtk::CellRendererText *renderer = nullptr;

      renderer = dynamic_cast<Gtk::CellRendererText *>(
          m_columns["text"]->get_first_cell());
      renderer->property_xalign() = state ? 0.5 : 0.0;
      renderer->property_alignment() =
          state ? Pango::ALIGN_CENTER : Pango::ALIGN_LEFT;

      renderer = dynamic_cast<Gtk::CellRendererText *>(
          m_columns["translation"]->get_first_cell());
      renderer->property_xalign() = state ? 0.5 : 0.0;
      renderer->property_alignment() =
          state ? Pango::ALIGN_CENTER : Pango::ALIGN_LEFT;
    }

    queue_draw();
  } else if (key == "show-character-per-line") {
    bool state;
    if (from_string(value, state)) {
      std::vector<Gtk::CellRenderer *> cells;

      cells = m_columns["text"]->get_cells();
      cells[1]->property_visible() = state;

      cells = m_columns["translation"]->get_cells();
      cells[1]->property_visible() = state;
    }
    queue_draw();
  } else if (key == "enable-rubberband-selection") {
    set_rubber_banding(utility::string_to_bool(value));
  }
}

void SubtitleView::on_set_style_to_selection(const Glib::ustring &name) {
  auto selection = m_refDocument->subtitles().get_selection();
  if (selection.empty())
    return;

  m_refDocument->start_command(_("Set style to selection"));
  for (auto &select : selection) {
    select.set("style", name);
  }
  m_refDocument->finish_command();
}

// COLUMN

// retourne la colonne par rapport a son nom (interne)
Gtk::TreeViewColumn *SubtitleView::get_column_by_name(
    const Glib::ustring &name) {
  auto it = m_columns.find(name);

  if (it != m_columns.end())
    return it->second;

  se_dbg_msg(SE_DBG_VIEW, "column: %s return NULL", name.c_str());

  return nullptr;
}

// retourne le nom utiliser en interne de la column
Glib::ustring SubtitleView::get_name_of_column(Gtk::TreeViewColumn *column) {
  for (const auto &col : m_columns) {
    if (col.second == column)
      return col.first;
  }

  return Glib::ustring();
}

void SubtitleView::set_column_visible(const Glib::ustring &name, bool state) {
  se_dbg_msg(SE_DBG_VIEW, "%s=%s", name.c_str(), state ? "true" : "false");

  Gtk::TreeViewColumn *column = get_column_by_name(name);

  g_return_if_fail(column);

  column->set_visible(state);
}

bool SubtitleView::get_column_visible(const Glib::ustring &name) {
  Gtk::TreeViewColumn *column = get_column_by_name(name);

  g_return_val_if_fail(column, false);

  se_dbg_msg(SE_DBG_VIEW, "<%s> = %s", name.c_str(),
             column->get_visible() ? "true" : "false");

  return column->get_visible();
}

// Get the columns displayed from the configuration and updates.
void SubtitleView::update_columns_displayed_from_config() {
  se_dbg(SE_DBG_VIEW);

  if (cfg::has_key("subtitle-view", "columns-displayed") == false)
    return;

  // hide all columns
  for (const auto &col_map : m_columns) {
    col_map.second->set_visible(false);
  }

  // reorder columns
  Gtk::TreeViewColumn *current_column = nullptr;

  // get columns order
  auto cols = cfg::get_string_list("subtitle-view", "columns-displayed");
  for (const auto &col : cols) {
    Glib::ustring name = col;

    if (current_column) {
      Gtk::TreeViewColumn *tmp = get_column_by_name(name);
      if (tmp) {
        move_column_after(*tmp, *current_column);
      }
      current_column = tmp;
    } else {  // it's the first, put at start
      current_column = get_column_by_name(name);
      if (current_column) {
        move_column_to_start(*current_column);
      }
    }

    // display column
    if (current_column)
      current_column->set_visible(true);
  }
}

// This is a static function.
// Return the humain label by the internal name of the column.
Glib::ustring SubtitleView::get_column_label_by_name(
    const Glib::ustring &name) {
  std::map<Glib::ustring, Glib::ustring> columns_labels;

  columns_labels["cps"] = _("CPS");
  columns_labels["duration"] = _("Duration");
  columns_labels["effect"] = _("Effect");
  columns_labels["end"] = _("End");
  columns_labels["layer"] = _("Layer");
  columns_labels["margin-l"] = _("L");
  columns_labels["margin-r"] = _("R");
  columns_labels["margin-v"] = _("V");
  columns_labels["name"] = _("Name");
  columns_labels["note"] = _("Note");
  columns_labels["number"] = _("Num");
  columns_labels["start"] = _("Start");
  columns_labels["style"] = _("Style");
  columns_labels["text"] = _("Text");
  columns_labels["translation"] = _("Translation");

  auto it = columns_labels.find(name);
  if (it != columns_labels.end())
    return it->second;

  return Glib::ustring("Invalid : ") + name;
}

// The position of the cursor (focused cell) has changed.
// Update the column title (bold).
void SubtitleView::on_cursor_changed() {
  se_dbg(SE_DBG_VIEW);

  Pango::AttrList normal;
  Pango::AttrInt att_normal =
      Pango::Attribute::create_attr_weight(Pango::WEIGHT_NORMAL);
  normal.insert(att_normal);

  Pango::AttrList active;
  Pango::AttrInt att_active =
      Pango::Attribute::create_attr_weight(Pango::WEIGHT_BOLD);
  active.insert(att_active);

  // get the focused column
  Gtk::TreeViewColumn *focused_column = nullptr;
  Gtk::TreeModel::Path path;
  get_cursor(path, focused_column);

  // if it's the same, doesn't needs update
  if (m_currentColumn == focused_column)
    return;

  // unbold the old column
  if (m_currentColumn != nullptr) {
    auto label = dynamic_cast<Gtk::Label *>(m_currentColumn->get_widget());
    label->set_attributes(normal);

    m_currentColumn = nullptr;
  }
  // bold the new current column
  if (focused_column) {
    Gtk::Label *label =
        dynamic_cast<Gtk::Label *>(focused_column->get_widget());
    label->set_attributes(active);

    m_currentColumn = focused_column;
  }
}

// Return the name of the current column focus.
// (start, end, duration, text, translation ...)
Glib::ustring SubtitleView::get_current_column_name() {
  if (m_currentColumn)
    return get_name_of_column(m_currentColumn);
  return Glib::ustring();
}
