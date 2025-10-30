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

#include "player.h"

Player::Player() {
}

Player::~Player() {
}

// Sets the current state of the pipeline.
// Block or unblock the timeout signal and emit the signal state_changed.
void Player::set_player_state(Player::State state) {
  m_player_state = state;

  // create the timeout callback
  // the signal is directly blocked
  if (!m_timeout_connection) {
    unsigned int msec = 100;
    m_timeout_connection = Glib::signal_timeout().connect(
        sigc::mem_fun(*this, &Player::on_timeout), msec);
    m_timeout_connection.block();
  }

  switch (state) {
    case NONE:
    case PAUSED: {
      // Update with the last position, block the signal and
      // send a player state message
      got_tick();
      m_timeout_connection.block();
      send_message((state == NONE) ? STATE_NONE : STATE_PAUSED);
    } break;
    case PLAYING: {
      m_timeout_connection.unblock();
      got_tick();
      send_message(STATE_PLAYING);
    } break;
  }
}

void Player::got_tick() {
  long current_time = get_position();
  long stream_length = get_duration();
  double current_position = 0.0;
  if (stream_length != 0)
    current_position = static_cast<double>(current_time) / stream_length;

  m_signal_tick(current_time, stream_length, current_position);
}

bool Player::on_timeout() {
  got_tick();

  return is_playing();
}

// Callback used by the player to send message to the application
// like the change of the state of the player or change on the stream...
sigc::signal<void, Player::Message>& Player::signal_message() {
  return m_signal_message;
}

void Player::send_message(Player::Message msg) {
  m_signal_message(msg);
}

// void my_tick(long current_time, long stream_length, double current_position)
// current_time: position in the stream in milliseconds
// stream_length: length of the stream in milliseconds
// current_position: position in the stream as a percentage betwwen 0 and 1 (%)
// Emitted every time event happens or at regular intervals during playing
// state.
sigc::signal<void, long, long, double>& Player::signal_tick() {
  return m_signal_tick;
}

// Return the state of the player.
// NONE can be considerate as NULL, the pipeline is not create.
Player::State Player::get_state() {
  return m_player_state;
}

void Player::set_keyframes(Glib::RefPtr<KeyFrames> keyframes) {
  m_keyframes = keyframes;
  send_message(KEYFRAME_CHANGED);
}

Glib::RefPtr<KeyFrames> Player::get_keyframes() {
  return m_keyframes;
}
