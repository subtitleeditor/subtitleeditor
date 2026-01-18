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

#include <list>
#include <map>

#include "extensioninfo.h"

class ExtensionManager {
  public:
   // Return the ExtensionManager instance.
   static ExtensionManager& instance();

   // Return all ExtensionInfo.
   std::list<ExtensionInfo*> get_extension_info_list();

   // Return all ExtensionInfo in the categorie.
   std::list<ExtensionInfo*> get_info_list_from_categorie(const Glib::ustring& categorie);

   // Return an ExtensionInfo from this name or NULL if failed.
   ExtensionInfo* get_extension_info(const Glib::ustring& name);

   // Enable or disable extension.
   bool set_extension_active(const Glib::ustring& name, bool state);

   // Active and create extensions
   void create_extensions();

   // Delete and close all extensions
   void destroy_extensions();

  protected:
   // Constructor
   ExtensionManager();

   // Destructor
   ~ExtensionManager();

   // Load the path and sub path to find extension description.
   // se-plugin file.
   void load_path(const Glib::ustring& path, bool fhs_directory);

   // Try to load an ExtensionInfo file.
   bool load_extension_info(const Glib::ustring& file, bool fhs_directory);

   // Try to activate the extension.
   // Load and create Extension.
   bool activate(ExtensionInfo* info);

   // Deactivate the extension.
   // Delete the extension and the module.
   bool deactivate(ExtensionInfo* info);

   // Open a module and create the extension.
   // If failed return a SubtitleError.
   void open_module(ExtensionInfo* info);

  protected:
   typedef std::map<Glib::ustring, std::list<ExtensionInfo*> > ExtensionInfoMap;

   ExtensionInfoMap m_extension_info_map;
};
