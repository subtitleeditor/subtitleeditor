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

#include "subtitlemodel.h"

#include <gtkmm/treemodel.h>

#include "command.h"
#include "debug.h"
#include "document.h"
#include "i18n.h"

class AddSubtitleCommand : public Command {
  public:
   AddSubtitleCommand(Document* doc, const Gtk::TreeIter& iter) : Command(doc, _("Add Subtitle")) {
      Subtitle sub(doc, iter);
      sub.get(m_backup);
   }

   void execute() {
      Glib::ustring path = m_backup["path"];

      Gtk::TreeIter iter = get_document_subtitle_model()->append();

      Subtitle sub(document(), iter);
      sub.set(m_backup);

      get_document_subtitle_model()->move(iter, get_document_subtitle_model()->get_iter(path));

      get_document_subtitle_model()->rebuild_column_num();
   }

   void restore() {
      Gtk::TreeIter iter = get_document_subtitle_model()->get_iter(m_backup["path"]);
      get_document_subtitle_model()->erase(iter);
      get_document_subtitle_model()->rebuild_column_num();
   }

  protected:
   std::map<Glib::ustring, Glib::ustring> m_backup;
};

class RemoveSubtitleCommand : public Command {
  public:
   RemoveSubtitleCommand(Document* doc, const Gtk::TreeIter& iter) : Command(doc, _("Remove Subtitle")) {
      Subtitle sub(doc, iter);
      sub.get(m_backup);
   }

   void execute() {
      Gtk::TreeIter iter = get_document_subtitle_model()->get_iter(m_backup["path"]);
      get_document_subtitle_model()->erase(iter);
      get_document_subtitle_model()->rebuild_column_num();
   }

   void restore() {
      Glib::ustring path = m_backup["path"];

      Gtk::TreeIter iter = get_document_subtitle_model()->append();

      Subtitle sub(document(), iter);
      sub.set(m_backup);

      get_document_subtitle_model()->move(iter, get_document_subtitle_model()->get_iter(path));

      get_document_subtitle_model()->rebuild_column_num();
   }

  protected:
   std::map<Glib::ustring, Glib::ustring> m_backup;
};

SubtitleModel::SubtitleModel(Document* doc) : m_document(doc) {
   set_column_types(m_column);
}

Gtk::TreeIter SubtitleModel::append() {
   Gtk::TreeIter it = Gtk::ListStore::append();
   init(it);

   (*it)[m_column.num] = getSize();
   return it;
}

// insert sub avant iter et retourne l'iter de sub
// et declale tout les autres (sub.num)
Gtk::TreeIter SubtitleModel::insertBefore(Gtk::TreeIter& iter) {
   Gtk::TreeIter res = insert(iter);
   init(res);
   (*res)[m_column.num] = (unsigned int)(*iter)[m_column.num];

   for (; iter; ++iter) {
      (*iter)[m_column.num] = (*iter)[m_column.num] + 1;
   }
   return res;
}

// insert sub apres iter et retourne l'iter de sub
// et declale tout les autres (sub.num)
Gtk::TreeIter SubtitleModel::insertAfter(Gtk::TreeIter& iter) {
   Gtk::TreeIter res = insert_after(iter);
   init(res);

   (*res)[m_column.num] = (*iter)[m_column.num] + 1;

   ++iter;  // le nouveau ajouter
   ++iter;  // le suivant on commence a partir de lui

   for (; iter; ++iter) {
      (*iter)[m_column.num] = (*iter)[m_column.num] + 1;
   }

   return res;
}

// efface un subtitle, on init les suivants avec le bon num
void SubtitleModel::remove(Gtk::TreeIter& it) {
   Gtk::TreeIter iter = erase(it);
   for (; iter; ++iter) {
      (*iter)[m_column.num] = (*iter)[m_column.num] - 1;
   }
}

void SubtitleModel::remove(unsigned int start, unsigned int end) {
   g_return_if_fail(end > start);

   Gtk::TreeIter a = find(start);
   Gtk::TreeIter b = find(end);

   g_return_if_fail(a);
   // g_return_if_fail(b);

   if (b) {
      ++b;
      for (; a != b;) {
         a = erase(a);
      }

      // on decale num des suivants si il y en a
      if (b) {
         int diff = end - start + 1;
         for (; b; ++b) {
            (*b)[m_column.num] = (*b)[m_column.num] - diff;
         }
      }
   } else {
      for (; a;) {
         a = erase(a);
      }
   }
}

// init l'iter a 0
void SubtitleModel::init(Gtk::TreeIter& iter) {
   (*iter)[m_column.num] = 0;

   // The visual value. Depend of *_value

   Glib::ustring default_view_value = (m_document->get_edit_timing_mode() == TIME) ? SubtitleTime::null() : "0";

   // The real value of time
   (*iter)[m_column.start_value] = 0;
   (*iter)[m_column.end_value] = 0;
   (*iter)[m_column.duration_value] = 0;

   (*iter)[m_column.text] = "";

   (*iter)[m_column.layer] = "0";
   (*iter)[m_column.style] = "Default";
   // (*iter)[m_column.name] = "";

   (*iter)[m_column.marginL] = "0";
   (*iter)[m_column.marginR] = "0";
   (*iter)[m_column.marginV] = "0";

   // (*iter)[m_column.effect] = "";
   // (*iter)[m_column.translation] = "";

   (*iter)[m_column.characters_per_line_text] = "0";
   (*iter)[m_column.characters_per_line_translation] = "0";
}

