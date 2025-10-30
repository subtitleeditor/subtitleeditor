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
#include "gstplayer.h"
#include "subtitleeditorwindow.h"
#include "utility.h"
#include "videoplayer.h"

// Player Controls Widgets
class PlayerControls : public Gtk::HBox {
 public:
  PlayerControls(BaseObjectType* cobject,
                 const Glib::RefPtr<Gtk::Builder>& builder)
      : Gtk::HBox(cobject) {
    m_current_seek = false;
    m_player = NULL;

    builder->get_widget("button-play", m_button_play);
    builder->get_widget("button-pause", m_button_pause);

    builder->get_widget("hscale-seek", m_hscale_seek);

    m_hscale_seek->set_range(0, 1);
    m_hscale_seek->set_value(0);
    m_hscale_seek->set_increments(
        0.05, 0.05);  // get_adjustment()->set_step_increment(0.05);
    // m_hscale_seek->set_update_policy(Gtk::UPDATE_CONTINUOUS);
    m_hscale_seek->set_value_pos(Gtk::POS_RIGHT);

    m_hscale_seek->signal_format_value().connect(
        sigc::mem_fun(*this, &PlayerControls::position_to_string));

    m_connection_scale_changed = m_hscale_seek->signal_value_changed().connect(
        sigc::mem_fun(*this, &PlayerControls::on_seek_value_changed));

    m_hscale_seek->signal_event().connect(
        sigc::mem_fun(*this, &PlayerControls::on_seek_event), true);

    // signal play/pause directly connect to the player
    m_button_play->signal_clicked().connect(
        sigc::mem_fun(*this, &PlayerControls::on_play));

    m_button_pause->signal_clicked().connect(
        sigc::mem_fun(*this, &PlayerControls::on_pause));

    show_all();

    m_button_pause->hide();

    set_sensitive(false);
  }

  void on_seek_value_changed() {
    if (m_current_seek)
      return;
    // convert pos (as percentage) to position in the stream
    double pos = m_hscale_seek->get_value();
    m_player->seek(long(m_player->get_duration() * pos));
  }

  void on_play() {
    if (m_player->is_playing() == false) {
      m_player->seek(m_player->get_position());
      m_player->play();
    }
  }

  void on_pause() {
    m_player->pause();
  }

  void init_with_player(Player* player) {
    m_player = player;

    m_player->signal_message().connect(
        sigc::mem_fun(*this, &PlayerControls::on_player_message));

    m_player->signal_tick().connect(
        sigc::mem_fun(*this, &PlayerControls::on_player_tick));
  }

  void on_player_message(Player::Message msg) {
    switch (msg) {
      case Player::STATE_NONE: {
        m_hscale_seek->set_value(0);
        m_button_play->show();
        m_button_pause->hide();
        set_sensitive(false);
      } break;
      case Player::STATE_PAUSED: {
        m_button_play->show();
        m_button_pause->hide();
        set_sensitive(true);
      } break;
      case Player::STATE_PLAYING: {
        m_button_play->hide();
        m_button_pause->show();
        set_sensitive(true);
      } break;
      case Player::STREAM_DURATION_CHANGED:
      case Player::STREAM_READY: {
        long pos = m_player->get_position();
        long dur = m_player->get_duration();
        double perc = 0;
        if (dur != 0) {
          perc = static_cast<double>(pos) / dur;
        }
        set_seek_position(perc);
      } break;
      default:
        break;
    }
  }

  void on_player_tick(long /*current_time*/, long /*stream_length*/,
                      double current_position) {
    m_connection_scale_changed.block();
    set_seek_position(current_position);
    m_connection_scale_changed.unblock();
  }

  void set_seek_position(double position) {
    if (m_current_seek == false && m_hscale_seek->get_value() != position) {
      m_hscale_seek->set_value(position);
      m_hscale_seek->queue_draw();
    }
  }

 protected:
  Gtk::Button* create_button(Gtk::StockID stock) {
    Gtk::Button* button = manage(new Gtk::Button);
    button->set_relief(Gtk::RELIEF_NONE);

    Gtk::Image* image = manage(new Gtk::Image(stock, Gtk::ICON_SIZE_MENU));

    button->add(*image);
    return button;
  }

  Glib::ustring position_to_string(double value) {
    // convert pos (as percentage) to position in the stream
    return SubtitleTime(long(m_player->get_duration() * value)).str();
  }

  bool on_seek_event(GdkEvent* ev) {
    if (ev->type == GDK_BUTTON_PRESS) {
      m_current_seek = true;
    } else if (ev->type == GDK_BUTTON_RELEASE) {
      // convert pos (as percentage) to position in the stream
      double pos = m_hscale_seek->get_value();
      m_player->seek(long(m_player->get_duration() * pos));

      m_current_seek = false;
    }
    return false;
  }

 protected:
  // controls
  Gtk::Button* m_button_play;
  Gtk::Button* m_button_pause;
  Gtk::Scale* m_hscale_seek;
  bool m_current_seek;
  sigc::connection m_connection_scale_changed;
  Player* m_player;
};

