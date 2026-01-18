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

#include "stylemodel.h"

#include "color.h"

StyleModel::StyleModel() {
   set_column_types(m_column);
}

Gtk::TreeIter StyleModel::append() {
   static Color blanc(255, 255, 255, 255);
   static Color jaune(255, 255, 0, 255);
   static Color marron(180, 120, 0, 255);
   static Color noir(0, 0, 0, 255);

   Gtk::TreeIter iter = Gtk::ListStore::append();

   (*iter)[m_column.name] = "Default";

   (*iter)[m_column.font_name] = "Sans";
   (*iter)[m_column.font_size] = 18;

   (*iter)[m_column.primary_colour] = blanc.to_string();
   (*iter)[m_column.secondary_colour] = jaune.to_string();
   (*iter)[m_column.outline_colour] = marron.to_string();
   (*iter)[m_column.shadow_colour] = noir.to_string();

   (*iter)[m_column.bold] = false;
   (*iter)[m_column.italic] = false;
   (*iter)[m_column.underline] = false;
   (*iter)[m_column.strikeout] = false;

   (*iter)[m_column.scale_x] = 100;
   (*iter)[m_column.scale_y] = 100;
   (*iter)[m_column.spacing] = 0;
   (*iter)[m_column.angle] = 0;

   (*iter)[m_column.border_style] = 1;
   (*iter)[m_column.outline] = 0;
   (*iter)[m_column.shadow] = 0;

   (*iter)[m_column.alignment] = 2;

   (*iter)[m_column.margin_l] = 20;
   (*iter)[m_column.margin_r] = 20;
   (*iter)[m_column.margin_v] = 20;

   (*iter)[m_column.alpha_level] = 0;
   (*iter)[m_column.encoding] = 0;

   return iter;
}

// retourne une copy de iter
Gtk::TreeIter StyleModel::copy(Gtk::TreeIter iter) {
   Gtk::TreeIter it = append();

   (*it)[m_column.name] = (Glib::ustring)(*iter)[m_column.name];

   (*it)[m_column.font_name] = (Glib::ustring)(*iter)[m_column.font_name];
   (*it)[m_column.font_size] = static_cast<double>((*iter)[m_column.font_size]);

   (*it)[m_column.primary_colour] = (Glib::ustring)(*iter)[m_column.primary_colour];
   (*it)[m_column.secondary_colour] = (Glib::ustring)(*iter)[m_column.secondary_colour];
   (*it)[m_column.outline_colour] = (Glib::ustring)(*iter)[m_column.outline_colour];
   (*it)[m_column.shadow_colour] = (Glib::ustring)(*iter)[m_column.shadow_colour];

   (*it)[m_column.bold] = static_cast<bool>((*iter)[m_column.bold]);
   (*it)[m_column.italic] = static_cast<bool>((*iter)[m_column.italic]);
   (*it)[m_column.underline] = static_cast<bool>((*iter)[m_column.underline]);
   (*it)[m_column.strikeout] = static_cast<bool>((*iter)[m_column.strikeout]);

   (*it)[m_column.scale_x] = (unsigned int)(*iter)[m_column.scale_x];
   (*it)[m_column.scale_y] = (unsigned int)(*iter)[m_column.scale_y];
   (*it)[m_column.spacing] = (unsigned int)(*iter)[m_column.spacing];
   (*it)[m_column.angle] = (unsigned int)(*iter)[m_column.angle];

   (*it)[m_column.border_style] = (unsigned int)(*iter)[m_column.border_style];
   (*it)[m_column.outline] = (unsigned int)(*iter)[m_column.outline];
   (*it)[m_column.shadow] = (unsigned int)(*iter)[m_column.shadow];

   (*it)[m_column.alignment] = (unsigned int)(*iter)[m_column.alignment];

   (*it)[m_column.margin_l] = (unsigned int)(*iter)[m_column.margin_l];
   (*it)[m_column.margin_r] = (unsigned int)(*iter)[m_column.margin_r];
   (*it)[m_column.margin_v] = (unsigned int)(*iter)[m_column.margin_v];

   (*it)[m_column.alpha_level] = static_cast<int>((*iter)[m_column.alpha_level]);
   (*it)[m_column.encoding] = static_cast<int>((*iter)[m_column.encoding]);

   return it;
}

// copy src dans this
void StyleModel::copy(Glib::RefPtr<StyleModel> src) {
#define SET(col, cast) (*new_it)[m_column.col] = (cast)(*it)[m_column.col]

   g_return_if_fail(src);

   Gtk::TreeNodeChildren rows = src->children();

   for (Gtk::TreeIter it = rows.begin(); it; ++it) {
      Gtk::TreeIter new_it = Gtk::ListStore::append();

      SET(name, Glib::ustring);
      SET(font_name, Glib::ustring);
      SET(font_size, double);

      SET(primary_colour, Glib::ustring);
      SET(secondary_colour, Glib::ustring);
      SET(outline_colour, Glib::ustring);
      SET(shadow_colour, Glib::ustring);

      SET(bold, bool);
      SET(italic, bool);
      SET(underline, bool);
      SET(strikeout, bool);

      SET(scale_x, unsigned int);
      SET(scale_y, unsigned int);
      SET(spacing, unsigned int);
      SET(angle, unsigned int);

      SET(border_style, unsigned int);
      SET(outline, unsigned int);
      SET(shadow, unsigned int);
      SET(alignment, unsigned int);

      SET(margin_l, unsigned int);
      SET(margin_r, unsigned int);
      SET(margin_v, unsigned int);

      SET(alpha_level, int);
      SET(encoding, int);
   }

#undef SET
}
