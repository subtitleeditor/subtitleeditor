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

#include <config.h>
#include <glibmm.h>

#include <vector>

class OptionGroup : public Glib::OptionGroup {
  public:
   OptionGroup();

   int get_debug_flags();

#ifdef DEBUG
   Glib::OptionGroup& get_debug_group() {
      return debug_group;
   }
#endif

  public:
   std::vector<Glib::ustring> files;
   std::vector<Glib::ustring> files_list;  // simple file (glibmm Bug #526831)

   Glib::ustring profile;                  // profile name
   Glib::ustring encoding;                 // subtitle encoding to be used
   Glib::ustring video;                    // video file location
   Glib::ustring waveform;                 // waveform file location
   Glib::ustring keyframes;                // keyframes file location

#ifdef DEBUG
   Glib::OptionGroup debug_group;

   bool debug_all;
   bool debug_app;
   bool debug_view;
   bool debug_io;
   bool debug_search;
   bool debug_regex;
   bool debug_video_player;
   bool debug_spell_checking;
   bool debug_waveform;
   bool debug_utility;
   bool debug_command;
   bool debug_plugins;
   bool debug_no_profiling;
#endif  // DEBUG
};
