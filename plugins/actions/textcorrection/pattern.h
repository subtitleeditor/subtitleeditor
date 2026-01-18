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

#include <glibmm.h>

#include <list>

class Pattern {
   friend class PatternManager;

   // Private class for Rule
   // Pattern can be have multiple rule
   class Rule {
     public:
      Glib::RefPtr<Glib::Regex> m_regex;
      Glib::ustring m_replacement;
      bool m_repeat;

      Glib::RefPtr<Glib::Regex> m_previous_match;
   };

  public:
   // Constructor
   Pattern();

   // Destructor
   // Delete rules.
   ~Pattern();

   // Return the name of the pattern.
   Glib::ustring get_name() const;

   // Return the name of the pattern.
   Glib::ustring get_label() const;

   // Return the name of the pattern.
   Glib::ustring get_description() const;

   // Return the active state of the pattern. (Enable by default)
   bool is_enable() const;

   // Apply the pattern if it is enabled.
   // With the repeat support.
   void execute(Glib::ustring& text, const Glib::ustring& previous);

  protected:
   bool m_enabled;
   Glib::ustring m_codes;
   Glib::ustring m_name;
   Glib::ustring m_label;
   Glib::ustring m_description;
   Glib::ustring m_classes;
   Glib::ustring m_policy;
   std::list<Rule*> m_rules;
};
