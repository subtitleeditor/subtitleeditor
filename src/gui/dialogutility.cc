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

#include "dialogutility.h"
#include "documents.h"
#include "utility.h"

// Constructor
DialogActionMultiDoc::DialogActionMultiDoc(
    BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
    : Gtk::Dialog(cobject) {
  builder->get_widget("radio-current-document", m_radioCurrentDocument);
  builder->get_widget("radio-all-documents", m_radioAllDocuments);
}

// Return true if the user choose to apply the action on all documents.
bool DialogActionMultiDoc::apply_to_all_documents() {
  return m_radioAllDocuments->get_active();
}

// Return a list of documents that the user wants to change.
DocumentList DialogActionMultiDoc::get_documents_to_apply() {
  DocumentList docs;

  if (apply_to_all_documents())
    docs = se::documents::all();
  else
    docs.push_back(se::documents::active());

  return docs;
}

ErrorDialog::ErrorDialog(const Glib::ustring& primary,
                         const Glib::ustring& secondary)
    : Gtk::MessageDialog(primary, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_NONE,
                         true) {
  utility::set_transient_parent(*this);

  if (secondary.empty() == false)
    set_secondary_text(secondary, false);
}

FramerateChooserDialog::FramerateChooserDialog(
    FramerateChooserDialog::Action action)
    : Gtk::Dialog() {
  utility::set_transient_parent(*this);

  set_title("");
  set_resizable(false);
  add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);

  Glib::ustring query;
  if (action == IMPORT)
    query = _("At what frame rate do you want to import?");
  else  // == EXPORT
    query = _("At what frame rate do you want to export?");

  query = Glib::ustring::compose(
      "<span weight=\"bold\" size=\"larger\">%1</span>", query);

  // hbox
  Gtk::HBox* hbox = manage(new Gtk::HBox(false, 12));
  hbox->set_border_width(12);
  get_vbox()->pack_start(*hbox, false, false);
  // img (info)
  Gtk::Image* img =
      manage(new Gtk::Image(Gtk::Stock::DIALOG_INFO, Gtk::ICON_SIZE_DIALOG));
  img->set_alignment(0.0, 0.0);
  hbox->pack_start(*img, false, false);
  // vbox
  Gtk::VBox* vbox = manage(new Gtk::VBox(false, 12));
  hbox->pack_start(*vbox);
  // label (query)
  Gtk::Label* label = manage(new Gtk::Label(query, 0.0, 0.0));
  label->set_use_markup(true);
  label->set_line_wrap(true);
  vbox->pack_start(*label, false, false);
  // hbox2
  Gtk::HBox* hbox2 = manage(new Gtk::HBox(false, 6));
  vbox->pack_start(*hbox2);
  // label2 (framerate:)
  Gtk::Label* label2 = manage(new Gtk::Label(_("_Framerate:"), 0.0, 0.5, true));
  hbox2->pack_start(*label2, false, false);
  // combobox
  m_comboFramerate = manage(new ComboBoxFramerate);
  hbox2->pack_start(*m_comboFramerate, false, false);

  hbox->show_all();
}

// Launch the dialog and return the framerate value.
FRAMERATE FramerateChooserDialog::execute() {
  run();
  return dynamic_cast<ComboBoxFramerate*>(m_comboFramerate)->get_value();
}

void FramerateChooserDialog::set_default_framerate(FRAMERATE framerate) {
  ComboBoxFramerate* cbf = dynamic_cast<ComboBoxFramerate*>(m_comboFramerate);
  cbf->set_value(framerate);
}
