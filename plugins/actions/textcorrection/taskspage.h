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

#include "page.h"
#include "patternspage.h"

class TasksPage : public AssistantPage {
   class Column : public Gtk::TreeModel::ColumnRecord {
     public:
      Column() {
         add(enabled);
         add(label);
         add(page);
      }
      Gtk::TreeModelColumn<bool> enabled;
      Gtk::TreeModelColumn<Glib::ustring> label;
      Gtk::TreeModelColumn<PatternsPage*> page;
   };

  public:
   TasksPage(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) : AssistantPage(cobject, builder) {
      builder->get_widget("treeview-tasks", m_treeview);
      create_treeview();
   }

   void create_treeview() {
      m_liststore = Gtk::ListStore::create(m_column);
      m_treeview->set_model(m_liststore);

      // column display
      {
         Gtk::TreeViewColumn* column = manage(new Gtk::TreeViewColumn(_("Display")));
         m_treeview->append_column(*column);

         Gtk::CellRendererToggle* toggle = manage(new Gtk::CellRendererToggle);
         column->pack_start(*toggle);
         column->add_attribute(toggle->property_active(), m_column.enabled);
         toggle->signal_toggled().connect(sigc::mem_fun(*this, &TasksPage::on_enabled_toggled));
      }
      // column label
      {
         Gtk::TreeViewColumn* column = manage(new Gtk::TreeViewColumn(_("Name")));
         m_treeview->append_column(*column);

         Gtk::CellRendererText* label = manage(new Gtk::CellRendererText);
         column->pack_start(*label);
         column->add_attribute(label->property_markup(), m_column.label);
      }
      m_treeview->signal_row_activated().connect(sigc::mem_fun(*this, &TasksPage::on_row_activated));
   }

   void on_enabled_toggled(const Glib::ustring& path) {
      Gtk::TreeIter it = m_liststore->get_iter(path);
      if (it) {
         bool enabled = !static_cast<bool>((*it)[m_column.enabled]);
         PatternsPage* page = (*it)[m_column.page];

         (*it)[m_column.enabled] = enabled;
         page->set_enable(enabled);
      }
   }

   void on_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn*) {
      on_enabled_toggled(path.to_string());
   }

   void add_task(PatternsPage* page) {
      Gtk::TreeIter it;
      it = m_liststore->append();
      (*it)[m_column.enabled] = page->is_enable();
      (*it)[m_column.label] = Glib::ustring::compose("<b>%1</b>\n%2", page->get_page_label(), page->get_page_description());
      (*it)[m_column.page] = page;
   }

  protected:
   Gtk::TreeView* m_treeview;
   Column m_column;
   Glib::RefPtr<Gtk::ListStore> m_liststore;
};
