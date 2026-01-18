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

#include "subtitleformatio.h"

#include "error.h"
#include "i18n.h"

SubtitleFormatIO::SubtitleFormatIO() {
}

SubtitleFormatIO::~SubtitleFormatIO() {
}

void SubtitleFormatIO::set_document(Document* document) {
   m_document = document;
}

Document* SubtitleFormatIO::document() {
   return m_document;
}

void SubtitleFormatIO::open(Reader& /*reader*/) {
   throw IOFileError(_("This function is not implemented for this format."));
}

void SubtitleFormatIO::save(Writer& /*writer*/) {
   throw IOFileError(_("This function is not implemented for this format."));
}
