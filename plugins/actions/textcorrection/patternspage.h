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

#include <isocodes.h>

#include "page.h"
#include "pattern.h"
#include "patternmanager.h"

class ComboBoxText : public Gtk::ComboBox {
   class Column : public Gtk::TreeModel::ColumnRecord {
     public:
      Column() {
         add(label);
         add(code);
      }
      Gtk::TreeModelColumn<Glib::ustring> label;
      Gtk::TreeModelColumn<Glib::ustring> code;
   };

  public:
   ComboBoxText(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>&) : Gtk::ComboBox(cobject) {
      init();
   }

   ComboBoxText() {
      init();
   }

   void init() {
      m_liststore = Gtk::ListStore::create(m_column);
      set_model(m_liststore);

      Gtk::CellRendererText* renderer = manage(new Gtk::CellRendererText);
      pack_start(*renderer);
      add_attribute(renderer->property_text(), m_column.label);

      // separator function
      Gtk::ComboBox::set_row_separator_func(sigc::mem_fun(*this, &ComboBoxText::on_row_separator_func));
   }

   void clear_model() {
      m_liststore->clear();
   }

   bool on_row_separator_func(const Glib::RefPtr<Gtk::TreeModel>&, const Gtk::TreeModel::iterator& it) {
      Glib::ustring text = (*it)[m_column.label];
      if (text == "---")
         return true;
      return false;
   }

   void append(const Glib::ustring& label, const Glib::ustring& code) {
      Gtk::TreeIter it = m_liststore->append();
      (*it)[m_column.label] = label;
      (*it)[m_column.code] = code;
   }

   Glib::ustring get_active_label() {
      Gtk::TreeIter it = get_active();
      if (it)
         return (*it)[m_column.label];
      return Glib::ustring();
   }

   Glib::ustring get_active_code() {
      Gtk::TreeIter it = get_active();
      if (it)
         return (*it)[m_column.code];
      return Glib::ustring();
   }

   void set_active_code(const Glib::ustring& code) {
      for (Gtk::TreeIter it = m_liststore->children().begin(); it; ++it) {
         if ((*it)[m_column.code] == code && (*it)[m_column.label] != "---") {
            set_active(it);
            return;
         }
      }
   }

  protected:
   Column m_column;
   Glib::RefPtr<Gtk::ListStore> m_liststore;
};

class PatternsPage : public AssistantPage {
   class Column : public Gtk::TreeModel::ColumnRecord {
     public:
      Column() {
         add(name);
         add(enabled);
         add(label);
      }
      Gtk::TreeModelColumn<Glib::ustring> name;
      Gtk::TreeModelColumn<bool> enabled;
      Gtk::TreeModelColumn<Glib::ustring> label;
   };

   // Sort pattern by locale name
   static bool sort_pattern(Pattern* a, Pattern* b) {
      return a->get_label() < b->get_label();
   }

   // Compare pattern name
   static bool unique_pattern(Pattern* a, Pattern* b) {
      return a->get_name() == b->get_name();
   }

  public:
   // Constructor for pattern page.
   // type = "common-error", "hearing-impaired"
   // label = the label of the page
   // description = a short description of the page
   PatternsPage(BaseObjectType* cobject,
                const Glib::RefPtr<Gtk::Builder>& builder,
                const Glib::ustring& type,
                const Glib::ustring& label,
                const Glib::ustring& description)
       : AssistantPage(cobject, builder), m_patternManager(type) {
      m_page_name = type;
      m_page_label = label;
      m_page_description = description;

      builder->get_widget("treeview-" + type, m_treeview);
      builder->get_widget_derived("combobox-script-" + type, m_comboScript);
      builder->get_widget_derived("combobox-language-" + type, m_comboLanguage);
      builder->get_widget_derived("combobox-country-" + type, m_comboCountry);

      initialize();
   }

