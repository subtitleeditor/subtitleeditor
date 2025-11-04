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
#include <extension/action.h>
#include <gtkmm_utility.h>
#include <utility.h>

#include <memory>

class DialogSplitDocument : public Gtk::Dialog {
 public:
  DialogSplitDocument(BaseObjectType *cobject,
                      const Glib::RefPtr<Gtk::Builder> &builder)
      : Gtk::Dialog(cobject) {
    utility::set_transient_parent(*this);

    builder->get_widget("spin-number", m_spinNumber);

    set_default_response(Gtk::RESPONSE_OK);
  }

  void execute(Document *doc) {
    g_return_if_fail(doc);

    unsigned int size = doc->subtitles().size();

    if (size == 0) {
      dialog_warning(
          _("You can't use the <i>split</i> action with this document"),
          build_message(_("There are no subtitles in document <b>%s</b>. It's "
                          "empty and there is nothing to split."),
                        doc->getName().c_str()));
      return;
    }

    m_spinNumber->set_range(1, size);

    // set by default the first selected subtitle
    {
      Subtitle selected = doc->subtitles().get_first_selected();
      if (selected)
        m_spinNumber->set_value(selected.get_num());
    }
    show();

    if (run() == Gtk::RESPONSE_OK) {
      unsigned int number = (unsigned int)m_spinNumber->get_value();

      split_doc(doc, number);
    }

    hide();
  }

  // Split the document in two and return the new one
  Document *split_doc(Document *doc, unsigned int number) {
    // Create new document based on the first one and rename it
    Document *newdoc = new Document(*doc, true);
    newdoc->setFilename(newdoc->getFilename() + "-par2");
    newdoc->subtitles().remove(1, number - 1);

    se::documents::append(newdoc);

    // Remove subtitles used by the new one
    doc->start_command(_("Split document"));
    doc->subtitles().remove(number, doc->subtitles().size());
    doc->finish_command();

    return newdoc;
  }

 protected:
  Gtk::SpinButton *m_spinNumber;
};

class SplitDocumentPlugin : public Action {
 public:
  SplitDocumentPlugin() {
    activate();
    update_ui();
  }

  ~SplitDocumentPlugin() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("SplitDocumentPlugin");

    action_group->add(
        Gtk::Action::create("split-document", Gtk::Stock::CUT,
                            _("Spl_it Document"),
                            _("Split the current document in two")),
        sigc::mem_fun(*this, &SplitDocumentPlugin::on_execute));

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui_id = ui->new_merge_id();

    ui->insert_action_group(action_group);

    ui->add_ui(ui_id, "/menubar/menu-tools/split-document", "split-document",
               "split-document");
  }

  void deactivate() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui->remove_ui(ui_id);
    ui->remove_action_group(action_group);
  }

  void update_ui() {
    se_dbg(SE_DBG_PLUGINS);

    bool visible = (get_current_document() != NULL);

    action_group->get_action("split-document")->set_sensitive(visible);
  }

 protected:
  void on_execute() {
    se_dbg(SE_DBG_PLUGINS);

    Document *doc = get_current_document();
    g_return_if_fail(doc);

    // create dialog
    std::unique_ptr<DialogSplitDocument> dialog(
        gtkmm_utility::get_widget_derived<DialogSplitDocument>(
            SE_DEV_VALUE(SE_PLUGIN_PATH_UI, SE_PLUGIN_PATH_DEV),
            "dialog-split-document.ui", "dialog-split-document"));

    dialog->execute(doc);
  }

 protected:
  Gtk::UIManager::ui_merge_id ui_id;
  Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(SplitDocumentPlugin)
