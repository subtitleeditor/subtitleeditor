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
#include <error.h>
#include <extension/subtitleformat.h>
#include <filereader.h>
#include <i18n.h>
#include <libxml++/libxml++.h>
#include <player.h>
#include <subtitleeditorwindow.h>
#include <utility.h>
#include <waveformmanager.h>

// TODO:
// <subtitleview>
//  <selection>
//  <position>
// </subtitleview>
// <metadata>
//  video, title, info, comment ...
// </metadata>
class SubtitleEditorProject : public SubtitleFormatIO {
  public:
   void open(Reader& file) {
      try {
         initalize_dirname(file);

         xmlpp::DomParser parser;
         // parser.set_validate();
         parser.set_substitute_entities();
         parser.parse_memory(file.get_data());

         if (!parser)
            throw IOFileError(_("Failed to open the file for reading."));

         const xmlpp::Node* root = parser.get_document()->get_root_node();

         open_player(root);
         open_waveform(root);
         open_keyframes(root);
         open_styles(root);
         open_subtitles(root);
         open_subtitles_selection(root);
      } catch (const std::exception& ex) {
         throw IOFileError(_("Failed to open the file for reading."));
      }
   }

   void save(Writer& file) {
      try {
         xmlpp::Document xmldoc;

         xmlpp::Element* root = xmldoc.create_root_node("SubtitleEditorProject");
         root->set_attribute("version", "1.0");

         save_player(root);
         save_waveform(root);
         save_keyframes(root);
         save_styles(root);
         save_subtitles(root);
         save_subtitles_selection(root);

         file.write(xmldoc.write_to_string_formatted());
      } catch (const std::exception& ex) {
         throw IOFileError(_("Failed to write to the file."));
      }
   }

  private:
   void initalize_dirname(Reader& reader) {
      FileReader* fr = dynamic_cast<FileReader*>(&reader);
      if (fr != NULL) {
         Glib::ustring filename = Glib::filename_from_uri(fr->get_uri());
         m_project_dirname = Glib::path_get_dirname(filename);
      }
   }

   bool test_uri(const Glib::ustring& uri) {
      return test_filename(Glib::filename_from_uri(uri));
   }

   bool test_filename(const Glib::ustring& filename) {
      return Glib::file_test(filename, Glib::FILE_TEST_EXISTS);
   }

   Glib::ustring uri_to_project_relative_filename(const Glib::ustring& uri) {
      Glib::ustring basename = Glib::path_get_basename(Glib::filename_from_uri(uri));
      Glib::ustring relative = Glib::build_filename(m_project_dirname, basename);
      return Glib::filename_to_uri(relative);
   }

   const xmlpp::Element* get_unique_children(const xmlpp::Node* root, const Glib::ustring& name) {
#ifdef HAVE_LIBXMLXX_3
      const xmlpp::Node::const_NodeList children = root->get_children(name);
#else
      const xmlpp::Node::NodeList children = root->get_children(name);
#endif
      if (children.empty())
         return NULL;
      return dynamic_cast<const xmlpp::Element*>(children.front());
   }

   void open_player(const xmlpp::Node* root) {
      const xmlpp::Element* xml_pl = get_unique_children(root, "player");
      if (xml_pl == NULL)
         return;

      Glib::ustring uri = xml_pl->get_attribute_value("uri");

      Player* pl = SubtitleEditorWindow::get_instance()->get_player();

      if (pl->get_uri() == uri)
         return;

      if (!test_uri(uri) && test_uri(uri_to_project_relative_filename(uri)))
         uri = uri_to_project_relative_filename(uri);

      pl->open(uri);
   }

   void save_player(xmlpp::Element* root) {
      Player* pl = SubtitleEditorWindow::get_instance()->get_player();
      if (pl == NULL)
         return;

      Glib::ustring uri = pl->get_uri();
      if (uri.empty())
         return;

#ifdef HAVE_LIBXMLXX_3
      xmlpp::Element* xmlpl = root->add_child_element("player");
#else
      xmlpp::Element* xmlpl = root->add_child("player");
#endif
      xmlpl->set_attribute("uri", uri);
   }

