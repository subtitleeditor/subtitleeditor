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

#include <error.h>
#include <extension/subtitleformat.h>
#include <libxml++/libxml++.h>
#include <utility.h>

class TimedTextAuthoringFormat1 : public SubtitleFormatIO {
 public:
  void open(Reader &file) {
    try {
      xmlpp::DomParser parser;
      // parser.set_validate();
      parser.set_substitute_entities();
      parser.parse_memory(file.get_data());

      if (!parser)
        throw IOFileError(_("Failed to open the file for reading."));

      // <tt> (root)
      const xmlpp::Node *root = parser.get_document()->get_root_node();

      // <body>
      const xmlpp::Element *body = dynamic_cast<const xmlpp::Element *>(
          root->get_children("body").front());
      if (body) {
        // <div>
        auto div = dynamic_cast<const xmlpp::Element *>(
            body->get_children("div").front());

        if (div) {
#ifdef HAVE_LIBXMLXX_3
          xmlpp::Node::const_NodeList list = div->get_children();
#else
          xmlpp::Node::NodeList list = div->get_children();
#endif
          for (const auto &node : list) {
            read_subtitle(dynamic_cast<const xmlpp::Element *>(node));
          }
        }
      }
    } catch (const std::exception &ex) {
      throw IOFileError(_("Failed to open the file for reading."));
    }
  }

  void save(Writer &file) {
    try {
      xmlpp::Document doc;

      xmlpp::Element *tt = doc.create_root_node("tt");
      tt->set_attribute("xml:lang", "");
      tt->set_attribute("xmlns", "http://www.w3.org/2006/10/ttaf1");

#ifdef HAVE_LIBXMLXX_3
      xmlpp::Element *body = tt->add_child_element("body");
#else
      xmlpp::Element *body = tt->add_child("body");
#endif

      // div subtitles
#ifdef HAVE_LIBXMLXX_3
      xmlpp::Element *div = body->add_child_element("div");
#else
      xmlpp::Element *div = body->add_child("div");
#endif

      div->set_attribute("xml:lang", "en");

      for (Subtitle sub = document()->subtitles().get_first(); sub; ++sub) {
        write_subtitle(div, sub);
      }

      file.write(doc.write_to_string_formatted());
    } catch (const std::exception &ex) {
      throw IOFileError(_("Failed to write to the file."));
    }
  }

  void read_subtitle(const xmlpp::Element *p) {
    if (p == NULL || p->get_name() != "p")
      return;

    Subtitle subtitle = document()->subtitles().append();

    // begin
    const xmlpp::Attribute *att_begin = p->get_attribute("begin");
    if (att_begin) {
      Glib::ustring begin = att_begin->get_value();

      subtitle.set_start(time_to_se(begin));
    }

    // end
    const xmlpp::Attribute *att_end = p->get_attribute("end");
    if (att_end) {
      Glib::ustring end = att_end->get_value();

      subtitle.set_end(time_to_se(end));
    } else {  // dur only if end failed
      const xmlpp::Attribute *att_dur = p->get_attribute("dur");
      if (att_dur) {
        Glib::ustring dur = att_dur->get_value();

        subtitle.set_duration(time_to_se(dur));
      }
    }

    // text
    if (p->has_child_text()) {
      Glib::ustring text;

#ifdef HAVE_LIBXMLXX_3
      xmlpp::Node::const_NodeList children = p->get_children();
      for (auto &node : children) {
        const xmlpp::ContentNode *cn = dynamic_cast<const xmlpp::ContentNode *>(node);
#else
      xmlpp::Node::NodeList children = p->get_children();
      for (const auto &node : children) {
        xmlpp::ContentNode *cn = dynamic_cast<xmlpp::ContentNode *>(node);
#endif
        if (cn == NULL)
          continue;
        if (!text.empty())
          text += "\n";
        text += cn->get_content();
      }

      subtitle.set_text(text);
    }
  }

  void write_subtitle(xmlpp::Element *root, const Subtitle &sub) {
    Glib::ustring text = sub.get_text();

    utility::replace(text, "\n", "<br/>");

#ifdef HAVE_LIBXMLXX_3
    xmlpp::Element *p = root->add_child_element("p");
#else
    xmlpp::Element *p = root->add_child("p");
#endif

    p->set_attribute("begin", time_to_ttaf1(sub.get_start()));
    p->set_attribute("end", time_to_ttaf1(sub.get_end()));
    p->set_attribute("dur", time_to_ttaf1(sub.get_duration()));
#ifdef HAVE_LIBXMLXX_3
    p->set_first_child_text(text);
#else
    p->set_child_text(text);
#endif
  }

  // Convert SE time to TT time.
  Glib::ustring time_to_ttaf1(const SubtitleTime &time) {
    return build_message("%.2i:%.2i:%.2i.%.3i", time.hours(), time.minutes(),
                         time.seconds(), time.mseconds());
  }

  // Convert TT time to SE time.
  SubtitleTime time_to_se(const Glib::ustring &value) {
    if (SubtitleTime::validate(value))
      return SubtitleTime(value);

    return SubtitleTime();
  }
};

class TimedTextAuthoringFormat1Plugin : public SubtitleFormat {
 public:
  SubtitleFormatInfo get_info() {
    SubtitleFormatInfo info;
    info.name = "Timed Text Authoring Format 1.0";
    info.extension = "xml";
    info.pattern = "^<tt";

    return info;
  }

  SubtitleFormatIO *create() {
    TimedTextAuthoringFormat1 *sf = new TimedTextAuthoringFormat1();
    return sf;
  }
};

REGISTER_EXTENSION(TimedTextAuthoringFormat1Plugin)
