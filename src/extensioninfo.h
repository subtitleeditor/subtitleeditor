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
#include "extension.h"

// This is a representation of an extension in subtitleeditor.
// Each extension have an se-plugin file (pseudo desktop file) to
// informs subtitleeditor about the extension like name, description, type...
class ExtensionInfo {
  // Only the ExtensionManager can create, delete and manage it.
  friend class ExtensionManager;

 public:
  // Return the name of the extension.
  Glib::ustring get_name() const;

  // Return the label of the extension.
  // The label is just the name translated.
  Glib::ustring get_label() const;

  // Return a description of the extension.
  Glib::ustring get_description() const;

  // Return the authors of the extension.
  Glib::ustring get_authors() const;

  // Return the type of the extension.
  // Most often it's "module"
  Glib::ustring get_type() const;

  // Return a categorie of the extension.
  Glib::ustring get_categorie() const;

  // Return the Extension instance only if the type
  // is a module or NULL;
  Extension* get_extension() const;

  // Return the state of the extension, activated or not.
  bool get_active() const;

  bool get_hidden() const;

 protected:
  // Constructor.
  ExtensionInfo();

  // Destructor.
  // Delete the extension and delete the module.
  ~ExtensionInfo();

 protected:
  Glib::ustring file;
  Glib::ustring name;
  Glib::ustring label;
  Glib::ustring description;
  Glib::ustring authors;
  Glib::ustring categorie;
  Glib::ustring type;
  Glib::ustring module_name;
  Glib::Module* module{nullptr};
  bool active{false};
  bool hidden{false};
  bool fhs_directory{false};
  Extension* extension{nullptr};
};
