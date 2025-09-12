// subtitleeditor -- a tool to create or edit subtitle
//
// https://kitone.github.io/subtitleeditor/
// https://github.com/kitone/subtitleeditor/
//
// Copyright @ 2005-2018, kitone
//
// - "cut, copy and paste - the 3 pillars of modern civilization"
// a subtitleeditor plugin by Eltomito <tomaspartl@centrum.cz>
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
#include <documents.h>
#include <extension/action.h>
#include <i18n.h>
#include <subtitleformatsystem.h>
#include <utility.h>
#include <memory>
#include "error.h"
#include "gtkmm_utility.h"
#include "player.h"
#include "subtitle.h"
#include "subtitleeditorwindow.h"
#include "subtitletime.h"
#include "subtitleview.h"
#include "widget_config_utility.h"

class ClipboardPlugin : public Action {
 public:
  bool is_configurable() {
    return false;
  }

  ClipboardPlugin() {
    se_dbg(SE_DBG_PLUGINS);

    target_default = "text/x-subtitles";
    target_text = "UTF8_STRING";

    clipdoc = NULL;
    activate();
    update_ui();
  }

  ~ClipboardPlugin() {
    se_dbg(SE_DBG_PLUGINS);

    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("ClipboardPlugin");

    action_group->add(
        Gtk::Action::create("clipboard-copy", _("_Copy"),
                            _("Copy selected subtitles to the clipboard.")),
        sigc::mem_fun(*this, &ClipboardPlugin::on_copy));
    action_group->add(
        Gtk::Action::create(
            "clipboard-cut", _("C_ut"),
            _("Copy selected subtitles to the clipboard and delete them.")),
        sigc::mem_fun(*this, &ClipboardPlugin::on_cut));
    action_group->add(
        Gtk::Action::create("clipboard-paste", _("_Paste"),
                            _("Paste subtitles from the clipboard AFTER the "
                              "currently selected subtitle.")),
        sigc::mem_fun(*this, &ClipboardPlugin::on_paste));
    action_group->add(
        Gtk::Action::create("clipboard-copy-with-timing", _("Copy With Timing"),
                            _("Copy selected subtitles and make their timing "
                              "visible to text-based applications.")),
        sigc::mem_fun(*this, &ClipboardPlugin::on_copy_with_timing));

    action_group->add(Gtk::Action::create("menu-edit/menu-paste-special",
                                          _("Paste Special")));

    action_group->add(
        Gtk::Action::create("clipboard-paste-at-player-position",
                            _("Paste At Current Player Position"),
                            _("Paste subtitles from the clipboard AFTER the "
                              "currently selected subtitle.")),
        sigc::mem_fun(*this, &ClipboardPlugin::on_paste_at_player_position));
    action_group->add(
        Gtk::Action::create("clipboard-paste-as-new-document",
                            _("Paste As New Document"),
                            _("Create a new document and paste the contents of "
                              "the clipboard into it.")),
        sigc::mem_fun(*this, &ClipboardPlugin::on_paste_as_new_document));
    action_group->add(
        Gtk::Action::create("clipboard-paste-over-text",
                            _("Paste Over Text"),
                            _("Overwrite subtitle text with clipboard content.")),
        sigc::mem_fun(*this, &ClipboardPlugin::on_paste_over_text));
    action_group->add(
        Gtk::Action::create("clipboard-paste-over-time",
                            _("Paste Over Time"),
                            _("Overwrite subtitle time with clipboard content.")),
        sigc::mem_fun(*this, &ClipboardPlugin::on_paste_over_time));
    action_group->add(
        Gtk::Action::create("clipboard-paste-unchanged",
                            _("Paste Unchanged"),
                            _("Paste without changing the time codes.")),
        sigc::mem_fun(*this, &ClipboardPlugin::on_paste_unchanged));

    action_group->add(
        Gtk::Action::create("clipboard-select-overlap",
                            _("Select Clipboard Overlap"),
                            _("Select subtitles that overlap clipboard content.")),
        sigc::mem_fun(*this, &ClipboardPlugin::on_select_overlap));

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui_id = ui->new_merge_id();

    ui->insert_action_group(action_group);

    Glib::ustring submenu_edit = R"(
      <ui>
        <menubar name='menubar'>
          <menu name='menu-edit' action='menu-edit'>
            <placeholder name='clipboard'>
              <separator/>
              <menuitem action='clipboard-copy'/>
              <menuitem action='clipboard-cut'/>
              <menuitem action='clipboard-paste'/>
              <separator/>
              <menuitem action='clipboard-copy-with-timing'/>
              <menu action='menu-edit/menu-paste-special'>
                <menuitem action='clipboard-paste-at-player-position'/>
                <menuitem action='clipboard-paste-as-new-document'/>
                <menuitem action='clipboard-paste-over-text'/>
                <menuitem action='clipboard-paste-over-time'/>
                <menuitem action='clipboard-paste-unchanged'/>
              </menu>
              <separator/>
            </placeholder>
          </menu>
        </menubar>
      </ui>
    )";

