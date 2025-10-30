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

#include <gui/treeviewextensionmanager.h>
#include "preferencepage.h"

// Manage the extension.
// Activate, deactivate, about, preferences
class ExtensionPage : public PreferencePage {
 public:
  ExtensionPage(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& xml)
      : PreferencePage(cobject) {
    xml->get_widget_derived("treeview-extension", m_treeview);
    xml->get_widget("button-extension-about", m_buttonAbout);
    xml->get_widget("button-extension-preferences", m_buttonPreferences);

    m_treeview->get_selection()->signal_changed().connect(
        sigc::mem_fun(*this, &ExtensionPage::on_selection_changed));

    m_buttonAbout->signal_clicked().connect(
        sigc::mem_fun(*this, &ExtensionPage::on_about));
    m_buttonPreferences->signal_clicked().connect(
        sigc::mem_fun(*this, &ExtensionPage::on_preferences));

    on_selection_changed();
  }

  // Update the state sensitvite of the buttons About and Preferences.
  void on_selection_changed() {
    ExtensionInfo* info = m_treeview->get_selected_extension();

    bool about = false;
    bool preference = false;

    if (info) {
      about = true;

      if (info->get_active() && info->get_extension() != NULL)
        preference = info->get_extension()->is_configurable();
    }

    m_buttonAbout->set_sensitive(about);
    m_buttonPreferences->set_sensitive(preference);
  }

  // Display imformation about the extension.
  // Label, Description, Authors...
  void on_about() {
    ExtensionInfo* info = m_treeview->get_selected_extension();
    if (info == NULL)
      return;

    Gtk::AboutDialog dialog;
    if (Gtk::Window* parent = dynamic_cast<Gtk::Window*>(get_toplevel()))
      dialog.set_transient_for(*parent);

    dialog.set_program_name(info->get_label());
    dialog.set_comments(info->get_description());

    std::vector<Glib::ustring> authors;
    authors.push_back(info->get_authors());
    dialog.set_authors(authors);

    dialog.run();
  }

  // Display the dialog preferences of the extension.
  void on_preferences() {
    ExtensionInfo* info = m_treeview->get_selected_extension();
    if (info == NULL)
      return;

    Extension* ext = info->get_extension();
    if (ext == NULL)
      return;

    ext->create_configure_dialog();
  }

 protected:
  TreeViewExtensionManager* m_treeview;
  Gtk::Button* m_buttonAbout;
  Gtk::Button* m_buttonPreferences;
};
