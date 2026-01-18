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

#include "treeviewextensionmanager.h"

#include <gtkmm/treemodelfilter.h>

class ColumnExtension : public Gtk::TreeModel::ColumnRecord {
  public:
   ColumnExtension() {
      add(active);
      add(stock_id);
      add(label);
      add(info);
   }

   Gtk::TreeModelColumn<bool> active;
   Gtk::TreeModelColumn<Glib::ustring> stock_id;
   Gtk::TreeModelColumn<Glib::ustring> label;
   Gtk::TreeModelColumn<ExtensionInfo*> info;
};

// Sort extension by categorie and by locale name
bool on_sort_extension(ExtensionInfo* a, ExtensionInfo* b) {
   if (a->get_categorie() != b->get_categorie())
      return a->get_categorie() < b->get_categorie();
   return a->get_label() < b->get_label();
}

// Call automatically create_view().
TreeViewExtensionManager::TreeViewExtensionManager(BaseObjectType* cobject, Glib::RefPtr<Gtk::Builder>&) : Gtk::TreeView(cobject) {
   create_view();
}

// Call automatically create_view().
TreeViewExtensionManager::TreeViewExtensionManager() {
   create_view();
}

// Create column with cell toggle (active state) and text (label and
// description). All extensions are added to the model.
void TreeViewExtensionManager::create_view() {
   ColumnExtension m_column;

   set_headers_visible(false);
   set_row_separator_func(sigc::mem_fun(*this, &TreeViewExtensionManager::on_row_separator_func));

   m_model = Gtk::ListStore::create(m_column);
   set_model(m_model);

   Gtk::TreeViewColumn* column = NULL;
   Gtk::CellRendererToggle* cell_toggle = NULL;
   Gtk::CellRendererText* cell_text = NULL;
   Gtk::CellRendererPixbuf* cell_pixbuf = NULL;

   // active
   column = manage(new Gtk::TreeViewColumn);
   append_column(*column);

   cell_toggle = manage(new Gtk::CellRendererToggle);
   cell_toggle->signal_toggled().connect(sigc::mem_fun(*this, &TreeViewExtensionManager::on_active_toggled));
   column->pack_start(*cell_toggle, false);
   column->add_attribute(cell_toggle->property_active(), m_column.active);

   // stock_id
   column = manage(new Gtk::TreeViewColumn);
   append_column(*column);

   cell_pixbuf = manage(new Gtk::CellRendererPixbuf);
   column->pack_start(*cell_pixbuf, true);
   column->add_attribute(cell_pixbuf->property_stock_id(), m_column.stock_id);

   // label
   column = manage(new Gtk::TreeViewColumn);
   append_column(*column);

   cell_text = manage(new Gtk::CellRendererText);
   cell_text->property_wrap_mode() = Pango::WRAP_WORD;
   cell_text->property_wrap_width() = 300;
   column->pack_start(*cell_text, true);
   column->add_attribute(cell_text->property_markup(), m_column.label);

   // property
   set_rules_hint(true);

   Glib::ustring categorie;

   auto extension_infos = ExtensionManager::instance().get_extension_info_list();
   // sort by categorie and by locale name
   extension_infos.sort(on_sort_extension);

   for (const auto& ext_info : extension_infos) {
      if (ext_info->get_hidden())
         continue;

      if (categorie.empty()) {
         categorie = ext_info->get_categorie();
      } else if (categorie != ext_info->get_categorie()) {
         // Categorie changed, add separator
         categorie = ext_info->get_categorie();

         Gtk::TreeIter sep = m_model->append();
         (*sep)[m_column.info] = NULL;
         (*sep)[m_column.active] = false;
         (*sep)[m_column.label] = "---";
      }

      Gtk::TreeIter iter = m_model->append();
      (*iter)[m_column.info] = ext_info;
      (*iter)[m_column.active] = ext_info->get_active();
      (*iter)[m_column.label] = Glib::ustring::compose("<b>%1</b>\n%2", ext_info->get_label(), ext_info->get_description());

      if (ext_info->get_extension() && ext_info->get_extension()->is_configurable()) {
         (*iter)[m_column.stock_id] = "gtk-preferences";
      }
   }
}

// Filter the model and display only one categorie
// ExtensionInfo->categorie
void TreeViewExtensionManager::set_filter(const Glib::ustring& categorie) {
   auto filter = Gtk::TreeModelFilter::create(get_model());

   filter->set_visible_func(sigc::bind(sigc::mem_fun(*this, &TreeViewExtensionManager::on_filter_visible), categorie));

   set_model(filter);
}

// Try to update the active state of the extension.
void TreeViewExtensionManager::on_active_toggled(const Glib::ustring& path) {
   ColumnExtension m_column;

   Gtk::TreeIter it = m_model->get_iter(path);

   ExtensionInfo* info = (*it)[m_column.info];
   if (info) {
      bool active = !info->get_active();

      // Only if the extension manager success
      if (ExtensionManager::instance().set_extension_active(info->get_name(), active))
         (*it)[m_column.active] = active;
   }
}

// Used by the filter.
bool TreeViewExtensionManager::on_filter_visible(const Gtk::TreeModel::const_iterator& iter, Glib::ustring categorie) {
   static ColumnExtension column;

   ExtensionInfo* info = (*iter)[column.info];
   if (info) {
      if (info->get_categorie() == categorie)
         return true;
   }
   return false;
}

// Return the current extension selected or NULL.
ExtensionInfo* TreeViewExtensionManager::get_selected_extension() {
   Gtk::TreeIter it = get_selection()->get_selected();
   if (!it)
      return NULL;

   ColumnExtension column;
   return (*it)[column.info];
}

// Used to define the separator
// "---"
bool TreeViewExtensionManager::on_row_separator_func(const Glib::RefPtr<Gtk::TreeModel>& /*model*/, const Gtk::TreeModel::iterator& it) {
   static ColumnExtension column;

   Glib::ustring text = (*it)[column.label];
   if (text == "---")
      return true;
   return false;
}