   void open_waveform(const xmlpp::Node* root) {
      const xmlpp::Element* xml_wf = get_unique_children(root, "waveform");
      if (xml_wf == NULL)
         return;

      Glib::ustring uri = xml_wf->get_attribute_value("uri");
      if (uri.empty())
         return;

      if (!test_uri(uri) && test_uri(uri_to_project_relative_filename(uri)))
         uri = uri_to_project_relative_filename(uri);

      SubtitleEditorWindow::get_instance()->get_waveform_manager()->open_waveform(uri);
   }

   void save_waveform(xmlpp::Element* root) {
      WaveformManager* wm = SubtitleEditorWindow::get_instance()->get_waveform_manager();
      if (wm->has_waveform() == false)
         return;  // don't need to save without Waveform...

      Glib::RefPtr<Waveform> wf = wm->get_waveform();
      if (!wf)
         return;

#ifdef HAVE_LIBXMLXX_3
      xmlpp::Element* xmlwf = root->add_child_element("waveform");
#else
      xmlpp::Element* xmlwf = root->add_child("waveform");
#endif

      xmlwf->set_attribute("uri", wf->get_uri());
   }

   void open_keyframes(const xmlpp::Node* root) {
      const xmlpp::Element* xml_kf = get_unique_children(root, "keyframes");
      if (xml_kf == NULL)
         return;

      Glib::ustring uri = xml_kf->get_attribute_value("uri");
      if (uri.empty())
         return;

      if (!test_uri(uri) && test_uri(uri_to_project_relative_filename(uri)))
         uri = uri_to_project_relative_filename(uri);

      Glib::RefPtr<KeyFrames> kf = KeyFrames::create_from_file(uri);
      if (kf)
         SubtitleEditorWindow::get_instance()->get_player()->set_keyframes(kf);
   }

   void save_keyframes(xmlpp::Element* root) {
      Glib::RefPtr<KeyFrames> kf = SubtitleEditorWindow::get_instance()->get_player()->get_keyframes();
      if (!kf)
         return;  // don't need to save without KeyFrames...

#ifdef HAVE_LIBXMLXX_3
      xmlpp::Element* xmlwf = root->add_child_element("keyframes");
#else
      xmlpp::Element* xmlwf = root->add_child("keyframes");
#endif

      xmlwf->set_attribute("uri", kf->get_uri());
   }

   void open_styles(const xmlpp::Node* root) {
      const xmlpp::Element* xmlstyles = get_unique_children(root, "styles");
      if (xmlstyles == NULL)
         return;

      Styles styles = document()->styles();

#ifdef HAVE_LIBXMLXX_3
      const xmlpp::Node::const_NodeList list_styles = xmlstyles->get_children("style");
#else
      const xmlpp::Node::NodeList list_styles = xmlstyles->get_children("style");
#endif

      for (const auto& node : list_styles) {
         auto el = dynamic_cast<const xmlpp::Element*>(node);

         Style style = styles.append();

         auto attr_list = el->get_attributes();

         for (const auto& att : attr_list) {
            style.set(att->get_name(), att->get_value());
         }
      }
   }

   void save_styles(xmlpp::Element* root) {
#ifdef HAVE_LIBXMLXX_3
      xmlpp::Element* xmlstyles = root->add_child_element("styles");
#else
      xmlpp::Element* xmlstyles = root->add_child("styles");
#endif

      Styles styles = document()->styles();

      for (Style style = styles.first(); style; ++style) {
#ifdef HAVE_LIBXMLXX_3
         xmlpp::Element* xml = xmlstyles->add_child_element("style");
#else
         xmlpp::Element* xml = xmlstyles->add_child("style");
#endif

         std::map<Glib::ustring, Glib::ustring> values;
         style.get(values);

         for (const auto& i : values) {
            xml->set_attribute(i.first, i.second);
         }
      }
   }

