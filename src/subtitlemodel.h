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

#include "subtitletime.h"

class NameModel : public Gtk::ListStore {
  public:
   class Column : public Gtk::TreeModel::ColumnRecord {
     public:
      Column() {
         add(name);
      }

      Gtk::TreeModelColumn<Glib::ustring> name;
   };

  public:
   // constructor
   NameModel() {
      set_column_types(m_column);
   }

  public:
   Column m_column;
};

class SubtitleColumnRecorder : public Gtk::TreeModel::ColumnRecord {
  public:
   SubtitleColumnRecorder() {
      add(num);
      add(layer);
      add(start_value);
      add(end_value);
      add(duration_value);
      add(gap_before);
      add(gap_after);
      add(style);
      add(name);
      add(marginL);
      add(marginR);
      add(marginV);
      add(effect);
      add(text);
      add(translation);
      add(characters_per_line_text);
      add(characters_per_second_text);
      add(characters_per_line_translation);
      add(note);
   }

   Gtk::TreeModelColumn<unsigned int> num;

   Gtk::TreeModelColumn<Glib::ustring> layer;

   Gtk::TreeModelColumn<long> start_value;
   Gtk::TreeModelColumn<long> end_value;
   Gtk::TreeModelColumn<long> duration_value;

   Gtk::TreeModelColumn<long> gap_before;
   Gtk::TreeModelColumn<long> gap_after;

   Gtk::TreeModelColumn<Glib::ustring> style;
   Gtk::TreeModelColumn<Glib::ustring> name;

   Gtk::TreeModelColumn<Glib::ustring> marginL;
   Gtk::TreeModelColumn<Glib::ustring> marginR;
   Gtk::TreeModelColumn<Glib::ustring> marginV;

   Gtk::TreeModelColumn<Glib::ustring> effect;

   Gtk::TreeModelColumn<Glib::ustring> text;

   Gtk::TreeModelColumn<Glib::ustring> translation;
   Gtk::TreeModelColumn<Glib::ustring> characters_per_line_text;
   Gtk::TreeModelColumn<Glib::ustring> characters_per_line_translation;
   Gtk::TreeModelColumn<Glib::ustring> note;

   Gtk::TreeModelColumn<double> characters_per_second_text;
};

class Document;

class SubtitleModel : public Gtk::ListStore {
  public:
   SubtitleModel(Document* doc);

   // num = 0, start=end=0, ...
   void init(Gtk::TreeIter& iter);

   Gtk::TreeIter append();

   // retourne le premier element de la list
   // ou un iterator invalide
   Gtk::TreeIter getFirst();

   // retourne le dernier element de la list
   // ou un iterator invalide
   Gtk::TreeIter getLast();

   // retourne le nombre d'element dans la list
   unsigned int getSize();

   // FONCTION DE RECHERCHE

   // recherche un subtitle
   // grace a son numero
   Gtk::TreeIter find(unsigned int num);

   // recherche un subtitle grace a son temps
   // si time est compris entre start et end
   Gtk::TreeIter find(const SubtitleTime& time);

   // recherche a partir de start (+1) dans le text des subtitles
   Gtk::TreeIter find_text(Gtk::TreeIter& start, const Glib::ustring& text);

   // recherche l'iterator precedant iter
   Gtk::TreeIter find_previous(const Gtk::TreeIter& iter);

   // recherche l'iterator suivant iter
   // (c'est pour la forme dans notre cas un simple ++iter donne la solution)
   Gtk::TreeIter find_next(const Gtk::TreeIter& iter);

   // FONCTION D'EDITION

   // insert sub avant iter et retourne l'iter de sub
   // puis declale tout les autres (num)
   Gtk::TreeIter insertBefore(Gtk::TreeIter& iter);

   // insert sub apres iter et retourne l'iter de sub
   // et declale tout les autres (num)
   Gtk::TreeIter insertAfter(Gtk::TreeIter& iter);

   // efface un subtitle, on init les suivants avec le bon num
   void remove(Gtk::TreeIter& iter);

   // efface des elements de start a end
   // [start,end]
   void remove(unsigned int start, unsigned int end);

   // fait une copy de src dans this
   void copy(Glib::RefPtr<SubtitleModel> src);

   // check la colonne num pour init de [1,size]
   void rebuild_column_num();

  protected:
   virtual bool drag_data_delete_vfunc(const TreeModel::Path& path);

   virtual bool drag_data_received_vfunc(const TreeModel::Path& dest, const Gtk::SelectionData& selection_data);

  protected:
   Document* m_document;
   SubtitleColumnRecorder m_column;

   sigc::signal<void, const Gtk::TreePath&, const Gtk::TreePath&> m_my_signal_row_reorderer;
};
