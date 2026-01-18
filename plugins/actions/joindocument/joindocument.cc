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
#include <extension/action.h>
#include <gtkmm_utility.h>
#include <gui/dialogfilechooser.h>
#include <gui/spinbuttontime.h>
#include <i18n.h>
#include <player.h>
#include <subtitleeditorwindow.h>
#include <utility.h>
#include <widget_config_utility.h>

#include <string>

class DialogJoinOffset : public Gtk::Dialog {
  public:
   DialogJoinOffset(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) : Gtk::Dialog(cobject) {
      utility::set_transient_parent(*this);
      builder->get_widget_derived("spin-offset", m_spinOffset);
   }

   void init(Document* doc, const Subtitle& last_subtitle) {
      TIMING_MODE edit_mode = doc->get_edit_timing_mode();
      m_spinOffset->set_timing_mode(edit_mode);

      long default_offset = 0;

      // Try to prefill with video end time
      Player* player = SubtitleEditorWindow::get_instance()->get_player();
      if (player && player->get_state() != Player::NONE)
         default_offset = std::max<long>(0, player->get_duration());
      else
         default_offset = last_subtitle.get_end().totalmsecs;

      m_spinOffset->set_value(static_cast<double>(default_offset));
      m_spinOffset->grab_focus();
   }

   long get_offset_value() const {
      return static_cast<long>(m_spinOffset->get_value());
   }

  protected:
   SpinButtonTime* m_spinOffset;
};

class JoinDocumentPlugin : public Action {
  public:
   JoinDocumentPlugin() {
      activate();
      update_ui();
   }

   ~JoinDocumentPlugin() {
      deactivate();
   }

   void activate() {
      se_dbg(SE_DBG_PLUGINS);

      // actions
      action_group = Gtk::ActionGroup::create("JoinDocumentPlugin");

      action_group->add(Gtk::Action::create("join-document",
                                            Gtk::Stock::CONNECT,
                                            _("_Join Document"),
                                            _("Add subtitles from a file to the current document, adjusting "
                                              "timecodes by given offset. If a video is open, its duration is "
                                              "offered as the offset. If no video is open, the end time of the "
                                              "last subtitle is offered as the offset.")),
                        sigc::mem_fun(*this, &JoinDocumentPlugin::on_execute_join));

      action_group->add(
         Gtk::Action::create("append-document", Gtk::Stock::ADD, _("_Append Document"), _("Append subtitles from file without changing timecodes")),
         sigc::mem_fun(*this, &JoinDocumentPlugin::on_execute_append));

      // ui
      Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

      ui_id = ui->new_merge_id();

      ui->insert_action_group(action_group);

      ui->add_ui(ui_id, "/menubar/menu-tools/join-document", "join-document", "join-document");
      ui->add_ui(ui_id, "/menubar/menu-tools/append-document", "append-document", "append-document");
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

      action_group->get_action("join-document")->set_sensitive(visible);
      action_group->get_action("append-document")->set_sensitive(visible);
   }

  protected:
   bool applying_offset;
   void on_execute_join() {
      se_dbg(SE_DBG_PLUGINS);
      applying_offset = true;
      execute(applying_offset);
   }

   void on_execute_append() {
      se_dbg(SE_DBG_PLUGINS);
      applying_offset = false;
      execute(applying_offset);
   }

   bool execute(bool applying_offset) {
      se_dbg(SE_DBG_PLUGINS);

      Document* doc = get_current_document();

      g_return_val_if_fail(doc, false);

      // Get the original document info before opening the file dialog
      unsigned int subtitle_size = doc->subtitles().size();

      DialogOpenDocument::unique_ptr ui = DialogOpenDocument::create();

      ui->show_video(false);
      ui->set_select_multiple(false);

      if (ui->run() != Gtk::RESPONSE_OK)
         return false;

      Glib::ustring uri = ui->get_uri();

      // tmp document to try to open the file
      Document* tmp = Document::create_from_file(uri);
      if (tmp == NULL)
         return false;

      ui->hide();  // hides (closes?) the dialog so the next one can display

      Glib::ustring encoding = tmp->getCharset();
      delete tmp;

      // Declare offset variable outside the if block so it's available later
      SubtitleTime offset = 0;

      // If we are joining, we ask for offset first, otherwise the subtitles
      // get joined without offset in the background, which looks ugly
      if (applying_offset) {
         // Get the last subtitle of the original document
         Subtitle last_orig_sub = doc->subtitles().get(subtitle_size);

         // Show offset dialog
         std::unique_ptr<DialogJoinOffset> offset_dialog(gtkmm_utility::get_widget_derived<DialogJoinOffset>(
            SE_DEV_VALUE(SE_PLUGIN_PATH_UI, SE_PLUGIN_PATH_DEV), "dialog-join-offset.ui", "dialog-join-offset"));

         offset_dialog->init(doc, last_orig_sub);

         if (offset_dialog->run() == Gtk::RESPONSE_OK) {
            offset = offset_dialog->get_offset_value();
         } else {
            doc->flash_message(_("Join cancelled."));
            return false;
         }
      }

      Glib::ustring ofile = doc->getFilename();
      Glib::ustring oformat = doc->getFormat();
      Glib::ustring ocharset = doc->getCharset();

      try {  // needs with Document::open
         doc->start_command(applying_offset ? _("Join document") : _("Append document"));
         doc->setCharset(encoding);
         doc->open(uri);

         // Get The first subtitle added to the original document
         Subtitle last_orig_sub = doc->subtitles().get(subtitle_size);
         Subtitle first_new_subs = doc->subtitles().get_next(last_orig_sub);

         // Now we apply offset
         if (applying_offset) {
            SubtitleTime min_gap = cfg::get_int("timing", "min-gap-between-subtitles");
            SubtitleTime gap = offset - last_orig_sub.get_end();
            se_dbg_msg(SE_DBG_PLUGINS, "First new %s", first_new_subs.get_start().str().c_str());
            se_dbg_msg(SE_DBG_PLUGINS, "Offset %s", offset.str().c_str());
            se_dbg_msg(SE_DBG_PLUGINS, "Last_orig %s", last_orig_sub.get_end().str().c_str());
            se_dbg_msg(SE_DBG_PLUGINS, "Min_gap %s", min_gap.str().c_str());
            se_dbg_msg(SE_DBG_PLUGINS, "Gap %s", gap.str().c_str());
            if (gap < min_gap)
               offset = offset + min_gap - gap;

            for (Subtitle sub = first_new_subs; sub; ++sub) {
               sub.set_start_and_end(sub.get_start() + offset, sub.get_end() + offset);
            }
         }

         // Make the user life easy by selecting the first new subtitle
         if (first_new_subs) {
            doc->subtitles().select(first_new_subs);
         }

         doc->setFilename(ofile);
         doc->setFormat(oformat);
         doc->setCharset(ocharset);
         doc->finish_command();

         unsigned int subtitles_added = doc->subtitles().size() - subtitle_size;

         doc->flash_message(
            ngettext("One subtitle has been added to this document.", "%d subtitles have been added to this document.", subtitles_added),
            subtitles_added);
      } catch (...) {
         se_dbg_msg(SE_DBG_PLUGINS, "Failed to %s document: %s", applying_offset ? "join" : "append", uri.c_str());
      }

      return true;
   }

  protected:
   Gtk::UIManager::ui_merge_id ui_id;
   Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(JoinDocumentPlugin)