   void open_subtitles(const xmlpp::Node* root) {
      const xmlpp::Element* xmlsubtitles = get_unique_children(root, "subtitles");
      if (xmlsubtitles == NULL)
         return;

      Glib::ustring timing_mode = xmlsubtitles->get_attribute_value("timing_mode");
      if (!timing_mode.empty()) {
         if (timing_mode == "TIME")
            document()->set_timing_mode(TIME);
         else if (timing_mode == "FRAME")
            document()->set_timing_mode(FRAME);
      }

      Glib::ustring edit_timing_mode = xmlsubtitles->get_attribute_value("edit_timing_mode");
      if (!edit_timing_mode.empty()) {
         if (edit_timing_mode == "TIME")
            document()->set_edit_timing_mode(TIME);
         else if (edit_timing_mode == "FRAME")
            document()->set_edit_timing_mode(FRAME);
      }

      Glib::ustring framerate = xmlsubtitles->get_attribute_value("framerate");
      if (!framerate.empty()) {
         float value = static_cast<float>(utility::string_to_double(framerate));
         if (value > 0)
            document()->set_framerate(get_framerate_from_value(value));
      }

      auto list_subtitles = xmlsubtitles->get_children("subtitle");

      Subtitles subtitles = document()->subtitles();

      for (const auto& node : list_subtitles) {
         auto el = dynamic_cast<const xmlpp::Element*>(node);

         Subtitle sub = subtitles.append();

         auto attr_list = el->get_attributes();

         for (const auto& att : attr_list) {
            sub.set(att->get_name(), att->get_value());
         }
      }
   }

   void save_subtitles(xmlpp::Element* root) {
#ifdef HAVE_LIBXMLXX_3
      xmlpp::Element* xmlsubtitles = root->add_child_element("subtitles");
#else
      xmlpp::Element* xmlsubtitles = root->add_child("subtitles");
#endif

      // document property
      xmlsubtitles->set_attribute("timing_mode", (document()->get_timing_mode() == TIME) ? "TIME" : "FRAME");
      xmlsubtitles->set_attribute("edit_timing_mode", (document()->get_edit_timing_mode() == TIME) ? "TIME" : "FRAME");
      xmlsubtitles->set_attribute("framerate", to_string(get_framerate_value(document()->get_framerate())));

      // subtitles
      Subtitles subtitles = document()->subtitles();

      for (Subtitle sub = subtitles.get_first(); sub; ++sub) {
#ifdef HAVE_LIBXMLXX_3
         xmlpp::Element* xmlsub = xmlsubtitles->add_child_element("subtitle");
#else
         xmlpp::Element* xmlsub = xmlsubtitles->add_child("subtitle");
#endif

         std::map<Glib::ustring, Glib::ustring> values;
         sub.get(values);

         for (const auto& i : values) {
            xmlsub->set_attribute(i.first, i.second);
         }
      }
   }

   void open_subtitles_selection(const xmlpp::Node* root) {
      const xmlpp::Element* xmlsubtitles = get_unique_children(root, "subtitles-selection");
      if (xmlsubtitles == NULL)
         return;

      auto list_subtitles = xmlsubtitles->get_children("subtitle");

      std::vector<Subtitle> selection(list_subtitles.size());

      Subtitles subtitles = document()->subtitles();

      unsigned int i = 0;
      for (auto it = list_subtitles.begin(); it != list_subtitles.end(); ++it, ++i) {
         const xmlpp::Element* el = dynamic_cast<const xmlpp::Element*>(*it);
         long path = utility::string_to_long(el->get_attribute_value("path"));

         selection[i] = subtitles.get(path + 1);  // /!\ warning: PATH is not NUM
      }
      subtitles.select(selection);
   }

   void save_subtitles_selection(xmlpp::Element* root) {
#ifdef HAVE_LIBXMLXX_3
      xmlpp::Element* xml = root->add_child_element("subtitles-selection");
#else
      xmlpp::Element* xml = root->add_child("subtitles-selection");
#endif

      std::vector<Subtitle> selection = document()->subtitles().get_selection();

      for (const auto& subtitle : selection) {
#ifdef HAVE_LIBXMLXX_3
         xmlpp::Element* xmlsub = xml->add_child_element("subtitle");
#else
         xmlpp::Element* xmlsub = xml->add_child("subtitle");
#endif
         xmlsub->set_attribute("path", subtitle.get("path"));
      }
   }

  protected:
   Glib::ustring m_project_dirname;
};

class SubtitleEditorProjectPlugin : public SubtitleFormat {
  public:
   SubtitleFormatInfo get_info() {
      SubtitleFormatInfo info;
      info.name = "Subtitle Editor Project";
      info.extension = "sep";
      info.pattern = "^<SubtitleEditorProject\\s.*>$";

      return info;
   }

   SubtitleFormatIO* create() {
      SubtitleEditorProject* sf = new SubtitleEditorProject();
      return sf;
   }
};

REGISTER_EXTENSION(SubtitleEditorProjectPlugin)