// Constructor
// Create the GStreamer Player, the PlayerControls (play/pause + seek)
VideoPlayer::VideoPlayer(BaseObjectType* cobject,
                         const Glib::RefPtr<Gtk::Builder>& builder)
    : Gtk::VBox(cobject) {
  m_cfg_display_translated_subtitle = false;

  m_player = manage(new GstPlayer);

  Gtk::Frame* m_framePlayer = NULL;
  PlayerControls* m_playerControls = NULL;

  builder->get_widget("frame-player", m_framePlayer);
  builder->get_widget_derived("player-controls", m_playerControls);

  m_framePlayer->add(*dynamic_cast<Gtk::Widget*>(m_player));
  m_playerControls->init_with_player(m_player);

  load_config();

  cfg::signal_changed("video-player")
      .connect(
          sigc::mem_fun(*this, &VideoPlayer::on_config_video_player_changed));

  se::documents::signal_active_changed().connect(
      sigc::mem_fun(*this, &VideoPlayer::on_active_document_changed));

  m_player->signal_tick().connect(
      sigc::mem_fun(*this, &VideoPlayer::on_player_tick));

  m_player->signal_message().connect(
      sigc::mem_fun(*this, &VideoPlayer::on_player_message));
}

// Destructor
VideoPlayer::~VideoPlayer() {
  se_dbg(SE_DBG_VIDEO_PLAYER);
}

// Load the video player config.
void VideoPlayer::load_config() {
  if (cfg::get_boolean("video-player", "display")) {
    show();
  } else {
    hide();
  }
  m_player->set_repeat(cfg::get_boolean("video-player", "repeat"));

  m_cfg_display_translated_subtitle =
      cfg::get_boolean("video-player", "display-translated-subtitle");
}

// Return the gstreamer player.
Player* VideoPlayer::player() {
  return m_player;
}

// The player state has changed.
// Clear subtitle.
void VideoPlayer::on_player_message(Player::Message msg) {
  switch (msg) {
    case Player::STATE_NONE:
    case Player::STATE_PAUSED:
    case Player::STATE_PLAYING:
      clear_subtitle();
      break;
    default:
      break;
  }
}

// The config of video player has changed.
void VideoPlayer::on_config_video_player_changed(const Glib::ustring& key,
                                                 const Glib::ustring& value) {
  if (key == "display") {
    bool state = utility::string_to_bool(value);

    if (state)
      show();
    else
      hide();
  } else if (key == "display-translated-subtitle") {
    m_cfg_display_translated_subtitle = utility::string_to_bool(value);
  }
}

// The current document has changed.
// Clear subtitle (sub and player text) and try to found the good subtitle.
void VideoPlayer::on_active_document_changed(Document* doc) {
  m_connection_document_changed.disconnect();
  if (doc != NULL)
    m_connection_document_changed =
        doc->get_signal("document-changed")
            .connect(sigc::mem_fun(*this, &VideoPlayer::clear_subtitle));

  clear_subtitle();
  find_subtitle();
}

// Check or search the good subtitle (find_subtitle).
void VideoPlayer::on_player_tick(long /*current_time*/, long /*stream_length*/,
                                 double /*current_position*/) {
  find_subtitle();
}

// Initialize the current subtitle and the player text to NULL.
void VideoPlayer::clear_subtitle() {
  m_subtitle = Subtitle();

  m_player->set_subtitle_text("");
}

// Check if time is in subtitle.
bool VideoPlayer::is_good_subtitle(const Subtitle& sub, long time) {
  if (sub)
    if (time >= sub.get_start().totalmsecs && time < sub.get_end().totalmsecs)
      return true;

  return false;
}

// Try to found the good subtitle and init the player (text).
// FIXME: this's call each time
bool VideoPlayer::find_subtitle() {
  Document* doc = SubtitleEditorWindow::get_instance()->get_current_document();
  if (doc == NULL) {
    clear_subtitle();
    return false;
  }

  long position = m_player->get_position();

  SubtitleTime time(position);

  if (!m_subtitle) {
    m_subtitle = doc->subtitles().find(time);
    if (m_subtitle)
      show_subtitle_text();
    else
      show_subtitle_null();
  } else {
    if (is_good_subtitle(m_subtitle, position)) {  // is good ?
      show_subtitle_text();
    } else if (time < m_subtitle.get_start()) {  // is the next ?
      show_subtitle_null();
    } else if (time >= m_subtitle.get_end()) {  // it's the old...
      // ... try with the next subtitle
      show_subtitle_null();

      Subtitle next = doc->subtitles().get_next(m_subtitle);
      if (next) {
        m_subtitle = next;
        if (is_good_subtitle(m_subtitle, position))
          show_subtitle_text();
      } else {
        m_subtitle = Subtitle();
      }
    }
  }
  return true;
}

// Sets the text of the player to NULL.
void VideoPlayer::show_subtitle_null() {
  m_player->set_subtitle_text("");
}

// Sets the text of the player with the current subtitle.
void VideoPlayer::show_subtitle_text() {
  if (!m_subtitle)
    return;

  Glib::ustring text;

  if (m_cfg_display_translated_subtitle &&
      !m_subtitle.get_translation().empty())
    text = m_subtitle.get_translation();
  else
    text = m_subtitle.get_text();

  m_player->set_subtitle_text(text);
}
