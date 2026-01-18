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

#include <gtkmm.h>

#include "color.h"

class StyleColumnRecorder : public Gtk::TreeModel::ColumnRecord {
  public:
   StyleColumnRecorder() {
      add(name);
      add(font_name);
      add(font_size);

      add(primary_colour);
      add(secondary_colour);
      add(outline_colour);  // outline_color
      add(shadow_colour);   // shadow

      add(bold);
      add(italic);
      add(underline);
      add(strikeout);

      add(scale_x);       // percent
      add(scale_y);       // percent
      add(spacing);       // pixel
      add(angle);         // degrees

      add(border_style);  // 1=Outline + drop shadow, 3=Opaque box
      add(outline);       // if border_style is 1, 0,1,2,3 or 4
      add(shadow);        // if border_style is 1
      // TODO : check field 13 style+
      add(alignment);  // 1=left, 2=centered, 3=right, 4=toptitle, 8=midtitle,
                       // 5=left-justified toptitle

      add(margin_l);
      add(margin_r);
      add(margin_v);
      // TODO > not present in ASS
      add(alpha_level);
      add(encoding);
   }

  public:
#define data(type, name) Gtk::TreeModelColumn<type> name;

   data(Glib::ustring, name);
   data(Glib::ustring, font_name);
   data(double, font_size);

   data(Glib::ustring, primary_colour);
   data(Glib::ustring, secondary_colour);
   data(Glib::ustring, outline_colour);  // outline_color
   data(Glib::ustring, shadow_colour);   // shadow

   data(bool, bold);
   data(bool, italic);
   data(bool, underline);
   data(bool, strikeout);

   data(unsigned int, scale_x);       // percent
   data(unsigned int, scale_y);       // percent
   data(unsigned int, spacing);       // pixel
   data(unsigned int, angle);         // degrees

   data(unsigned int, border_style);  // 1=Outline + drop shadow, 3=Opaque box
   data(unsigned int, outline);       // if border_style is 1,	0,1,2,3 or 4
   data(unsigned int, shadow);        // if border_style is 1
   // TODO : check field 13 style+
   data(unsigned int, alignment);  // 1=left, 2=centered, 3=right, 4=toptitle,
                                   // 8=midtitle, 5=left-justified toptitle

   data(unsigned int, margin_l);
   data(unsigned int, margin_r);
   data(unsigned int, margin_v);
   // TODO > not present in ASS
   data(int, alpha_level);
   data(int, encoding);
#undef data
};

class StyleModel : public Gtk::ListStore {
  public:
   StyleModel();

   Gtk::TreeIter append();

   // retourne une copy de iter
   Gtk::TreeIter copy(Gtk::TreeIter iter);

   // copy src dans this
   void copy(Glib::RefPtr<StyleModel> src);

  public:
   StyleColumnRecorder m_column;
};
