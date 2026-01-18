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

#include "documents.h"

#include "i18n.h"
#include "utility.h"

namespace se {
namespace documents {

namespace internal {

struct manager {
   ~manager() {
      active = nullptr;
      for (auto doc : documents) {
         delete doc;
      }
      documents.clear();
   }

  public:
   Document* active{nullptr};
   vector<Document*> documents;
   // signals
   signal_document signal_created;
   signal_document signal_deleted;
   signal_document signal_active_changed;
   signal_document_modified signal_modified;
};

static manager instance;

}  // namespace internal

using internal::instance;
using std::find;

void append(Document* doc) {
   instance.documents.push_back(doc);
   instance.signal_created(doc);
}

void remove(Document* doc) {
   auto it = find(instance.documents.begin(), instance.documents.end(), doc);
   instance.documents.erase(it);

   if (instance.active == doc) {
      active(nullptr);
   }
   instance.signal_deleted(doc);
   delete doc;
}

vector<Document*> all() {
   return instance.documents;
}

void active(Document* doc) {
   instance.active = doc;
   instance.signal_active_changed(doc);
}

Document* active() {
   return instance.active;
}

signal_document& signal_created() {
   return instance.signal_created;
}

signal_document& signal_deleted() {
   return instance.signal_deleted;
}

signal_document& signal_active_changed() {
   return instance.signal_active_changed;
}

signal_document_modified& signal_modified() {
   return instance.signal_modified;
}

Document* find_by_name(const ustring& name) {
   for (const auto& doc : instance.documents) {
      if (doc->getName() == name) {
         return doc;
      }
   }
   return nullptr;
}

ustring generate_untitled_name(const ustring& extension) {
   ustring ext = extension.empty() ? "" : "." + extension;
   int num = 1;
   while (find_by_name(build_message(_("Untitled %d"), num) + ext) != nullptr) {
      ++num;
   }
   return build_message(_("Untitled %d"), num) + ext;
}

}  // namespace documents
}  // namespace se
