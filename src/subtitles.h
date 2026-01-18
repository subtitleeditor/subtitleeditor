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

#include <vector>

#include "subtitle.h"

class Document;

class Subtitles {
  public:
   Subtitles(Document& doc);
   ~Subtitles();

   unsigned int size();

   Subtitle get(unsigned int num);

   Subtitle get_first();

   Subtitle get_last();

   Subtitle get_previous(const Subtitle& sub);

   Subtitle get_next(const Subtitle& sub);

   Subtitle append();

   Subtitle insert_before(const Subtitle& sub);

   Subtitle insert_after(const Subtitle& sub);

   void remove(std::vector<Subtitle>& subs);

   void remove(unsigned int start, unsigned int end);

   // Prefer the function using an array if there is a need to remove several
   // subtitles.
   void remove(const Subtitle& sub);

   Subtitle find(const SubtitleTime& time);

   // Selection

   std::vector<Subtitle> get_selection();

   Subtitle get_first_selected();

   Subtitle get_last_selected();

   void select(const std::vector<Subtitle>& sub);
   void select(const std::list<Subtitle>& sub);

   void select(const Subtitle& sub, bool start_editing = false);

   bool is_selected(const Subtitle& sub);

   void unselect(const Subtitle& sub);

   void select_all();

   void unselect_all();

   void invert_selection();

   guint sort_by_time();

  protected:
   Document& m_document;
};
