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

#include "debug.h"
#include "extensioninfo.h"

// Constructor.
ExtensionInfo::ExtensionInfo() {
  se_dbg(SE_DBG_APP);
}

// Destructor.
// Delete the extension and delete the module.
ExtensionInfo::~ExtensionInfo() {
  se_dbg(SE_DBG_APP);

  delete extension;
  delete module;
}

// Return the name of the extension.
Glib::ustring ExtensionInfo::get_name() const {
  return name;
}

// Return the label of the extension.
// The label is just the name translated.
Glib::ustring ExtensionInfo::get_label() const {
  return label;
}

// Return a description of the extension.
Glib::ustring ExtensionInfo::get_description() const {
  return description;
}

// Return the authors of the extension.
Glib::ustring ExtensionInfo::get_authors() const {
  return authors;
}

// Return a categorie of the extension.
Glib::ustring ExtensionInfo::get_categorie() const {
  return categorie;
}

// Return the type of the extension.
// Most often it's "module"
Glib::ustring ExtensionInfo::get_type() const {
  return type;
}

// Return the Extension instance only if the type
// is a module or NULL;
Extension* ExtensionInfo::get_extension() const {
  return extension;
}

// Return the state of the extension, activated or not.
bool ExtensionInfo::get_active() const {
  return active;
}

// Return the state of the extension, activated or not.
bool ExtensionInfo::get_hidden() const {
  return hidden;
}
