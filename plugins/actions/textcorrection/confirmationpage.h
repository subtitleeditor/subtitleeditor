#pragma once

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

#include <gui/cellrenderercustom.h>
#include <gui/textviewcell.h>
#include <widget_config_utility.h>
#include "page.h"
#include "patternmanager.h"

class ComfirmationPage : public AssistantPage {
  class Column : public Gtk::TreeModel::ColumnRecord {
   public:
    Column() {
      add(num);
      add(accept);
      add(original);
      add(corrected);
    }
    Gtk::TreeModelColumn<guint> num;
    Gtk::TreeModelColumn<bool> accept;
    Gtk::TreeModelColumn<Glib::ustring> original;
    Gtk::TreeModelColumn<Glib::ustring> corrected;
  };

 public:
  ComfirmationPage(BaseObjectType* cobject,
                   const Glib::RefPtr<Gtk::Builder>& builder)
      : AssistantPage(cobject, builder) {
    builder->get_widget("treeview-comfirmation", m_treeview);
    builder->get_widget("button-comfirmation-mark-all", m_buttonMarkAll);
    builder->get_widget("button-comfirmation-unmark-all", m_buttonUnmarkAll);
    builder->get_widget("check-comfirmation-remove-blank", m_checkRemoveBlank);

    create_treeview();
    init_signals();

    widget_config::read_config_and_connect(m_checkRemoveBlank,
                                           "comfirmation-page", "remove-blank");
  }

  void create_treeview() {
    m_liststore = Gtk::ListStore::create(m_column);
    m_treeview->set_model(m_liststore);

    // column Num
    {
      Gtk::TreeViewColumn* column = manage(new Gtk::TreeViewColumn(_("Num")));
      m_treeview->append_column(*column);

      Gtk::CellRendererText* label = manage(new Gtk::CellRendererText);
      column->pack_start(*label);
      column->add_attribute(label->property_text(), m_column.num);
    }
    // column Accept
    {
      Gtk::TreeViewColumn* column =
          manage(new Gtk::TreeViewColumn(_("Accept")));
      m_treeview->append_column(*column);

      Gtk::CellRendererToggle* toggle = manage(new Gtk::CellRendererToggle);
      column->pack_start(*toggle);
      column->add_attribute(toggle->property_active(), m_column.accept);
      toggle->signal_toggled().connect(
          sigc::mem_fun(*this, &ComfirmationPage::on_accept_toggled));
    }
    // column Original
    {
      Gtk::TreeViewColumn* column =
          manage(new Gtk::TreeViewColumn(_("Original Text")));
      m_treeview->append_column(*column);

      CellRendererCustom<TextViewCell>* label =
          manage(new CellRendererCustom<TextViewCell>);
      column->pack_start(*label);
      column->add_attribute(label->property_text(), m_column.original);
    }
    // column Corrected
    {
      m_column_corrected_text =
          manage(new Gtk::TreeViewColumn(_("Corrected Text")));
      m_treeview->append_column(*m_column_corrected_text);

      CellRendererCustom<TextViewCell>* renderer =
          manage(new CellRendererCustom<TextViewCell>);
      m_column_corrected_text->pack_start(*renderer);
      m_column_corrected_text->add_attribute(renderer->property_text(),
                                             m_column.corrected);
      renderer->property_editable() = true;
      renderer->signal_edited().connect(
          sigc::mem_fun(*this, &ComfirmationPage::on_corrected_edited));
    }
    m_treeview->signal_row_activated().connect(
        sigc::mem_fun(*this, &ComfirmationPage::on_row_activated));
  }

  void init_signals() {
    m_buttonMarkAll->signal_clicked().connect(
        sigc::mem_fun(*this, &ComfirmationPage::on_mark_all));
    m_buttonUnmarkAll->signal_clicked().connect(
        sigc::mem_fun(*this, &ComfirmationPage::on_unmark_all));
  }

  bool comfirme(Document* doc, const std::list<Pattern*>& patterns) {
    m_liststore->clear();

    Subtitles subs = doc->subtitles();

    Glib::ustring text, previous;
    for (Subtitle sub = subs.get_first(); sub; ++sub) {
      text = sub.get_text();

      for (const auto& pattern : patterns) {
        pattern->execute(text, previous);
      }

      if (sub.get_text() != text) {
        Gtk::TreeIter it = m_liststore->append();
        (*it)[m_column.num] = sub.get_num();
        (*it)[m_column.accept] = true;
        (*it)[m_column.original] = sub.get_text();
        (*it)[m_column.corrected] = text;
      }

      previous = text;
    }
    return !m_liststore->children().empty();
  }

  Glib::ustring get_page_title() {
    unsigned int size = m_liststore->children().size();
    if (size == 0)
      return _("There Is No Change");

    return Glib::ustring::compose(
        ngettext("Confirm %1 Change", "Confirm %1 Changes", size), size);
  }

  // Apply the accepted change to the document.
  void apply(Document* doc) {
    g_return_if_fail(doc);

    bool remove_blank = m_checkRemoveBlank->get_active();
    std::vector<Subtitle> blank_subs, selection;

    doc->start_command(_("Text Correction"));
    Subtitles subtitles = doc->subtitles();

    for (Gtk::TreeIter it = m_liststore->children().begin(); it; ++it) {
      bool accept = (*it)[m_column.accept];
      if (accept == false)
        continue;

      unsigned int num = (*it)[m_column.num];
      Glib::ustring corrected = (*it)[m_column.corrected];

      Subtitle sub = subtitles.get(num);
      if (sub.get_text() != corrected) {
        sub.set_text(corrected);
        selection.push_back(sub);
      }
      if (remove_blank)
        if (sub.get_text().empty())
          blank_subs.push_back(sub);
    }
    // Select the modified subtitles
    subtitles.select(selection);
    // Remove the blank subtitles
    if (remove_blank && blank_subs.empty() == false)
      subtitles.remove(blank_subs);

    doc->finish_command();
  }

 protected:
  // Mark all items.
  void on_mark_all() {
    Gtk::TreeIter it = m_liststore->children().begin();
    while (it) {
      (*it)[m_column.accept] = true;
      ++it;
    }
  }

  // Unmark all items.
  void on_unmark_all() {
    Gtk::TreeIter it = m_liststore->children().begin();
    while (it) {
      (*it)[m_column.accept] = false;
      ++it;
    }
  }

  // Toggle the state of accept value.
  void on_accept_toggled(const Glib::ustring& path) {
    Gtk::TreeIter it = m_liststore->get_iter(path);
    if (it) {
      (*it)[m_column.accept] = !static_cast<bool>((*it)[m_column.accept]);
    }
  }

  void on_row_activated(const Gtk::TreeModel::Path& path,
                        Gtk::TreeViewColumn* column) {
    if (column == m_column_corrected_text)
      return;
    on_accept_toggled(path.to_string());
  }

  // Update the item text.
  void on_corrected_edited(const Glib::ustring& path,
                           const Glib::ustring& text) {
    Gtk::TreeIter it = m_liststore->get_iter(path);
    if (it) {
      (*it)[m_column.corrected] = text;
    }
  }

 protected:
  Column m_column;
  Glib::RefPtr<Gtk::ListStore> m_liststore;
  Gtk::TreeView* m_treeview;
  Gtk::TreeViewColumn* m_column_corrected_text;
  Gtk::Button* m_buttonMarkAll;
  Gtk::Button* m_buttonUnmarkAll;
  Gtk::CheckButton* m_checkRemoveBlank;
};
