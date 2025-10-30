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

#include <debug.h>
#include <gtkmm.h>
#include "cellrenderercustom.h"

template <class T>
class CellRendererCustom : public Gtk::CellRendererText {
 public:
  CellRendererCustom();

  virtual Gtk::CellEditable* start_editing_vfunc(
      GdkEvent* event, Gtk::Widget& widget, const Glib::ustring& path,
      const Gdk::Rectangle& background_area, const Gdk::Rectangle& cell_area,
      Gtk::CellRendererState flags);

 protected:
  // Disable all actions.
  virtual void begin_editing();

  // Enable all actions.
  virtual void finish_editing();

  void cell_editing_done(const Glib::ustring& path);

 protected:
  T* m_editable;
};

template <class T>
CellRendererCustom<T>::CellRendererCustom()
    : Glib::ObjectBase(typeid(CellRendererCustom)),
      Gtk::CellRendererText(),
      m_editable(NULL) {
  se_dbg(SE_DBG_VIEW);
}

template <class T>
Gtk::CellEditable* CellRendererCustom<T>::start_editing_vfunc(
    GdkEvent* /*event*/, Gtk::Widget& /*widget*/, const Glib::ustring& path,
    const Gdk::Rectangle& /*background_area*/, const Gdk::Rectangle& cell_area,
    Gtk::CellRendererState /*flags*/) {
  se_dbg(SE_DBG_VIEW);

  if (!property_editable())
    return nullptr;

  m_editable = manage(new T);
  m_editable->set_size_request(cell_area.get_width(), cell_area.get_height());

  m_editable->signal_editing_done().connect(sigc::bind(
      sigc::mem_fun(*this, &CellRendererCustom<T>::cell_editing_done), path));

  // prepare widget
  if (Gtk::Entry* entry = dynamic_cast<Gtk::Entry*>(m_editable)) {
    entry->set_has_frame(false);
    entry->set_alignment(property_xalign());
  }
  m_editable->set_text(property_text());

  // Begin/Finish editing (Fix #10494)
  // Disable actions during editing. Enable at the exit.
  begin_editing();

  m_editable->signal_remove_widget().connect(
      sigc::mem_fun(*this, &CellRendererCustom<T>::finish_editing));

  m_editable->show();
  return m_editable;
}

// Disable all actions.
template <class T>
void CellRendererCustom<T>::begin_editing() {
  se_dbg(SE_DBG_VIEW);
}

// Enable all actions.
template <class T>
void CellRendererCustom<T>::finish_editing() {
  se_dbg(SE_DBG_VIEW);
}

template <class T>
void CellRendererCustom<T>::cell_editing_done(const Glib::ustring& path) {
  se_dbg(SE_DBG_VIEW);

  if (m_editable == NULL) {
    se_dbg_msg(SE_DBG_VIEW, "m_editable is NULL");
    return;
  }

  Glib::ustring text = m_editable->get_text();
  se_dbg_msg(SE_DBG_VIEW, "text from editable='%s'", text.c_str());

  // Define to NULL to avoid a duplicate call
  m_editable = NULL;
  edited(path, text);

  finish_editing();
}
