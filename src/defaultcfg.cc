// subtitleeditor -- a tool to create or edit subtitle
//
// https://kitone.github.io/subtitleeditor/
// https://github.com/kitone/subtitleeditor/
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

#include <glibmm/ustring.h>
#include <map>

void get_default_config(
    std::map<Glib::ustring, std::map<Glib::ustring, Glib::ustring> > &config) {
  // [general]

  // [document]
  config["document"]["format"] = "SubRip";
  config["document"]["newline"] = "Unix";

  // [video-player]
  config["video-player"]["display-translated-subtitle"] = "false";
  config["video-player"]["shaded-background"] = "true";
  config["video-player"]["font-desc"] = "Sans 26";
  config["video-player"]["force-aspect-ratio"] = "true";
  config["video-player"]["brightness"] = "0";
  config["video-player"]["contrast"] = "0";
  config["video-player"]["saturation"] = "0";
  config["video-player"]["hue"] = "0";
  config["video-player"]["skip-tiny"] = "500";
  config["video-player"]["skip-very-short"] = "3";
  config["video-player"]["skip-short"] = "10";
  config["video-player"]["skip-medium"] = "60";
  config["video-player"]["skip-long"] = "300";
  config["video-player"]["audio-sink"] = DEFAULT_PLAYER_AUDIO_SINK;
  config["video-player"]["video-sink"] = DEFAULT_PLAYER_VIDEO_SINK;
  config["video-player"]["timeout"] = "100";
  config["video-player"]["repeat"] = "false";
  config["video-player"]["display"] = "true";
  config["video-player"]["automatically-open-video"] = "true";

  // [waveform]
  config["waveform"]["zoom"] = "1";
  config["waveform"]["scale"] = "1";
  config["waveform"]["display-background"] = "false";
  config["waveform"]["display-waveform-fill"] = "true";
  config["waveform"]["scrolling-with-player"] = "true";
  config["waveform"]["scrolling-with-selection"] = "true";
  config["waveform"]["respect-timing"] = "true";
  config["waveform"]["display"] = "true";
  config["waveform"]["renderer"] = "cairo";

  // [waveform-renderer]
  config["waveform-renderer"]["display-subtitle-text"] = "true";
  config["waveform-renderer"]["color-background"] = "#4C4C4CFF";
  config["waveform-renderer"]["color-wave"] = "#99CC4CFF";
  config["waveform-renderer"]["color-wave-fill"] = "#FFFFFFFF";
  config["waveform-renderer"]["color-subtitle"] = "#994C1999";
  config["waveform-renderer"]["color-subtitle-selected"] = "#E57F4C99";
  config["waveform-renderer"]["color-subtitle-invalid"] = "#FFFF00CC";
  config["waveform-renderer"]["color-text"] = "#FFFFFFFF";
  config["waveform-renderer"]["color-player-position"] = "#FFFFFFFF";

  // [interface]
  config["interface"]["use-dynamic-keyboard-shortcuts"] = "true";
  config["interface"]["maximize-window"] = "false";
  config["interface"]["used-autosave"] = "true";
  config["interface"]["ask-to-save-on-exit"] = "true";
  config["interface"]["create-backup-copy"] = "false";
  config["interface"]["autosave-minutes"] = "10";
  config["interface"]["max-undo"] = "200";

  // [encodings]
  config["encodings"]["encodings"] = "ISO-8859-15;UTF-8";
  config["encodings"]["used-auto-detected"] = "true";
  config["encodings"]["default"] = "UTF-8";

  // [subtitle-view]
  config["subtitle-view"]["property-alignment-center"] = "false";
  config["subtitle-view"]["show-character-per-line"] = "true";
  config["subtitle-view"]["enable-rubberband-selection"] = "false";
  config["subtitle-view"]["columns-displayed"] =
      "number;start;end;duration;text";
  config["subtitle-view"]["columns-list"] =
      "cps;duration;effect;end;layer;margin-l;margin-r;margin-v;name;note;"
      "number;start;style;text;translation";
  config["subtitle-view"]["used-ctrl-enter-to-confirm-change"] = "false";
  config["subtitle-view"]["do-not-disable-actions-during-editing"] = "false";
  config["subtitle-view"]["enable-goto-subtitle-number"] = "false";

  // [timing]
  config["timing"]["min-characters-per-second"] = "5";
  config["timing"]["max-characters-per-second"] = "20";
  config["timing"]["ideal-characters-per-second"] = "16";
  config["timing"]["min-gap-between-subtitles"] = "100";
  config["timing"]["min-display"] = "1000";
  config["timing"]["max-characters-per-line"] = "40";
  config["timing"]["max-line-per-subtitle"] = "2";
  config["timing"]["ignore-space"] = "false";
  config["timing"]["do-auto-timing-check"] = "true";

  // [external-video-player]
  config["external-video-player"]["command"] =
	  "mpv --sub-file=\"#subtitle_uri\" --start=\"#time\" \"#video_uri\" "
	  "--sub-auto=no --osd-level=2";
  config["external-video-player"]["format"] = "SubRip";
  config["external-video-player"]["use-video-player-file"] = "true";
  config["external-video-player"]["offset"] = "500";

  // [dialog-check-errors]
  config["dialog-check-errors"]["check-overlapping"] = "true";
  config["dialog-check-errors"]["check-too-short-display-time"] = "true";
  config["dialog-check-errors"]["check-too-long-display-time"] = "true";
  config["dialog-check-errors"]["check-too-long-line"] = "true";
  config["dialog-check-errors"]["check-gap-between-subtitles"] = "true";
  config["dialog-check-errors"]["check-max-line-per-subtitle"] = "true";

  // [check-error-plugins]
  config["check-error-plugins"]["overlapping-color"] = "#FF0000FF";
  config["check-error-plugins"]["too-short-display-time-color"] = "#B6D2FFFF";
  config["check-error-plugins"]["too-long-display-time-color"] = "#0414FFFF";
  config["check-error-plugins"]["too-long-line-color"] = "#FFDC0AFF";
  config["check-error-plugins"]["max-line-per-subtitle-color"] = "#00BF23FF";
  config["check-error-plugins"]["min-gap-between-subtitles-color"] =
      "#FF66ECFF";
}