   PatternsPage(const Glib::ustring& type, const Glib::ustring& title, const Glib::ustring& label, const Glib::ustring description)
       : AssistantPage(), m_patternManager(type) {
      m_page_name = type;
      m_page_title = title;
      m_page_label = label;
      m_page_description = description;

      Gtk::VBox* vbox = manage(new Gtk::VBox(false, 6));
      pack_start(*vbox, true, true);

      // treeview
      Gtk::ScrolledWindow* scrolled = manage(new Gtk::ScrolledWindow);
      scrolled->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
      scrolled->set_shadow_type(Gtk::SHADOW_ETCHED_IN);
      vbox->pack_start(*scrolled, true, true);

      m_treeview = manage(new Gtk::TreeView);
      scrolled->add(*m_treeview);

      // Box for Script, Language and Country combobox
      Gtk::Label* wlabel = NULL;

      Gtk::Table* table = manage(new Gtk::Table(3, 2, false));
      table->set_spacings(6);
      vbox->pack_start(*table, false, false);

      // Script
      wlabel = manage(new Gtk::Label(_("_Script:"), 0.0, 0.5, true));
      table->attach(*wlabel, 0, 1, 0, 1, Gtk::FILL);

      m_comboScript = manage(new ComboBoxText);
      table->attach(*m_comboScript, 1, 2, 0, 1);

      // Language
      wlabel = manage(new Gtk::Label(_("_Language:"), 0.0, 0.5, true));
      table->attach(*wlabel, 0, 1, 1, 2, Gtk::FILL);

      m_comboLanguage = manage(new ComboBoxText);
      table->attach(*m_comboLanguage, 1, 2, 1, 2);

      // Country
      wlabel = manage(new Gtk::Label(_("_Country:"), 0.0, 0.5, true));
      table->attach(*wlabel, 0, 1, 2, 3, Gtk::FILL);

      m_comboCountry = manage(new ComboBoxText);
      table->attach(*m_comboCountry, 1, 2, 2, 3);

      vbox->show_all();
      initialize();
   }

   void initialize() {
      create_treeview();

      init_signals();

      init_model();
      init_script();
      init_language();
      init_country();

      load_cfg();
   }

   // Return the title of the page.
   Glib::ustring get_page_title() {
      return m_page_title;
   }

   // Return the label of the page.
   Glib::ustring get_page_label() {
      return m_page_label;
   }

   // Return the description of the page.
   Glib::ustring get_page_description() {
      return m_page_description;
   }

   // Update the visibility state of the page. (enable/disable)
   void set_enable(bool state) {
      cfg::set_boolean(m_page_name, "enabled", state);

      (state) ? show() : hide();
   }

   // Return the visibility state. (enable/disable)
   bool is_enable() {
      return cfg::get_boolean(m_page_name, "enabled");
   }

   // Return patterns from the script, language and country.
   std::list<Pattern*> get_patterns() {
      return m_patternManager.get_patterns(get_script(), get_language(), get_country());
   }

   // Create the treeview with two columns, active and label (+description)
   void create_treeview() {
      m_treeview->set_headers_visible(false);
      m_treeview->set_rules_hint(true);

      m_liststore = Gtk::ListStore::create(m_column);
      m_treeview->set_model(m_liststore);
      // column active
      {
         Gtk::TreeViewColumn* column = manage(new Gtk::TreeViewColumn);
         m_treeview->append_column(*column);

         Gtk::CellRendererToggle* toggle = manage(new Gtk::CellRendererToggle);
         column->pack_start(*toggle);
         column->add_attribute(toggle->property_active(), m_column.enabled);
         toggle->signal_toggled().connect(sigc::mem_fun(*this, &PatternsPage::on_enabled_toggled));
      }
      // column label
      {
         Gtk::TreeViewColumn* column = manage(new Gtk::TreeViewColumn);
         m_treeview->append_column(*column);

         Gtk::CellRendererText* renderer = manage(new Gtk::CellRendererText);
         column->pack_start(*renderer);
         column->add_attribute(renderer->property_markup(), m_column.label);
      }
      m_treeview->signal_row_activated().connect(sigc::mem_fun(*this, &PatternsPage::on_row_activated));
   }

   // Update the state of the pattern and the patternmanager.
   void on_enabled_toggled(const Glib::ustring& path) {
      Gtk::TreeIter it = m_liststore->get_iter(path);
      if (it) {
         Glib::ustring name = (*it)[m_column.name];
         bool value = !static_cast<bool>((*it)[m_column.enabled]);

         (*it)[m_column.enabled] = value;
         m_patternManager.set_active(name, value);
      }
   }

