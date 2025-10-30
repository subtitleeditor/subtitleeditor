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

#include <document.h>
#include <extension/subtitleformat.h>
#include <gtkmm.h>
#include <gtkmm_utility.h>
#include <utility.h>

#include <cstdio>
#include <iomanip>
#include <memory>

class DialogAdvancedSubStationAlphaPreferences : public Gtk::Dialog {
 protected:
  class ComboBoxLineBreakPolicy : public Gtk::ComboBoxText {
   public:
    ComboBoxLineBreakPolicy(BaseObjectType *cobject,
                            const Glib::RefPtr<Gtk::Builder> &)
        : Gtk::ComboBoxText(cobject) {
      append(_("Soft"));
      append(_("Hard"));
      append(_("Intelligent"));
    }

    // From config value
    void set_line_break_policy(const Glib::ustring &value) {
      if (value == "soft")
        set_active(0);
      else if (value == "hard")
        set_active(1);
      else if (value == "intelligent")
        set_active(2);
      else  // default if value is empty
        set_active(2);
    }

    Glib::ustring get_line_break_policy() {
      gint active = get_active_row_number();

      if (active == 0)
        return "soft";
      else if (active == 1)
        return "hard";
      else
        return "intelligent";
    }
  };

 public:
  DialogAdvancedSubStationAlphaPreferences(
      BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &xml)
      : Gtk::Dialog(cobject), m_comboLineBreakPolicy(nullptr) {
    xml->get_widget_derived("combo-line-break-policy", m_comboLineBreakPolicy);

    m_comboLineBreakPolicy->signal_changed().connect(
        sigc::mem_fun(*this, &DialogAdvancedSubStationAlphaPreferences::
                                 on_combo_line_break_policy_changed));

    Glib::ustring policy =
        cfg::get_string("AdvancedSubStationAlpha", "line-break-policy");
    m_comboLineBreakPolicy->set_line_break_policy(policy);
  }

  static void create() {
    std::unique_ptr<DialogAdvancedSubStationAlphaPreferences> dialog(
        gtkmm_utility::get_widget_derived<
            DialogAdvancedSubStationAlphaPreferences>(
            SE_DEV_VALUE(SE_PLUGIN_PATH_UI, SE_PLUGIN_PATH_DEV),
            "dialog-advancedsubstationalpha-preferences.ui",
            "dialog-advancedsubstationalpha-preferences"));

    dialog->run();
  }

  void on_combo_line_break_policy_changed() {
    cfg::set_string("AdvancedSubStationAlpha", "line-break-policy",
                    m_comboLineBreakPolicy->get_line_break_policy());
  }

 protected:
  ComboBoxLineBreakPolicy *m_comboLineBreakPolicy;
};

class AdvancedSubStationAlpha : public SubtitleFormatIO {
  int m_line_break_policy{3};

 public:
  AdvancedSubStationAlpha() {
    read_config_line_break_policy();
  }

  // soft:1
  // hard:2
  // intelligent:3 (default)
  void read_config_line_break_policy() {
    if (cfg::has_key("AdvancedSubStationAlpha", "line-break-policy") == false) {
      cfg::set_string("AdvancedSubStationAlpha", "line-break-policy",
                      "intelligent");
      cfg::set_comment("AdvancedSubStationAlpha", "line-break-policy",
                       "determine the policy of the line break, 3 options: "
                       "'soft', 'hard' or 'intelligent' (without quote, the "
                       "default value is 'intelligent')");
    }

    Glib::ustring policy =
        cfg::get_string("AdvancedSubStationAlpha", "line-break-policy");
    if (policy == "soft") {
      m_line_break_policy = 1;
    } else if (policy == "hard") {
      m_line_break_policy = 2;
    } else if (policy == "intelligent") {
      m_line_break_policy = 3;
    } else {
      cfg::set_string("AdvancedSubStationAlpha", "line-break-policy",
                      "intelligent");
      cfg::set_comment("AdvancedSubStationAlpha", "line-break-policy",
                       "determine the policy of the line break, 3 options: "
                       "'soft', 'hard' or 'intelligent' (without quote, the "
                       "default value is 'intelligent')");
      m_line_break_policy = 3;
    }
  }

  void open(Reader &file) {
    std::vector<Glib::ustring> lines = file.get_lines();

    read_script_info(lines);
    read_styles(lines);
    read_events(lines);
  }

  void save(Writer &file) {
    write_script_info(file);
    write_styles(file);
    write_events(file);
  }

