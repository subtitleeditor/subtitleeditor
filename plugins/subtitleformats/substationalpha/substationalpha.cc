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

#include <document.h>
#include <extension/subtitleformat.h>
#include <gtkmm.h>
#include <gtkmm_utility.h>
#include <utility.h>
#include <cstdio>
#include <iomanip>
#include <memory>

class DialogSubStationAlphaPreferences : public Gtk::Dialog {
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
  DialogSubStationAlphaPreferences(BaseObjectType *cobject,
                                   const Glib::RefPtr<Gtk::Builder> &xml)
      : Gtk::Dialog(cobject), m_comboLineBreakPolicy(nullptr) {
    xml->get_widget_derived("combo-line-break-policy", m_comboLineBreakPolicy);

    m_comboLineBreakPolicy->signal_changed().connect(sigc::mem_fun(
        *this,
        &DialogSubStationAlphaPreferences::on_combo_line_break_policy_changed));

    Glib::ustring policy =
        cfg::get_string("SubStationAlpha", "line-break-policy");
    m_comboLineBreakPolicy->set_line_break_policy(policy);
  }

  static void create() {
    std::unique_ptr<DialogSubStationAlphaPreferences> dialog(
        gtkmm_utility::get_widget_derived<DialogSubStationAlphaPreferences>(
            SE_DEV_VALUE(SE_PLUGIN_PATH_UI, SE_PLUGIN_PATH_DEV),
            "dialog-substationalpha-preferences.ui",
            "dialog-substationalpha-preferences"));

    dialog->run();
  }

  void on_combo_line_break_policy_changed() {
    cfg::set_string("SubStationAlpha", "line-break-policy",
                    m_comboLineBreakPolicy->get_line_break_policy());
  }

 protected:
  ComboBoxLineBreakPolicy *m_comboLineBreakPolicy;
};

class SubStationAlpha : public SubtitleFormatIO {
  int m_line_break_policy{3};

 public:
  SubStationAlpha() {
    read_config_line_break_policy();
  }

  // soft:1
  // hard:2
  // intelligent:3 (default)
  void read_config_line_break_policy() {
    if (cfg::has_key("SubStationAlpha", "line-break-policy") == false) {
      cfg::set_string("SubStationAlpha", "line-break-policy", "intelligent");
      cfg::set_comment("SubStationAlpha", "line-break-policy",
                       "determine the policy of the line break, 3 options: "
                       "'soft', 'hard' or 'intelligent' (without quote, the "
                       "default value is 'intelligent')");
    }

    Glib::ustring policy =
        cfg::get_string("SubStationAlpha", "line-break-policy");
    if (policy == "soft") {
      m_line_break_policy = 1;
    } else if (policy == "hard") {
      m_line_break_policy = 2;
    } else if (policy == "intelligent") {
      m_line_break_policy = 3;
    } else {
      cfg::set_string("SubStationAlpha", "line-break-policy", "intelligent");
      cfg::set_comment("SubStationAlpha", "line-break-policy",
                       "determine the policy of the line break, 3 options: "
                       "'soft', 'hard' or 'intelligent' (without quote, the "
                       "default value is 'intelligent')");
      m_line_break_policy = 3;
    }
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
        // This is the beginning of the script info block, reading
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

  // Read the bloc [V4 Style]
  void read_styles(const std::vector<Glib::ustring> &lines) {
    se_dbg_msg(SE_DBG_IO, "read style...");

    Styles styles = document()->styles();

    Glib::RefPtr<Glib::Regex> re = Glib::Regex::create(
        "^Style:\\s*([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^"
        ",]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),("
        "[^,]*),([^,]*)$");

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

      style.set("primary-color", from_ssa_color(group[4]));
      style.set("secondary-color", from_ssa_color(group[5]));
      style.set("outline-color", from_ssa_color(group[6]));
      style.set("shadow-color", from_ssa_color(group[7]));

      style.set("bold", from_ssa_bool(group[8]));
      style.set("italic", from_ssa_bool(group[9]));

      style.set("border-style", group[10]);

      style.set("outline", group[11]);
      style.set("shadow", group[12]);

      style.set("alignment", alignment_from_ssa(group[13]));

      style.set("margin-l", group[14]);
      style.set("margin-r", group[15]);
      style.set("margin-v", group[16]);

      // style.set("alpha", group[17]);
      // style.set("encoding", group[18]);
    }
  }

