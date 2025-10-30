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

#include <deque>
#include <list>
#include "command.h"

class Document;

class CommandGroup : public Command {
 public:
  explicit CommandGroup(const Glib::ustring &description);
  ~CommandGroup();

  void add(Command *cmd);

  void restore();
  void execute();

 protected:
  std::list<Command *> m_stack;
};

class CommandSystem {
 public:
  explicit CommandSystem(Document &doc);

  virtual ~CommandSystem();

  // return the description of the last undo command or NULL
  Glib::ustring get_undo_description();

  // return the description of the last redo command or NULL
  Glib::ustring get_redo_description();

  // Start recording
  void start(const Glib::ustring &description);

  // Add a new command
  void add(Command *cmd);

  // return true if it is recording. You can add your command if it's.
  bool is_recording();

  // Stop recording
  void finish();

  // Undo the last command
  void undo();

  // Redo the last undone commande
  void redo();

  // Clear all stack (undo/redo)
  void clear();

  // emit with undo/redo/start/finish
  sigc::signal<void> &signal_changed();

 protected:
  void clearRedo();

  void on_config_interface_changed(const Glib::ustring &name,
                                   const Glib::ustring &value);

 protected:
  Document &m_document;
  int m_max_undo_stack{10};
  bool m_is_recording{false};
  std::deque<Command *> m_undo_stack;
  std::deque<Command *> m_redo_stack;

  sigc::signal<void> m_signal_changed;
};