  // Read the block [Script Info]
  void read_script_info(const std::vector<Glib::ustring> &lines) {
    se_dbg_msg(SE_DBG_IO, "read script info...");

    ScriptInfo &script_info = document()->get_script_info();

    Glib::RefPtr<Glib::Regex> re = Glib::Regex::create("^(.*?):\\s(.*?)$");
    Glib::RefPtr<Glib::Regex> re_block = Glib::Regex::create("^\\[.*\\]$");

    bool read = false;

    for (const auto &line : lines) {
      // We want to only read the scrip info block
      if (read) {
        if (re_block->match(line))
          return;  // new block, stop reading
      } else if (line.find("[Script Info]") != Glib::ustring::npos) {
        // This is the beginning of the script info block, start reading
        read = true;
      }

      if (!read)
        continue;
      if (!re->match(line))
        continue;

      std::vector<Glib::ustring> group = re->split(line);

      if (group.size() == 1)
        continue;

      Glib::ustring key = group[1];
      Glib::ustring value = group[2];

      script_info.data[key] = value;
    }
  }

  // Read the block [V4+ Styles]
  void read_styles(const std::vector<Glib::ustring> &lines) {
    se_dbg_msg(SE_DBG_IO, "read style...");

    Styles styles = document()->styles();

    Glib::RefPtr<Glib::Regex> re = Glib::Regex::create(
        "^Style:\\s*"
        "([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,"
        "]*"
        "),([^,]*),"
        "([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,"
        "]*"
        "),([^,]*),"
        "([^,]*),([^,]*),([^,]*)$");

    for (const auto &line : lines) {
      if (!re->match(line))
        continue;

      std::vector<Glib::ustring> group = re->split(line);
      if (group.size() == 1)
        continue;

      Style style = styles.append();

      style.set("name", group[1]);

      style.set("font-name", group[2]);
      style.set("font-size", group[3]);

      style.set("primary-color", from_ass_color(group[4]));
      style.set("secondary-color", from_ass_color(group[5]));
      style.set("outline-color", from_ass_color(group[6]));
      style.set("shadow-color", from_ass_color(group[7]));

      style.set("bold", from_ass_bool(group[8]));
      style.set("italic", from_ass_bool(group[9]));
      style.set("underline", from_ass_bool(group[10]));
      style.set("strikeout", from_ass_bool(group[11]));

      style.set("scale-x", group[12]);
      style.set("scale-y", group[13]);

      style.set("spacing", group[14]);
      style.set("angle", group[15]);

      style.set("border-style", group[16]);
      style.set("outline", group[17]);
      style.set("shadow", group[18]);

      style.set("alignment", group[19]);

      style.set("margin-l", group[20]);
      style.set("margin-r", group[21]);
      style.set("margin-v", group[22]);

      style.set("encoding", group[23]);
    }
  }

  // Read the block [Events]
  void read_events(const std::vector<Glib::ustring> &lines) {
    se_dbg_msg(SE_DBG_IO, "read events...");

    Subtitles subtitles = document()->subtitles();

    Glib::RefPtr<Glib::Regex> re = Glib::Regex::create(
        "^Dialogue:\\s*([^,]*),([^,]*),([^,]*),\\**([^,]*),([^,]*),([^,]*),(["
        "^,"
        "]*),([^,]*),([^,]*),(.*)$");

    for (const auto &line : lines) {
      if (!re->match(line))
        continue;

      std::vector<Glib::ustring> group = re->split(line);
      if (group.size() == 1)
        continue;
      Subtitle sub = subtitles.append();

      // layer (ASS: integer, default 0)
      int layer = 0;
      if (!group[1].empty()) {
        // safe integer parse; defaults to 0 on failure
        char *endptr = nullptr;
        long l = std::strtol(group[1].c_str(), &endptr, 10);
        if (endptr != group[1].c_str())  // parsed something
          layer = static_cast<int>(l);
      }
      sub.set_layer(Glib::ustring::format(layer));

      // start, end times
      sub.set_start_and_end(from_ass_time(group[2]), from_ass_time(group[3]));

      // style
      sub.set_style(group[4]);

      // name
      sub.set_name(group[5]);

      // margin lrv
      sub.set_margin_l(group[6]);
      sub.set_margin_r(group[7]);
      sub.set_margin_v(group[8]);

      // effect
      sub.set_effect((group[9]));

      // text
      utility::replace(group[10], "\\n", "\n");
      utility::replace(group[10], "\\N", "\n");

      sub.set_text(group[10]);
    }
  }