// retourne le premier element de la list
// ou un iterator invalide
Gtk::TreeIter SubtitleModel::getFirst() {
   if (getSize() > 0) {
      Gtk::TreeNodeChildren rows = children();
      return rows.begin();
   }
   Gtk::TreeIter nul;
   return nul;
}

// retourne le dernier element de la list
// ou un iterator invalide
Gtk::TreeIter SubtitleModel::getLast() {
   Gtk::TreeNodeChildren rows = children();

   if (!rows.empty()) {
      return rows[rows.size() - 1];
   }

   Gtk::TreeIter nul;
   return nul;
}

// retourne le nombre d'element dans la list
unsigned int SubtitleModel::getSize() {
   // Gtk::TreeNodeChildren rows = children();
   return children().size();
}

// FONCTION DE RECHERCHE

// recherche un subtitle
// grace a son numero
Gtk::TreeIter SubtitleModel::find(unsigned int num) {
   Gtk::TreeNodeChildren rows = children();
   for (Gtk::TreeIter it = rows.begin(); it; ++it) {
      if ((*it)[m_column.num] == num)
         return it;
   }
   Gtk::TreeIter nul;
   return nul;
}

Gtk::TreeIter SubtitleModel::find(const SubtitleTime& time) {
   // We need to convert time to frame if the current model is frame based.
   long val = 0;
   if (m_document->get_timing_mode() == TIME)
      val = time.totalmsecs;
   else
      val = SubtitleTime::time_to_frame(time, get_framerate_value(m_document->get_framerate()));

   Gtk::TreeNodeChildren rows = children();
   for (Gtk::TreeIter it = rows.begin(); it; ++it) {
      if (val >= long((*it)[m_column.start_value]) && val <= long((*it)[m_column.end_value]))
         return it;
   }
   Gtk::TreeIter nul;
   return nul;
}

// hack ?
bool compare_str(const Glib::ustring& src, const Glib::ustring& txt) {
   unsigned int size = src.size();
   if (txt.size() < size) {
      for (unsigned int i = 0; i <= size - txt.size(); ++i) {
         if (src.substr(i, txt.size()) == txt) {
            return true;
         }
      }
   }

   return false;
}

// recherche a partir de start (+1) dans le text des subtitles
Gtk::TreeIter SubtitleModel::find_text(Gtk::TreeIter& start, const Glib::ustring& text) {
   if (start) {
      Glib::ustring it_text;
      Gtk::TreeIter it = start;
      ++it;

      for (; it; ++it) {
         it_text = (*it)[m_column.text];

         if (compare_str(it_text, text))
            return it;
      }
   }
   Gtk::TreeIter nul;
   return nul;
}

// recherche l'iterator precedant iter
Gtk::TreeIter SubtitleModel::find_previous(const Gtk::TreeIter& iter) {
   Gtk::TreeIter res;
   Gtk::TreeNodeChildren rows = children();
   for (Gtk::TreeIter it = rows.begin(); it; ++it) {
      if (it == iter)
         return res;

      res = it;
   }
   return res;
}

// recherche l'iterator suivant iter
// (c'est pour la forme dans notre cas un simple ++iter donne la solution)
Gtk::TreeIter SubtitleModel::find_next(const Gtk::TreeIter& iter) {
   Gtk::TreeIter res = iter;
   ++res;
   return res;
}

// FONCTION D'EDITION

// fait une copy de src
// dans this
void SubtitleModel::copy(Glib::RefPtr<SubtitleModel> src) {
   g_return_if_fail(src);

#define SET(col, cast) (*new_it)[m_column.col] = (cast)(*it)[m_column.col]

   Gtk::TreeNodeChildren rows = src->children();

   for (Gtk::TreeIter it = rows.begin(); it; ++it) {
      Gtk::TreeIter new_it = Gtk::ListStore::append();

      SET(num, unsigned int);

      SET(layer, Glib::ustring);

      SET(start_value, long);
      SET(end_value, long);
      SET(duration_value, long);

      SET(style, Glib::ustring);
      SET(name, Glib::ustring);

      SET(marginL, Glib::ustring);
      SET(marginR, Glib::ustring);
      SET(marginV, Glib::ustring);

      SET(effect, Glib::ustring);

      SET(text, Glib::ustring);

      SET(translation, Glib::ustring);

      SET(characters_per_line_text, Glib::ustring);
      SET(characters_per_line_translation, Glib::ustring);

      SET(note, Glib::ustring);
   }
#undef SET
}

// check la colonne num pour init de [1,size]
void SubtitleModel::rebuild_column_num() {
   unsigned int id = 1;
   Gtk::TreeNodeChildren rows = children();

   for (Gtk::TreeIter it = rows.begin(); it; ++it, ++id) {
      (*it)[m_column.num] = id;
   }
}

bool SubtitleModel::drag_data_delete_vfunc(const TreeModel::Path& path) {
   m_document->add_command(new RemoveSubtitleCommand(m_document, get_iter(path)));
   m_document->finish_command();

   bool res = Gtk::ListStore::drag_data_delete_vfunc(path);

   rebuild_column_num();

   return res;
}

bool SubtitleModel::drag_data_received_vfunc(const TreeModel::Path& dest, const Gtk::SelectionData& selection_data) {
   Gtk::TreePath src;
   Gtk::TreePath::get_from_selection_data(selection_data, src);

   Gtk::ListStore::drag_data_received_vfunc(dest, selection_data);

   m_document->start_command(_("Reordered Subtitle"));
   m_document->add_command(new AddSubtitleCommand(m_document, get_iter(dest)));

   return true;
}