   void on_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn*) {
      on_enabled_toggled(path.to_string());
   }

   // Init the treeview model with all patterns available from the cfg (script,
   // language, country).
   void init_model() {
      m_liststore->clear();

      std::list<Pattern*> patterns = get_patterns();
      // Sort the list by human translated label and unique items
      patterns.sort(sort_pattern);
      patterns.unique(unique_pattern);

      for (const auto& p : patterns) {
         Gtk::TreeIter iter = m_liststore->append();
         (*iter)[m_column.name] = p->get_name();
         (*iter)[m_column.enabled] = p->is_enable();
         (*iter)[m_column.label] = build_message("<b>%s</b>\n%s", _(p->get_label().c_str()), _(p->get_description().c_str()));
      }
   }

   // Init the signal of widgets.
   void init_signals() {
      m_comboScript->signal_changed().connect(sigc::mem_fun(*this, &PatternsPage::init_language));
      m_comboLanguage->signal_changed().connect(sigc::mem_fun(*this, &PatternsPage::init_country));
      m_comboCountry->signal_changed().connect(sigc::mem_fun(*this, &PatternsPage::init_model));
   }

   // Initialize the combobox with the last value "Other"
   // if there is no active item and if it's not empty.
   void init_combo(ComboBoxText* combo) {
      Gtk::TreeIter it = combo->get_active();
      if (!it) {
         unsigned n = combo->get_model()->children().size();
         if (n > 0)
            combo->set_active(n - 1);
      }
   }

   // Initialize the combo script from with the pattern available.
   void init_script() {
      auto scripts = m_patternManager.get_scripts();

      m_comboScript->clear_model();

      std::map<Glib::ustring, Glib::ustring> sort_map;

      for (const auto& s : scripts) {
         sort_map[isocodes::to_script(s)] = s;
      }
      for (const auto& i : sort_map) {
         m_comboScript->append(i.first, i.second);
      }
      m_comboScript->append("---", "");
      m_comboScript->append(_("Other"), "");

      init_combo(m_comboScript);
      init_model();
   }

   // Initialize the combo script from with the pattern available for the script.
   void init_language() {
      Glib::ustring script = get_script();

      auto languages = m_patternManager.get_languages(script);

      m_comboLanguage->clear_model();

      std::map<Glib::ustring, Glib::ustring> sort_map;

      for (const auto& l : languages) {
         sort_map[isocodes::to_language(l)] = l;
      }

      for (const auto& i : sort_map) {
         m_comboLanguage->append(i.first, i.second);
      }
      if (!languages.empty()) {
         m_comboLanguage->append("---", "");
         m_comboLanguage->append(_("Other"), "");
      }
      init_combo(m_comboLanguage);
      init_model();
   }

   // Initialize the combo script from with the pattern available from the script
   // and language.
   void init_country() {
      Glib::ustring script = get_script();
      Glib::ustring language = get_language();

      auto countries = m_patternManager.get_countries(script, language);

      m_comboCountry->clear_model();

      std::map<Glib::ustring, Glib::ustring> sort_map;
      for (const auto& c : countries) {
         sort_map[isocodes::to_country(c)] = c;
      }
      for (const auto& i : sort_map) {
         m_comboCountry->append(i.first, i.second);
      }

      if (!countries.empty()) {
         m_comboCountry->append("---", "");
         m_comboCountry->append(_("Other"), "");
      }
      init_combo(m_comboCountry);
      init_model();
   }

   // Read the configuration.
   // script, language, country and enabled (page).
   void load_cfg() {
      // Default enabled value is true
      if (cfg::has_key(m_page_name, "enabled") == false) {
         cfg::set_boolean(m_page_name, "enabled", true);
      }

      if (cfg::get_boolean(m_page_name, "enabled")) {
         show();
      } else {
         hide();
      }
      Glib::ustring script = cfg::get_string(m_page_name, "script");
      Glib::ustring language = cfg::get_string(m_page_name, "language");
      Glib::ustring country = cfg::get_string(m_page_name, "country");

      m_comboScript->set_active_code(script);
      m_comboLanguage->set_active_code(language);
      m_comboCountry->set_active_code(country);
   }

   // Save the configuration.
   // script, language, country and enabled (page).
   void save_cfg() {
      cfg::set_string(m_page_name, "script", get_script());
      cfg::set_string(m_page_name, "language", get_language());
      cfg::set_string(m_page_name, "country", get_country());
      cfg::set_boolean(m_page_name, "enabled", is_enable());
   }

   // Return the current script code.
   Glib::ustring get_script() {
      Glib::ustring value = m_comboScript->get_active_code();
      return value;
   }

   // Return the current language code.
   Glib::ustring get_language() {
      Glib::ustring value = m_comboLanguage->get_active_code();
      return value;
   }

   // Return the current country code.
   Glib::ustring get_country() {
      Glib::ustring value = m_comboCountry->get_active_code();
      return value;
   }

  protected:
   Glib::ustring m_page_name;
   Glib::ustring m_page_title;
   Glib::ustring m_page_label;
   Glib::ustring m_page_description;

   PatternManager m_patternManager;
   Gtk::TreeView* m_treeview;
   Column m_column;
   Glib::RefPtr<Gtk::ListStore> m_liststore;

   ComboBoxText* m_comboScript;
   ComboBoxText* m_comboLanguage;
   ComboBoxText* m_comboCountry;
};
