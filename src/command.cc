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

#include "command.h"
#include "debug.h"
#include "document.h"

Command::Command(Document *doc, const Glib::ustring &description)
    : m_document(doc), m_description(description) {
  se_dbg_msg(SE_DBG_COMMAND, "description=%s", description.c_str());
}

Command::~Command() {
  se_dbg_msg(SE_DBG_COMMAND, "description=%s", m_description.c_str());
}

Document *Command::document() {
  return m_document;
}

Glib::ustring Command::description() const {
  return m_description;
}

SubtitleModelPtr Command::get_document_subtitle_model() {
  return document()->get_subtitle_model();
}

SubtitleViewPtr Command::get_document_subtitle_view() {
  return document()->get_subtitle_view();
}