    ui_id = ui->add_ui_from_string(submenu_edit);

    Glib::ustring submenu_selection = R"(
      <ui>
        <menubar name='menubar'>
          <menu name='menu-selection' action='menu-selection'>
            <placeholder name='clipboard_select'>
              <menuitem action='clipboard-select-overlap'/>
            </placeholder>
          </menu>
        </menubar>
      </ui>
    )";

    ui_id = ui->add_ui_from_string(submenu_selection);
    // clear the clipdoc
    clear_clipdoc();

    // reset paste document
    pastedoc = NULL;

    // gtk clipboard
    target_instance = Glib::ustring::compose("subtitleeditor clipboard %1",
                                             (unsigned long)(this));

    my_targets.push_back(Gtk::TargetEntry(target_instance));
    my_targets.push_back(Gtk::TargetEntry(target_default));
    my_targets.push_back(Gtk::TargetEntry(target_text));
    chosen_clipboard_target = "";

    // connect clipboard signal
    se_dbg_msg(SE_DBG_PLUGINS, "Connecting to system clipboard.");
    connection_owner_change =
        Gtk::Clipboard::get()->signal_owner_change().connect(
            sigc::mem_fun(*this, &ClipboardPlugin::on_clipboard_owner_change));

    // initialize the clipboard targets
    update_paste_targets();

    // command shading
    connection_document_changed =
        se::documents::signal_active_changed().connect(
            sigc::mem_fun(*this, &ClipboardPlugin::on_active_document_changed));

    connection_player_message =
        get_subtitleeditor_window()->get_player()->signal_message().connect(
            sigc::mem_fun(*this, &ClipboardPlugin::on_player_message));

    on_active_document_changed(se::documents::active());
  }

  void deactivate() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    connection_owner_change.disconnect();
    connection_document_changed.disconnect();
    connection_player_message.disconnect();
    connection_selection_changed.disconnect();

    clear_clipdoc();
    clear_pastedoc();

    ui->remove_ui(ui_id);
    ui->remove_action_group(action_group);
  }

  void update_ui() {
    se_dbg(SE_DBG_PLUGINS);

    update_copy_and_cut_visibility();
    update_paste_visibility();
  }

 protected:
  void update_copy_and_cut_visibility() {
    se_dbg(SE_DBG_PLUGINS);

    Document *doc = get_current_document();

    // Is there subtitle selected ?
    bool visible = (doc) ? !doc->subtitles().get_selection().empty() : false;

    action_group->get_action("clipboard-copy")->set_sensitive(visible);
    action_group->get_action("clipboard-cut")->set_sensitive(visible);
    action_group->get_action("clipboard-copy-with-timing")
        ->set_sensitive(visible);
  }

  void update_paste_visibility() {
    se_dbg(SE_DBG_PLUGINS);

    bool paste_visible = false;
    bool paste_now_visible = false;
    bool paste_over_visible = false;

    if (chosen_clipboard_target != "") {
      paste_visible = true;
      paste_now_visible =
          (get_subtitleeditor_window()->get_player()->get_state() !=
           Player::NONE);
    }

		Document * doc = get_current_document();
		paste_over_visible = (doc) ? !doc->subtitles().get_selection().empty() : false;

    action_group->get_action("clipboard-paste")->set_sensitive(paste_visible);
    action_group->get_action("clipboard-paste-at-player-position")
        ->set_sensitive(paste_now_visible);
    action_group->get_action("clipboard-paste-as-new-document")
        ->set_sensitive(paste_visible);
    action_group->get_action("clipboard-paste-over-text")
        ->set_sensitive( paste_over_visible );
    action_group->get_action("clipboard-paste-over-time")
        ->set_sensitive( paste_over_visible );
    action_group->get_action("clipboard-paste-unchanged")
        ->set_sensitive( paste_visible );
    action_group->get_action("clipboard-select-overlap")
        ->set_sensitive( paste_visible );
  }

  void on_player_message(Player::Message) {
    update_paste_visibility();
  }

  void on_selection_changed() {
    se_dbg(SE_DBG_PLUGINS);

    update_paste_visibility();
    update_copy_and_cut_visibility();
  }

  void on_active_document_changed(Document *doc) {
    se_dbg(SE_DBG_PLUGINS);

    // We need to disconnect the old callback first
    if (connection_selection_changed)
      connection_selection_changed.disconnect();

    if (doc) {
      connection_selection_changed =
          doc->get_signal("subtitle-selection-changed")
              .connect(
                  sigc::mem_fun(*this, &ClipboardPlugin::on_selection_changed));
      // We need to force the update
      on_selection_changed();
    }
  }

  void on_clipboard_owner_change(GdkEventOwnerChange *) {
    se_dbg(SE_DBG_PLUGINS);

    update_paste_targets();
  }

  void update_paste_targets() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Gtk::Clipboard> refClipboard = Gtk::Clipboard::get();

    // Discover what targets are available:
    refClipboard->request_targets(
        sigc::mem_fun(*this, &ClipboardPlugin::on_clipboard_received_targets));
  }

  // Inspect targets available on the system clipboard
  // and decide which one, if any, is most useful to us.
  void on_clipboard_received_targets(
      const Glib::StringArrayHandle &targets_array) {
    se_dbg(SE_DBG_PLUGINS);

    // Get the list of available clipboard targets:
    std::vector<std::string> avail_targets = targets_array;

    // We reset and try to find the best target available
    // Then update the paste visiblity
    chosen_clipboard_target = Glib::ustring();

    for (const auto &target : my_targets) {
      if (std::find(avail_targets.begin(), avail_targets.end(),
                    target.get_target().c_str()) != avail_targets.end()) {
        chosen_clipboard_target = target.get_target();
        break;
      }
    }

    update_paste_visibility();

    se_dbg_msg(SE_DBG_PLUGINS, "The winning target is: '%s'.",
               chosen_clipboard_target.c_str());
  }

  // Somebody is asking for data we've copied to the clipboard. Let's give it to
  // them.
  void on_clipboard_get(Gtk::SelectionData &selection_data, guint /*info*/) {
    se_dbg(SE_DBG_PLUGINS);

    const Glib::ustring target = selection_data.get_target();
    Glib::ustring format;

    // what subtitle format should we provide the clipboard data in?
    if (target == target_default) {
      // paste data in the subtitle format of the clipboard document,
      // but avoid using the native format, because it sets the video, waveform,
      // keyframes, etc.
      format = clipdoc->getFormat();
      if (format == "Subtitle Editor Project") {
        format = "Advanced Subtitle Station Alpha";
      }
    } else if (target == target_text) {
      format = plaintext_format;
    } else {
      se_dbg_msg(SE_DBG_PLUGINS,
                 "Somebody asked for clipboard data in this strange "
                 "target format: '%s'.",
                 target.c_str());
      g_warning(
          "Subtitleeditor ClipboardPlugin::on_clipboard_get(): Unexpected "
          "clipboard target format.");
      return;
    }

    // "save" the clipdoc subtitles to the gtk clipboard
    try {
      se_dbg_msg(SE_DBG_PLUGINS, "Supplying clipboard data as '%s' format.",
                 format.c_str());

      Glib::ustring clipboard_data;
      SubtitleFormatSystem::instance().save_to_data(clipdoc, clipboard_data,
                                                    format);

      // The gtk documentation says that set() makes a copy of the data
      // provided, so we can feed it a local variable.
      selection_data.set(target, clipboard_data);

      se_dbg_msg(SE_DBG_PLUGINS, "%s", clipboard_data.c_str());
    } catch (const UnrecognizeFormatError &ex) {
      se_dbg_msg(SE_DBG_PLUGINS, "Failed to save clipboard subtitles as '%s'.",
                 format.c_str());
    }
  }

  void on_clipboard_clear() {
    se_dbg(SE_DBG_PLUGINS);

    clear_clipdoc();
  }

  // Tell gtk the system clipboard is mine now.
  // This must be called before actually storing the data,
  // because the documentation says Clipboard->set() may trigger a request to
  // clear the clipboard.
  void grab_system_clipboard() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Gtk::Clipboard> refClipboard = Gtk::Clipboard::get();

    // Targets:
    refClipboard->set(
        my_targets, sigc::mem_fun(*this, &ClipboardPlugin::on_clipboard_get),
        sigc::mem_fun(*this, &ClipboardPlugin::on_clipboard_clear));
  }

  // Was the data that's on the clipboard pasted by this instance of me?
  bool is_clipboard_mine() {
    return (chosen_clipboard_target == target_instance);
  }

  void request_clipboard_data() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Gtk::Clipboard> refClipboard = Gtk::Clipboard::get();

    refClipboard->request_contents(
        chosen_clipboard_target,
        sigc::mem_fun(*this, &ClipboardPlugin::on_clipboard_received));
  }

  // FIXME kitone-tomas: could you explain what going on this function ?
  // Specialy the last code, why we paste on the current document ?
  // FIXME tomas-kitone:
  // 1) First, we try to recognize and import the data on the clipboard as a
  // valid subtitle format. If it's the default target, we need to do it because
  // we don't know what format the subtitles are in. If it's the text target, we
  // do it in case somebody tried to paste e.g. subrip subtitles from a text
  // editor. (This is really useful, BTW. I am subtitling MTV Storytellers now
  // and they've sent me English subtitles as BTC inside a .doc. So since I have
  // this cool clipboard, all I need to do is copy from libreoffice and paste to
  // subtitleeditor :) 2) Originally, it pasted into the current document,
  // because the only case this would be wrong is if the user managed to
  // switch documents or close the current document between the time we ask Gtk
  // for clipboard data and the time we receive it. This might actually happen,
  // if the clipboard owner is very slow to provide paste data, so I now keep
  // the document which is current at the time the user selects paste in the
  // pastedoc variable and watch for the document being deleted using
  // connection_pastedoc_deleted.
  void on_clipboard_received(const Gtk::SelectionData &selection_data) {
    se_dbg(SE_DBG_PLUGINS);

    Document *doc = pastedoc;
    if (!doc) {
      // the document the user wanted to paste into was deleted before
      // we received clipboard data.
      return;
    }

    clear_pastedoc();
    clear_clipdoc(doc);

    // official examples say: info is meant to indicate the target, but it
    // seems to be always 0, so we use the selection_data's target instead.
    const Glib::ustring target = selection_data.get_target();

    // this is where we hold the received clipboard data
    Glib::ustring received_string;

    // try to recognize the clipboard data as a subtitle format
    if ((target == target_default) || (target == target_text)) {
      received_string = selection_data.get_data_as_string();
      try {
        se_dbg_msg(SE_DBG_PLUGINS, "Try to automatically recognize its format");
        // open file, automatically recognize its format
        // and read it into clipdoc
        SubtitleFormatSystem::instance().open_from_data(clipdoc,
                                                        received_string);
      } catch (...) {  // const UnrecognizeFormatError &ex)
        // import the data as plain text,
        // if the target we are receiving is text.
        if (target == target_text) {
          se_dbg_msg(SE_DBG_PLUGINS,
                     "Read the content of clipboard as Plain Text Format");

          SubtitleFormatSystem::instance().open_from_data(
              clipdoc, received_string, "Plain Text Format");
          // FIXME tomas: Now, we should invoke Minimize Duration and Stack
          // Subtitles on the entire clipdoc
        } else {  // target == target_text
          se_dbg_msg(
              SE_DBG_PLUGINS,
              "Failed to recognize the default target as a subtitle format!");
          return;
        }
      }
    } else {  //( target == target_default )||( target == target_text )
      se_dbg_msg(SE_DBG_PLUGINS,
                 "Somebody is sending us data as this strange target: '%s'.",
                 target.c_str());
      g_warning(
          "Subtitleeditor ClipboardPlugin::on_clipboard_received(): "
          "Unexpected "
          "clipboard target format.");
      return;
    }

    if(( paste_flags & SELECT_OVERLAP) != 0 ) { //clipboard used for selection only
      select_overlap( doc );
      doc->emit_signal("subtitle-time-changed");
    } else { //clipboard will be pasted
      // actually paste the data from clipdoc to the current document
      doc->start_command(_("Paste"));
      paste(doc, paste_flags);
      doc->emit_signal("subtitle-time-changed");
      doc->finish_command();
    }
  }

  // Clear clipboard document by first destroying it
  // and optionally recreating it as en empty copy of the supplied document.
  // If you don't want a new clipdoc created, supply doc = NULL.
  bool clear_clipdoc(Document *doc = nullptr) {
    se_dbg(SE_DBG_PLUGINS);

    if (clipdoc != NULL) {
      delete clipdoc;
      clipdoc = NULL;
    }

    if (doc != NULL) {
      clipdoc = new Document(*doc, false);
      if (clipdoc == 0) {
        se_dbg_msg(SE_DBG_PLUGINS, "Failed to create the clipboard document.");
        return false;
      }
    }
    return true;
  }

  enum CopyFlags {
    COPY_IS_CUT = 0x01,      // this copy is actually a cut
    COPY_WITH_TIMING = 0x02  // text target provides timed subtitles
  };

  bool copy_to_clipdoc(Document *doc, unsigned long flags = 0) {
    se_dbg(SE_DBG_PLUGINS);

    // is there anything to copy?
    std::vector<Subtitle> selection = doc->subtitles().get_selection();
    if (selection.size() < 1) {
      doc->flash_message(
          _("Nothing to copy. Select at least one subtitle first."));
      return false;
    }

    grab_system_clipboard();

    clear_clipdoc(doc);

    Subtitles clipsubs = clipdoc->subtitles();
    unsigned long i = 0;
    while (i < selection.size()) {
      Subtitle sub = clipsubs.append();
      selection[i].copy_to(sub);
      i++;
    }

    // format for the plain-text clipboard target
    if (flags & COPY_WITH_TIMING)
      plaintext_format = clipdoc->getFormat();
    else
      plaintext_format = "Plain Text Format";

    // copy X cut
    if (flags & COPY_IS_CUT)
      doc->subtitles().remove(selection);

    return true;
  }

  void on_copy() {
    se_dbg(SE_DBG_PLUGINS);

    Document *doc = get_current_document();
    g_return_if_fail(doc);

    copy_to_clipdoc(doc, 0);
  }

  void on_copy_with_timing() {
    se_dbg(SE_DBG_PLUGINS);

    Document *doc = get_current_document();
    g_return_if_fail(doc);

    copy_to_clipdoc(doc, COPY_WITH_TIMING);
  }

  void on_cut() {
    se_dbg(SE_DBG_PLUGINS);

    Document *doc = get_current_document();
    g_return_if_fail(doc);

    doc->start_command(_("Cut"));

    copy_to_clipdoc(doc, COPY_IS_CUT);

    doc->emit_signal("subtitle-time-changed");
    doc->finish_command();
  }

  // FIXME tomas-kitone: I kept the flags for the new Paste At Player Position
  // and Paste As New Document commands.
  void paste(Document *doc, unsigned long flags = 0) {
    se_dbg(SE_DBG_PLUGINS);
//XXX
    Subtitles subtitles = doc->subtitles();
    std::vector<Subtitle> new_subtitles;
    Subtitle paste_after;

    // Make sure there is something to paste
    if (is_something_to_paste() == false)
      return;

    if( (flags & (PASTE_OVER_TEXT|PASTE_OVER_TIME) ) != 0 )
    {
      new_subtitles = subtitles.get_selection();
      int howmany = std::min( (int)new_subtitles.size(), (int)clipdoc->subtitles().size() );
      Subtitle clip_sub = clipdoc->subtitles().get_first();
      Subtitle oversub = subtitles.get_first();

      for( int i = 0; i < howmany; i++ )
      {
        //overwrite
        if( ( flags & PASTE_OVER_TEXT ) != 0 ) {
          new_subtitles[i].set_text( clip_sub.get_text() );
        }
        if( ( flags & PASTE_OVER_TIME ) != 0 ) {
          new_subtitles[i].set_start_and_end( clip_sub.get_start(), clip_sub.get_end() );
        }
        ++clip_sub;
      }

      //tell the user what happened
      doc->flash_message(_("%i subtitle(s) overwritten."), howmany );
    } else { //don't PASTE_OVER_TEXT
      paste_after = where_to_paste(subtitles);
  
      // We get the new subtitles in the new_subtitles array
      create_and_insert_paste_subtitles(subtitles, paste_after, new_subtitles);

			if( (flags & PASTE_UNCHANGED) == 0 ) {
        calculate_and_apply_timeshift(subtitles, paste_after, new_subtitles, flags);
      }
  
      // We can now remove the old selected subtitles, only if the selection is >
      // 1
      std::vector<Subtitle> selection = subtitles.get_selection();
      if (selection.size() > 1)
        subtitles.remove(selection);
  
      // We select the pasted subtitles, this way the user see where are the new
      // subtitles
      subtitles.unselect_all();
      subtitles.select(new_subtitles);
  
      // show the pasted subtitles
      // FIXME tomas-kitone: this is a clumsy implementation.
      // I think we should add a show_subtitle( Subtitle &sub ) function to class
      // SubtitleView or at least get_iter() or get_path() to class Subtitle
      SubtitleView *view = reinterpret_cast<SubtitleView *>(doc->widget());
      if (view != NULL) {
        int sub_num = new_subtitles[0].get_num() - 1;
        Gtk::TreeModel::Path sub_path =
            Gtk::TreeModel::Path(Glib::ustring::compose("%1", sub_num));
        view->scroll_to_row(sub_path, 0.25);
      }
  
      // tell the user what happened
      doc->flash_message(_("%i subtitle(s) pasted."), new_subtitles.size());
    }
  }

  bool is_something_to_paste() {
    if (clipdoc == NULL) {
      se_dbg_msg(
          SE_DBG_PLUGINS,
          "No clipboard document, nothing to paste. How come I was called?");
      return false;
    } else if (clipdoc->subtitles().size() == 0) {
      se_dbg_msg(
          SE_DBG_PLUGINS,
          "No subtitles in the clipboard document - how come I was called?");
      return false;
    }
    return true;
  }

  // The pasted subtitles will be added after the returned subtitle.
  // The returned subtitle can be invalid, if the document is empty or if there
  // are no subtitle selected
  Subtitle where_to_paste(Subtitles &subtitles) {
    Subtitle paste_after;

    std::vector<Subtitle> selection = subtitles.get_selection();

    if (selection.empty())
      return Subtitle();
    else
      return selection[0];
  }

  void create_and_insert_paste_subtitles(Subtitles &subtitles,
                                         Subtitle &paste_after,
                                         std::vector<Subtitle> &new_subtitles) {
    // We can reserve the size of the array new_subtitles because we already
    // know the number of new subtitles
    new_subtitles.reserve(clipdoc->subtitles().size());

    Subtitle after = paste_after;

    for (Subtitle clip_sub = clipdoc->subtitles().get_first(); clip_sub;
         ++clip_sub) {
      Subtitle new_sub =
          (after) ? subtitles.insert_after(after) : subtitles.append();

      clip_sub.copy_to(new_sub);

      new_subtitles.push_back(new_sub);

      after = new_sub;
    }
  }

  void calculate_and_apply_timeshift(Subtitles &subtitles,
                                     Subtitle &paste_after,
                                     std::vector<Subtitle> &new_subtitles,
                                     unsigned long flags) {
    SubtitleTime timeshift;

    if (flags & PASTE_TIMING_AFTER) {
      // We use the old selection to know how-to apply the time shift
      guint selection_size = subtitles.get_selection().size();

      if (selection_size == 0) {
        // There're no subtitles, we want to keep the original subtitles times
        // No needs for use to continue
        return;
      } else if (selection_size == 1) {
        // One subtitle select, new subtitles goes just after the selected we
        // gap respect
        SubtitleTime gap = cfg::get_int("timing", "min-gap-between-subtitles");

        timeshift = paste_after.get_end() + gap - new_subtitles[0].get_start();
      } else {  // selection_size > 1
        // We will replace the selected subtitles
        // so we need to start at the same time of the first selected subtitle
        timeshift = paste_after.get_start() - new_subtitles[0].get_start();
      }
    } else if (flags & PASTE_TIMING_PLAYER) {
      SubtitleTime player_pos =
          get_subtitleeditor_window()->get_player()->get_position();
      timeshift = player_pos - new_subtitles[0].get_start();
    } else {  // no time shift
      return;
    }

    // aply the time shift
    for (auto &sub : new_subtitles) {
      sub.set_start_and_end(sub.get_start() + timeshift,
                            sub.get_end() + timeshift);
    }
  }

  // ================= SELECT OVERLAP COMMAND =====================

  void on_select_overlap()
 {
    se_dbg(SE_DBG_PLUGINS);

    Document *doc = get_current_document();
    g_return_if_fail(doc);

    if (is_clipboard_mine()) {
      select_overlap( doc );
    } else {
      // remember what document to select subs in...
      set_pastedoc(doc);
      // ...and that we are just selecting, not pasting anything.
      paste_flags = SELECT_OVERLAP;
      request_clipboard_data();
    }
  }

  void select_overlap( Document *doc )
  {
    g_return_if_fail(doc);

    //are there any subtitled in the clipboard document?
    if (is_something_to_paste() == false) {
      doc->flash_message(_("No subtitles on the clipboard."));
      return;
    }

    std::vector<Subtitle> selection;
    Subtitles subtitles = doc->subtitles();

    Subtitle clipsub = clipdoc->subtitles().get_first();
    while( clipsub ) {
      add_overlaps( selection, clipsub, subtitles );
      clipsub = clipdoc->subtitles().get_next( clipsub );
    }
    subtitles.unselect_all();
    subtitles.select( selection );
    doc->flash_message(_("Selected %i subtitles."), selection.size() );
  }

  bool subs_overlap( const Subtitle &a, const Subtitle &b ) {
    long astart = a.get_start().totalmsecs;
    long aend = a.get_end().totalmsecs;
    long bstart = b.get_start().totalmsecs;
    long bend = b.get_end().totalmsecs;
    if(( astart < bend )&&( bstart < aend )) {
      return true;
    }
    return false;
  };

  void add_overlaps( std::vector<Subtitle> &dst, const Subtitle &selsub, Subtitles &subs ) {
    Subtitle cursub = subs.get_first();
    while( cursub ) {
      if( subs_overlap( cursub, selsub ) ) {
        dst.push_back( cursub );
      }
      cursub = subs.get_next( cursub );
    }
  }

  // ================= PASTE COMMANDS =====================

  void on_paste() {
    se_dbg(SE_DBG_PLUGINS);

    paste_common(PASTE_TIMING_AFTER);
  }

  void on_paste_at_player_position() {
    se_dbg(SE_DBG_PLUGINS);

    paste_common(PASTE_TIMING_PLAYER);
  }

  void on_paste_as_new_document() {
    se_dbg(SE_DBG_PLUGINS);

    paste_common(PASTE_AS_NEW_DOCUMENT);
  }

  void on_paste_over_text()
  {
    se_dbg(SE_DBG_PLUGINS);

    paste_common( PASTE_OVER_TEXT );
  };

  void on_paste_over_time()
  {
    se_dbg(SE_DBG_PLUGINS);

    paste_common( PASTE_OVER_TIME );
  };

  void on_paste_unchanged()
  {
    se_dbg(SE_DBG_PLUGINS);

    paste_common( PASTE_UNCHANGED );
  };

  void paste_common(unsigned long flags) {
    se_dbg(SE_DBG_PLUGINS);

    // what document are we pasting into?
    Document *doc = get_current_document();
    if (!doc || flags & PASTE_AS_NEW_DOCUMENT) {
      // open a new document
      doc = new Document();
      g_return_if_fail(doc);

      doc->setFilename(se::documents::generate_untitled_name());
      se::documents::append(doc);
    }

    if (is_clipboard_mine()) {
      doc->start_command(_("Paste"));
      paste(doc, flags);
      doc->emit_signal("subtitle-time-changed");
      doc->finish_command();
    } else {
      // remember what document to paste into
      set_pastedoc(doc);
      // and how
      paste_flags = flags;

      request_clipboard_data();
    }
  }

  void set_pastedoc(Document *doc) {
    se_dbg(SE_DBG_PLUGINS);

    pastedoc = doc;

    // if the user deletes the document we are planning to paste into
    // before we get the data from the gtk clipboard, we need to know about it.
    if (connection_pastedoc_deleted)
      connection_pastedoc_deleted.disconnect();

    connection_pastedoc_deleted = se::documents::signal_deleted().connect(
        sigc::mem_fun(*this, &ClipboardPlugin::on_pastedoc_deleted));
  }

  void on_pastedoc_deleted(Document *doc) {
    se_dbg(SE_DBG_PLUGINS);

    if (doc == pastedoc)
      clear_pastedoc();
  }

  void clear_pastedoc() {
    se_dbg(SE_DBG_PLUGINS);

    pastedoc = NULL;

    if (connection_pastedoc_deleted)
      connection_pastedoc_deleted.disconnect();
  }

 protected:
  Gtk::UIManager::ui_merge_id ui_id;
  Glib::RefPtr<Gtk::ActionGroup> action_group;

  // data store
  Document *clipdoc;

  // This is the name of the subtitle format we supply,
  // when plain-text data is requested.
  // Usually, it's "UTF8_STRING" but when the user selects "Copy With Timing",
  // it can be anything else, e.g., "SubRip", "Spruce STL" or whatever.
  Glib::ustring plaintext_format;

  // where to paste
  Document *pastedoc;

  // how to paste
  enum PasteFlags {
    PASTE_TIMING_AFTER =
        0x01,  // snap the pasted subtitles after the preceding subtitle
    PASTE_TIMING_PLAYER = 0x02,  // paste at the current player position
    PASTE_AS_NEW_DOCUMENT = 0x04,
    PASTE_UNCHANGED = 0x08,  // don't change the time codes
    PASTE_OVER_TEXT = 0x10,  // keep current timing but overwrite the text
    PASTE_OVER_TIME = 0x20,  // keep current text but overwrite the timing
    SELECT_OVERLAP = 0x40  // select clipboard overlap, no pasting
  };
  unsigned long paste_flags;

  // the best clipboard target available at the moment
  Glib::ustring chosen_clipboard_target;

  // target definitions
  Glib::ustring target_instance;
  Glib::ustring target_default;
  Glib::ustring target_text;

  // provided targets
  std::vector<Gtk::TargetEntry> my_targets;

  // connections

  // connected in activate(), disconnected in deactivate()
  sigc::connection connection_owner_change;
  sigc::connection connection_document_changed;
  sigc::connection connection_player_message;
  // connected in on_document_changed(), disconnected in on_document_changed()
  // and deactivate()
  sigc::connection connection_selection_changed;
  // connected in set_pastedoc(), disconnected in set_pastedoc() and
  // clear_pastedoc()
  sigc::connection connection_pastedoc_deleted;
};

REGISTER_EXTENSION(ClipboardPlugin)
