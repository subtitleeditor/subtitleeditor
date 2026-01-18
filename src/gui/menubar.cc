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

#include "menubar.h"

#include "utility.h"

MenuBar::MenuBar() : Gtk::VBox(false, 0) {
   m_refUIManager = Gtk::UIManager::create();
}

// Use to show the tooltip in the statusbar.
void MenuBar::connect_proxy(const Glib::RefPtr<Gtk::Action>& action, Gtk::Widget* widget) {
   if (Gtk::MenuItem* item = dynamic_cast<Gtk::MenuItem*>(widget)) {
      Glib::ustring tooltip = action->property_tooltip();

      item->signal_select().connect(sigc::bind(sigc::mem_fun(m_statusbar, &Statusbar::push_text), tooltip));
      item->signal_deselect().connect(sigc::mem_fun(m_statusbar, &Statusbar::pop_text));
   }
}

void MenuBar::create(Gtk::Window& window, Statusbar& statusbar) {
   m_statusbar = &statusbar;

   Glib::RefPtr<Gtk::ActionGroup> actiongroup = Gtk::ActionGroup::create("default");

   // create all menu
   actiongroup->add(Gtk::Action::create("menu-file", _("_File")));
   actiongroup->add(Gtk::Action::create("menu-selection", _("_Selection")));
   actiongroup->add(Gtk::Action::create("menu-edit", _("_Edit")));
   actiongroup->add(Gtk::Action::create("menu-timings", _("_Timings")));
   actiongroup->add(Gtk::Action::create("menu-tools", _("T_ools")));
   actiongroup->add(Gtk::Action::create("menu-video", _("_Video")));
   actiongroup->add(Gtk::Action::create("menu-keyframes", _("_Keyframes")));
   actiongroup->add(Gtk::Action::create("menu-waveform", _("_Waveform")));
   actiongroup->add(Gtk::Action::create("menu-view", _("V_iew")));
   actiongroup->add(Gtk::Action::create("menu-options", _("_Options")));
   actiongroup->add(Gtk::Action::create("menu-extensions", _("E_xtensions")));
   actiongroup->add(Gtk::Action::create("menu-help", _("_Help")));
   // file submenu
   actiongroup->add(Gtk::Action::create("menu-open", _("_Open"), _("Open a document or translation")));
   actiongroup->add(Gtk::Action::create("menu-save", _("_Save"), _("Save document, documents or translation")));
   // If we add the stock_id when we create the actions,
   // they will automatically set up with the
   // default shortcut (Open: Ctrl+O, Save: Ctrl+S).
   // This make a conflict with others actions.
#if GTKMM_CHECK_VERSION(2, 16, 0)
   actiongroup->get_action("menu-open")->set_stock_id(Gtk::Stock::OPEN);
   actiongroup->get_action("menu-save")->set_stock_id(Gtk::Stock::SAVE);
#endif  // GTKMM_CHECK_VERSION(2, 16, 0)

   // UIManager
   m_refUIManager->signal_connect_proxy().connect(sigc::mem_fun(*this, &MenuBar::connect_proxy));

   m_refUIManager->insert_action_group(actiongroup);

   window.add_accel_group(m_refUIManager->get_accel_group());

   create_ui_from_file();
}

void MenuBar::create_ui_from_file() {
   Glib::ustring menubar_xml = Glib::build_filename(SE_DEV_VALUE(PACKAGE_SHARE_DIR, PACKAGE_SHARE_DIR_DEV), "menubar.xml");

   m_refUIManager->add_ui_from_file(menubar_xml);

   pack_start(*m_refUIManager->get_widget("/menubar"), false, false);

   show_all();
}

Glib::RefPtr<Gtk::UIManager> MenuBar::get_ui_manager() {
   return m_refUIManager;
}