  // Write the block [Script Info]
  void write_script_info(Writer &file) {
    file.write(Glib::ustring::compose(
        "[Script Info]\n"
        "; This script was created by subtitleeditor (%1)\n"
        "; https://kitone.github.io/subtitleeditor/\n",
        Glib::ustring(VERSION)));

    ScriptInfo &scriptInfo = document()->get_script_info();

    scriptInfo.data["ScriptType"] = "V4.00+";  // Set ASS format

    for (const auto &i : scriptInfo.data) {
      file.write(i.first + ": " + i.second + "\n");
    }

    // Only if one of PlayRes is missing
    guint width, height;
    if (get_screen_resolution(width, height) &&
        has_play_res(scriptInfo) == false) {
      file.write(
          Glib::ustring::compose("PlayResX: %1\n"
                                 "PlayResY: %2\n",
                                 width, height));
    }

    // End of block, empty line
    file.write("\n");
  }

  // Write the block [V4+ Styles]
  void write_styles(Writer &file) {
    file.write("[V4+ Styles]\n");
    file.write(
        "Format: "
        "Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, "
        "OutlineColour, "
        "BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, "
        "Spacing, Angle, "
        "BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, "
        "Encoding\n");

    // Default style if it's empty
    if (document()->styles().size() == 0) {
      Glib::ustring default_style;

      if (cfg::has_key("AdvancedSubStationAlpha", "default-style") == false) {
        // Write the default ASS style
        default_style =
            "Default,Sans,18,&H00FFFFFF,&H0000FFFF,&H000078B4,&H00000000,0,0,"
            "0,"
            "0,100,100,0,0,1,0,0,2,20,20,20,0";
        cfg::set_string("AdvancedSubStationAlpha", "default-style",
                        default_style);
        cfg::set_comment("AdvancedSubStationAlpha", "default-style",
                         "Without style, this one will be used during save");
      } else {
        default_style =
            cfg::get_string("AdvancedSubStationAlpha", "default-style");
      }

      // write without changing the document
      file.write("Style: " + default_style + "\n");

      // Style style = document()->styles().append();
      // style.set("name", "Default");
    }

    for (Style style = document()->styles().first(); style; ++style) {
      file.write(Glib::ustring::compose(
          "Style: %1,%2,%3,%4,%5,%6,%7\n",
          Glib::ustring::compose("%1,%2,%3", style.get("name"),
                                 style.get("font-name"),
                                 style.get("font-size")),

          Glib::ustring::compose("%1,%2,%3,%4",
                                 to_ass_color(style.get("primary-color")),
                                 to_ass_color(style.get("secondary-color")),
                                 to_ass_color(style.get("outline-color")),
                                 to_ass_color(style.get("shadow-color"))),

          Glib::ustring::compose("%1,%2,%3,%4", to_ass_bool(style.get("bold")),
                                 to_ass_bool(style.get("italic")),
                                 to_ass_bool(style.get("underline")),
                                 to_ass_bool(style.get("strikeout"))),

          Glib::ustring::compose("%1,%2,%3,%4", style.get("scale-x"),
                                 style.get("scale-y"), style.get("spacing"),
                                 style.get("angle")),

          Glib::ustring::compose("%1,%2,%3,%4", style.get("border-style"),
                                 style.get("outline"), style.get("shadow"),
                                 style.get("alignment")),

          Glib::ustring::compose("%1,%2,%3", style.get("margin-l"),
                                 style.get("margin-r"), style.get("margin-v")),

          style.get("encoding")));
    }

    // End of block, empty line
    file.write("\n");
  }

  // Write the block [Events]
  void write_events(Writer &file) {
    file.write("[Events]\n");
    // format:
    file.write(
        "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, "
        "Effect, Text\n");

    Glib::RefPtr<Glib::Regex> re_intelligent_linebreak =
        Glib::Regex::create("\n(?=-\\s.*)", Glib::REGEX_MULTILINE);

    // line break policy: 1 = soft, 2=hard, 3=intelligent

    for (Subtitle sub = document()->subtitles().get_first(); sub; ++sub) {
      Glib::ustring text = sub.get_text();

      if (m_line_break_policy == 1) {
        utility::replace(text, "\n", "\\n");
      } else if (m_line_break_policy == 2) {
        utility::replace(text, "\n", "\\N");
      } else if (m_line_break_policy == 3) {
        if (re_intelligent_linebreak->match(text))
          utility::replace(text, "\n", "\\N");
        else
          utility::replace(text, "\n", "\\n");
      }

      file.write(Glib::ustring::compose(
          "Dialogue: %1,%2,%3,%4,%5,%6,%7,%8\n", sub.get_layer(),
          to_ass_time(sub.get_start()), to_ass_time(sub.get_end()),
          sub.get_style(), sub.get_name(),
          Glib::ustring::compose(
              "%1,%2,%3",
              Glib::ustring::format(std::setw(4), std::setfill(L'0'),
                                    sub.get_margin_l()),
              Glib::ustring::format(std::setw(4), std::setfill(L'0'),
                                    sub.get_margin_r()),
              Glib::ustring::format(std::setw(4), std::setfill(L'0'),
                                    sub.get_margin_v())),
          sub.get_effect(), text));
    }

    // End of block, empty line
    // file << std::endl;
  }

