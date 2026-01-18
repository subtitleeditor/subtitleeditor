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

#include <documents.h>
#include <gtkmm.h>
#include <gui/comboboxframerate.h>

class DialogActionMultiDoc : public Gtk::Dialog {
  public:
   // Constructor
   DialogActionMultiDoc(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);

   // Return true if the user choose to apply the action on all documents.
   bool apply_to_all_documents();

   // Return a list of documents that the user wants to change.
   DocumentList get_documents_to_apply();

  protected:
   Gtk::RadioButton* m_radioCurrentDocument;
   Gtk::RadioButton* m_radioAllDocuments;
};

class ErrorDialog : public Gtk::MessageDialog {
  public:
   ErrorDialog(const Glib::ustring& primary, const Glib::ustring& secondary = Glib::ustring());
};

class FramerateChooserDialog : public Gtk::Dialog {
  public:
   enum Action { IMPORT, EXPORT };

   explicit FramerateChooserDialog(Action action = IMPORT);

   // Launch the dialog and return the framerate value.
   FRAMERATE execute();

   void set_default_framerate(FRAMERATE framerate);

  protected:
   Gtk::ComboBox* m_comboFramerate;
};
