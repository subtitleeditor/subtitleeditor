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

#include <glibmm/ustring.h>

#include <string>
#include <vector>

#include "document.h"

namespace se {
namespace documents {

using Glib::ustring;
using sigc::signal;
using std::string;
using std::vector;

typedef signal<void, Document*> signal_document;
typedef signal<void, Document*, const string&> signal_document_modified;

void append(Document* doc);

void remove(Document* doc);

vector<Document*> all();

void active(Document*);
Document* active();

signal_document& signal_created();
signal_document& signal_deleted();
signal_document& signal_active_changed();
signal_document_modified& signal_modified();

Document* find_by_name(const ustring& name);

ustring generate_untitled_name(const ustring& ext = "");

}  // namespace documents
}  // namespace se