  // Convert time from SE to ASS
  Glib::ustring to_ass_time(const SubtitleTime &time) {
    auto hundredths = static_cast<int>((time.mseconds() + 0.5) / 10);
    return build_message("%01i:%02i:%02i.%02i", time.hours(), time.minutes(),
                         time.seconds(), hundredths);
  }

  // Convert time from ASS to SE
  SubtitleTime from_ass_time(const Glib::ustring &t) {
    int h, m, s, ms;
    if (std::sscanf(t.c_str(), "%d:%d:%d.%d", &h, &m, &s, &ms) == 4)
      return SubtitleTime(h, m, s, ms * 10);

    return SubtitleTime::null();
  }

  // Convert bool from SE to ASS
  // ASS: false == 0, true == -1
  Glib::ustring to_ass_bool(const Glib::ustring &value) {
    return (value == "0") ? "0" : "-1";
  }

  // Convert bool from ASS to SE
  // ASS: 0 == false, -1 == true
  Glib::ustring from_ass_bool(const Glib::ustring &value) {
    return (value == "0") ? "0" : "1";
  }

  // Convert color from SE to ASS
  Glib::ustring to_ass_color(const Color &color) {
    Color c(color);

    unsigned int r = c.getR();
    unsigned int g = c.getG();
    unsigned int b = c.getB();
    unsigned int a = 255 - c.getA();

    unsigned int abgr = a << 24 | b << 16 | g << 8 | r << 0;

    return build_message("&H%08X", abgr);
  }

  // Convert color from ASS to SE
  Glib::ustring from_ass_color(const Glib::ustring &str) {
    try {
      Glib::ustring value = str;

      if (value.size() > 2) {
        if (value[0] == '&')
          value.erase(0, 1);
        if (value[0] == 'h' || value[0] == 'H')
          value.erase(0, 1);
        if (value[value.size()] == '&')
          value.erase(value.size() - 1, 1);
      }

      long temp[4] = {0, 0, 0, 0};

      for (int i = 0; i < 4; ++i) {
        if (value.size() > 0) {
          Glib::ustring tmp = value.substr(value.size() - 2, 2);

          temp[i] = strtoll(tmp.c_str(), NULL, 16);

          value = value.substr(0, value.size() - 2);
        }
      }
      return Color(static_cast<unsigned int>(temp[0]),
                   static_cast<unsigned int>(temp[1]),
                   static_cast<unsigned int>(temp[2]),
                   static_cast<unsigned int>(255 - temp[3]))
          .to_string();
    } catch (...) {
    }

    return Color(255, 255, 255, 255).to_string();
  }

  // Convert time from SE to SSA
  Glib::ustring to_ssa_time(const SubtitleTime &t) {
    return build_message("%01i:%02i:%02i.%02i", t.hours(), t.minutes(),
                         t.seconds(), (t.mseconds() + 5) / 10);
  }

  bool get_screen_resolution(guint &width, guint &height) {
    Glib::RefPtr<Gdk::Screen> screen =
        Gdk::Display::get_default()->get_default_screen();
    if (!screen)
      return false;

    width = screen->get_width();
    height = screen->get_height();

    return true;
  }

  bool has_play_res(const ScriptInfo &script) {
    if (script.data.find("PlayResX") != script.data.end() ||
        script.data.find("PlayResY") != script.data.end())
      return true;
    return false;
  }
};

class AdvancedSubStationAlphaPlugin : public SubtitleFormat {
 public:
  bool is_configurable() {
    return true;
  }

  void create_configure_dialog() {
    DialogAdvancedSubStationAlphaPreferences::create();
  }

  SubtitleFormatInfo get_info() {
    SubtitleFormatInfo info;
    info.name = "Advanced Sub Station Alpha";
    info.extension = "ass";
    info.pattern = "^ScriptType:\\s*[vV]4.00\\+$";

    return info;
  }

  virtual SubtitleFormatIO *create() {
    AdvancedSubStationAlpha *sf = new AdvancedSubStationAlpha();
    return sf;
  }
};

REGISTER_EXTENSION(AdvancedSubStationAlphaPlugin)
