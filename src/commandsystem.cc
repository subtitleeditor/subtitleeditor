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

#include "commandsystem.h"

#include "cfg.h"
#include "document.h"
#include "utility.h"

class SubtitleSelectionCommand : public Command {
  public:
   explicit SubtitleSelectionCommand(Document* doc) : Command(doc, _("Subtitle Selection")) {
      std::vector<Gtk::TreeModel::Path> rows = get_document_subtitle_view()->get_selection()->get_selected_rows();

      m_paths.resize(rows.size());

      for (unsigned int i = 0; i < rows.size(); ++i) m_paths[i] = rows[i].to_string();
   }

   void execute() {
      Glib::RefPtr<Gtk::TreeSelection> selection = get_document_subtitle_view()->get_selection();

      selection->unselect_all();
      for (unsigned int i = 0; i < m_paths.size(); ++i) selection->select(Gtk::TreePath(m_paths[i]));
   }

   void restore() {
      Glib::RefPtr<Gtk::TreeSelection> selection = get_document_subtitle_view()->get_selection();

      selection->unselect_all();
      for (unsigned int i = 0; i < m_paths.size(); ++i) selection->select(Gtk::TreePath(m_paths[i]));
   }

  protected:
   std::vector<Glib::ustring> m_paths;
};

CommandGroup::CommandGroup(const Glib::ustring& description) : Command(NULL, description) {
   se_dbg_msg(SE_DBG_COMMAND, "description=%s", description.c_str());
}

CommandGroup::~CommandGroup() {
   se_dbg(SE_DBG_COMMAND);

   while (!m_stack.empty()) {
      delete m_stack.back();
      m_stack.pop_back();
   }
}

void CommandGroup::add(Command* cmd) {
   se_dbg(SE_DBG_COMMAND);

   m_stack.push_back(cmd);
}

void CommandGroup::execute() {
   se_dbg(SE_DBG_COMMAND);

   for (const auto& cmd : m_stack) {
      cmd->execute();
   }
}

void CommandGroup::restore() {
   se_dbg(SE_DBG_COMMAND);

   for (auto it = m_stack.rbegin(); it != m_stack.rend(); ++it) {
      (*it)->restore();
   }
}

// Constructor
// get the maximum stack to config
CommandSystem::CommandSystem(Document& doc) : m_document(doc) {
   m_max_undo_stack = cfg::get_int("interface", "max-undo");

   cfg::signal_changed("interface").connect(sigc::mem_fun(*this, &CommandSystem::on_config_interface_changed));
}

CommandSystem::~CommandSystem() {
   clear();
}

void CommandSystem::on_config_interface_changed(const Glib::ustring& key, const Glib::ustring& value) {
   if (key == "max-undo") {
      int max = utility::string_to_int(value);

      m_max_undo_stack = max;
   }
}

// efface les piles undo/redo
void CommandSystem::clear() {
   // on efface la pile undo
   while (!m_undo_stack.empty()) {
      Command* cmd = m_undo_stack.back();

      m_undo_stack.pop_back();

      delete cmd;
   }

   // on efface la pile redo
   clearRedo();
}

void CommandSystem::clearRedo() {
   while (!m_redo_stack.empty()) {
      Command* cmd = m_redo_stack.back();

      m_redo_stack.pop_back();

      delete cmd;
   }
}

void CommandSystem::add(Command* cmd) {
   g_return_if_fail(cmd);

   clearRedo();

   if (m_is_recording) {
      Command* last = m_undo_stack.back();
      CommandGroup* group = dynamic_cast<CommandGroup*>(last);

      g_return_if_fail(group);

      group->add(cmd);
   } else {
      m_undo_stack.push_back(cmd);
   }

   if (m_max_undo_stack != 0) {
      while (m_undo_stack.size() > m_max_undo_stack) {
         Command* tmp = m_undo_stack.front();
         m_undo_stack.pop_front();
         delete tmp;
      }
   }
}

void CommandSystem::undo() {
   if (m_undo_stack.empty())
      return;

   Command* cmd = m_undo_stack.back();

   m_undo_stack.pop_back();

   // m_document.flash_message(_("Undo: %s"), cmd->description().c_str());
   cmd->restore();

   m_redo_stack.push_back(cmd);

   m_signal_changed();
}

void CommandSystem::redo() {
   if (m_redo_stack.empty())
      return;

   Command* cmd = m_redo_stack.back();

   m_redo_stack.pop_back();

   // m_document.flash_message(_("Redo: %s"), cmd->description().c_str());
   cmd->execute();

   m_undo_stack.push_back(cmd);

   m_signal_changed();
}

void CommandSystem::start(const Glib::ustring& description) {
   m_is_recording = true;

   m_undo_stack.push_back(new CommandGroup(description));

   add(new SubtitleSelectionCommand(&m_document));

   m_signal_changed();
}

bool CommandSystem::is_recording() {
   return m_is_recording;
}

void CommandSystem::finish() {
   if (m_is_recording)
      add(new SubtitleSelectionCommand(&m_document));

   m_is_recording = false;

   m_signal_changed();
}

Glib::ustring CommandSystem::get_undo_description() {
   if (!m_undo_stack.empty()) {
      return m_undo_stack.back()->description();
   }
   return Glib::ustring();
}

Glib::ustring CommandSystem::get_redo_description() {
   if (!m_redo_stack.empty()) {
      return m_redo_stack.back()->description();
   }
   return Glib::ustring();
}

sigc::signal<void>& CommandSystem::signal_changed() {
   return m_signal_changed;
}