  // Read the bloc [Events]
  void read_events(const std::vector<Glib::ustring> &lines) {
    se_dbg_msg(SE_DBG_IO, "read events...");

    Subtitles subtitles = document()->subtitles();

    Glib::RefPtr<Glib::Regex> re = Glib::Regex::create(
        "^Dialogue:\\s*([^,]*),([^,]*),([^,]*),\\**([^,]*),([^,]*),([^,]*),([^,"
        "]*),([^,]*),([^,]*),(.*)$");

    for (const auto &line : lines) {
      if (!re->match(line))
        continue;

      std::vector<Glib::ustring> group = re->split(line);
      if (group.size() == 1)
        continue;

      Subtitle sub = subtitles.append();

      // start, end times
      sub.set_start_and_end(from_ssa_time(group[2]), from_ssa_time(group[3]));

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

  void write_script_info(Writer &file) {
    file.write(Glib::ustring::compose(
        "[Script Info]\n"
        "; This script was created by subtitleeditor (%1)\n"
        "; https://subtitleeditor.github.io/subtitleeditor/\n",
        Glib::ustring(VERSION)));

    ScriptInfo &scriptInfo = document()->get_script_info();

    scriptInfo.data["ScriptType"] = "V4.00";  // Set SSA format

    for (const auto &i : scriptInfo.data) {
      file.write(i.first + ": " + i.second + "\n");
    }

    // End of block, empty line
    file.write("\n");
  }

  void write_styles(Writer &file) {
    file.write("[V4 Styles]\n");
    file.write(
        "Format: "
        "Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, "
        "TertiaryColour, "
        "BackColour, Bold, Italic, BorderStyle, Outline, Shadow, Alignment, "
        "MarginL, "
        "MarginR, MarginV, AlphaLevel, Encoding\n");

    // Default style if it's empty
    if (document()->styles().size() == 0) {
      Glib::ustring default_style;

      if (cfg::has_key("SubStationAlpha", "default-style") == false) {
        // Write the default ASS style
        default_style =
            "Default,Sans,18,16777215,65535,30900,0,0,0,1,0,0,2,20,20,20,0,0";
        cfg::set_string("SubStationAlpha", "default-style",
                        "Without style, this one will be used during save");
      } else {
        default_style = cfg::get_string("SubStationAlpha", "default-style");
      }
      // write without changing the document
      file.write("Style: " + default_style + "\n");

      // Style style = document()->styles().append();
      // style.set("name", "Default");
    }

    for (Style style = document()->styles().first(); style; ++style) {
      file.write(Glib::ustring::compose(
          "Style: %1,%2,%3,%4,%5,%6\n",
          Glib::ustring::compose("%1,%2,%3", style.get("name"),
                                 style.get("font-name"),
                                 style.get("font-size")),

          Glib::ustring::compose("%1,%2,%3,%4",
                                 to_ssa_color(style.get("primary-color")),
                                 to_ssa_color(style.get("secondary-color")),
                                 to_ssa_color(style.get("outline-color")),
                                 to_ssa_color(style.get("shadow-color"))),

          Glib::ustring::compose("%1,%2", to_ssa_bool(style.get("bold")),
                                 to_ssa_bool(style.get("italic"))),

          Glib::ustring::compose("%1,%2,%3,%4", style.get("border-style"),
                                 style.get("outline"), style.get("shadow"),
                                 alignment_to_ssa(style.get("alignment"))),

          Glib::ustring::compose("%1,%2,%3", style.get("margin-l"),
                                 style.get("margin-r"), style.get("margin-v")),

          Glib::ustring::compose("%1,%2",
                                 0,  // alpha
                                 style.get("encoding"))));
    }

    // End of block, empty line
    file.write("\n");
  }

  // Write the block [Events]
  void write_events(Writer &file) {
    file.write("[Events]\n");
    // format:
    file.write(
        "Format: Marked, Start, End, Style, Name, MarginL, MarginR, MarginV, "
        "Effect, Text\n");

    // line break policy: 1 = soft, 2=hard, 3=intelligent
    Glib::RefPtr<Glib::Regex> re_intelligent_linebreak =
        Glib::Regex::create("\n(?=-\\s.*)", Glib::REGEX_MULTILINE);

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
          "Dialogue: Marked=0,%1,%2,%3,%4,%5,%6,%7\n",
          to_ssa_time(sub.get_start()), to_ssa_time(sub.get_end()),
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
    file.write("\n");
  }

  // Convert time from SE to SSA
  Glib::ustring to_ssa_time(const SubtitleTime &time) {
    auto hundredths = static_cast<int>((time.mseconds() + 0.5) / 10);
    return build_message("%01i:%02i:%02i.%02i", time.hours(), time.minutes(),
                         time.seconds(), hundredths);
  }

  // Convert time from ssa to SE
  SubtitleTime from_ssa_time(const Glib::ustring &t) {
    int h, m, s, ms;
    if (std::sscanf(t.c_str(), "%d:%d:%d.%d", &h, &m, &s, &ms) == 4)
      return SubtitleTime(h, m, s, ms * 10);

    return SubtitleTime::null();
  }

  // Convert bool from SE to SSA
  // SSA: false == 0, true == -1
  Glib::ustring to_ssa_bool(const Glib::ustring &value) {
    return (value == "0") ? "0" : "-1";
  }

  // Convert bool from SSA to SE
  // 0 == false -1 == true
  Glib::ustring from_ssa_bool(const Glib::ustring &value) {
    return (value == "0") ? "0" : "1";
  }

  // Convert color from SE to SSA
  Glib::ustring to_ssa_color(const Color &color) {
    Color c(color);

    unsigned int r = c.getR();
    unsigned int g = c.getG();
    unsigned int b = c.getB();

    unsigned int bgr = b << 16 | g << 8 | r << 0;

    return to_string(bgr);
  }

  Glib::ustring from_ssa_color(const Glib::ustring &str) {
    int ssa = utility::string_to_int(str);

    unsigned int r = (ssa & 0x0000FF) >> 0;
    unsigned int g = (ssa & 0x00FF00) >> 8;
    unsigned int b = (ssa & 0xFF0000) >> 16;

    Color color;
    color.set(r, g, b, 255);
    return color.to_string();
  }

  Glib::ustring alignment_to_ssa(const Glib::ustring &value) {
    std::map<int, int> map;
    map[1] = 1;
    map[2] = 2;
    map[3] = 3;
    map[4] = 9;
    map[5] = 10;
    map[6] = 11;
    map[7] = 5;
    map[8] = 6;
    map[9] = 7;

    return to_string(map[utility::string_to_int(value)]);
  }

  Glib::ustring alignment_from_ssa(const Glib::ustring &value) {
    std::map<int, int> map;
    map[1] = 1;
    map[2] = 2;
    map[3] = 3;
    map[9] = 4;
    map[10] = 5;
    map[11] = 6;
    map[5] = 7;
    map[6] = 8;
    map[7] = 9;

    return to_string(map[utility::string_to_int(value)]);
  }
};

class SubStationAlphaPlugin : public SubtitleFormat {
 public:
  bool is_configurable() {
    return true;
  }

  void create_configure_dialog() {
    DialogSubStationAlphaPreferences::create();
  }

  SubtitleFormatInfo get_info() {
    SubtitleFormatInfo info;
    info.name = "Sub Station Alpha";
    info.extension = "ssa";
    info.pattern = "^ScriptType:\\s*[vV]4.00$";

    return info;
  }

  SubtitleFormatIO *create() {
    SubStationAlpha *sf = new SubStationAlpha();
    return sf;
  }
};

REGISTER_EXTENSION(SubStationAlphaPlugin)
