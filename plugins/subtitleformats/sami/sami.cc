// subtitleeditor -- a tool to create or edit subtitle
//
// https://subtitleeditor.github.io/subtitleeditor/
// https://github.com/subtitleeditor/subtitleeditor/
//
// Copyright @ 2005-2018, kitone
// Copyright @ 2011, advance38 (author)
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
#include <string.h>
#include <utility.h>

static const int MAXBUF = 1024;
static const char STARTATT[] = "start=";
static const char SYNCTAG[] = "<Sync";
static const char BRTAG[] = "<br>";
static const char CRCHAR = '\r';
static const char LFCHAR = '\n';
// 12 hours * 60 minutes * 60 seconds * 1000
static const unsigned long SAMISYNC_MAXVAL = 43200000UL;

enum sami_state_t {
   SAMI_STATE_INIT = 0,
   SAMI_STATE_SYNC_START = 1,
   SAMI_STATE_P_OPEN = 2,
   SAMI_STATE_P_CLOSE = 3,
   SAMI_STATE_SYNC_END = 4,
   SAMI_STATE_FORCE_QUIT = 99
};

// format of sami (.smi) subtitles:
// <SAMI>
// <BODY>
// <SYNC Start=starttime>text
// <SYNC Start=endtime>&nbsp;
// (empty line)
class Sami : public SubtitleFormatIO {
  public:
   // open()
   //  : reads sami subtitle data from the handler 'file', parse each line,
   //    and store it to the internal data structure 'subtitles'.
   void open(Reader& file) {
      read_subtitle(file);
   }

   // save()
   //  : store each line from the internal data structure, in according to the
   // SAMI format.
   void save(Writer& file) {
      Subtitle sub_first = document()->subtitles().get_first();
      Glib::ustring sub_name = sub_first.get_name();

      Glib::ustring sami_header = Glib::ustring::compose(
         "<SAMI>\n"
         "<HEAD>\n"
         "<Title>%1</Title>\n"
         "<STYLE TYPE=Text/css>\n"
         "<!--\n"
         "P {margin-left: 8pt; margin-right: 8pt; margin-bottom: 2pt; "
         "margin-top: 2pt;\n"
         "   text-align: center; font-size: 12pt; font-family: arial, "
         "sans-serif;\n"
         "   font-weight: normal; color: white;}\n"
         ".ENCC {Name: English; lang: en-US; SAMIType: CC;}\n"
         ".KRCC {Name: Korean; lang: ko-KR; SAMIType: CC;}\n"
         "#STDPrn {Name: Standard Print;}\n"
         "#LargePrn {Name: Large Print; font-size: 20pt;}\n"
         "#SmallPrn {Name: Small Print; font-size: 10pt;}\n"
         "-->\n"
         "<!--\n"
         "subtitleeditor\n"
         "-->\n"
         "</STYLE>\n"
         "</HEAD>\n"
         "<BODY>\n",
         sub_name);

      Glib::ustring sami_tail = Glib::ustring(
         "</BODY>\n"
         "</SAMI>\n");

      file.write(sami_header);

      write_subtitle(file);

      file.write(sami_tail);
   }

   // trail_space()
   //  : trim the trailing white space characters in s.
   void trail_space(char* s) {
      while (isspace(*s)) {
         char* copy = s;
         do {
            copy[0] = copy[1];
            copy++;
         } while (*copy);
      }
      size_t i = strlen(s) - 1;
      while (i > 0 && isspace(s[i])) s[i--] = '\0';
   }

   // time_to_sami
   //  :
   Glib::ustring time_to_sami(const SubtitleTime& t) {
      unsigned int total_sec = (t.hours() * 3600) + (t.minutes() * 60) + t.seconds();

      return build_message("%i%03i", total_sec, t.mseconds());
   }

   // read_subtitle
   //  : read each line of subtitle
   void read_subtitle(Reader& file) {
      Subtitles subtitles = document()->subtitles();

      unsigned long start_sync = 0, end_sync = 0;
      int state = 0;
      Glib::ustring line;
      Glib::ustring text;
      Subtitle curSt;
      char tmptext[MAXBUF + 1] = "";
      char *p = nullptr, *q = nullptr;
      if (!file.getline(line))
         return;

      char* inptr = (char*)(line.c_str());

      do {
         switch (state) {
            case SAMI_STATE_INIT:
               inptr = strcasestr(inptr, STARTATT);
               if (inptr) {
                  start_sync = utility::string_to_int(inptr + 6);

                  // Get a line from the current subtitle on memory
                  curSt = subtitles.append();
                  curSt.set_start(start_sync);

                  state = SAMI_STATE_SYNC_START;
                  continue;
               }
               break;

            case SAMI_STATE_SYNC_START:  // find "<P"
               if ((inptr = strcasestr(inptr, "<P"))) {
                  inptr += 2;
                  state = SAMI_STATE_P_OPEN;
                  continue;
               }
               break;

            case SAMI_STATE_P_OPEN:  // find ">"
               if ((inptr = strchr(inptr, '>'))) {
                  inptr++;
                  state = SAMI_STATE_P_CLOSE;
                  p = tmptext;
                  continue;
               }
               break;

            case SAMI_STATE_P_CLOSE:  // get all text until '<' appears
               if (*inptr == '\0') {
                  break;
               } else if (strncasecmp(inptr, "&nbsp;", 6) == 0) {
                  *p++ = ' ';
                  inptr += 6;
               } else if (strncasecmp(inptr, "nbsp;", 5) == 0) {
                  *p++ = ' ';
                  inptr += 5;
               } else if (*inptr == CRCHAR) {
                  inptr++;
               } else if (strncasecmp(inptr, BRTAG, sizeof(BRTAG) - 1) == 0 || *inptr == LFCHAR) {
                  *p++ = LFCHAR;
                  trail_space(inptr);
                  if (*inptr == LFCHAR)
                     inptr++;
                  else
                     inptr += (sizeof(BRTAG) - 1);
               } else if (strncasecmp(inptr, SYNCTAG, sizeof(SYNCTAG) - 1) == 0) {
                  state = SAMI_STATE_SYNC_END;
               } else {
                  *p++ = *inptr++;
               }
               continue;

            case SAMI_STATE_SYNC_END:  // get the line for end sync or skip <TAG>
               q = strcasestr(inptr, STARTATT);
               if (q) {
                  // Now we are sure that this line is the end sync.

                  end_sync = utility::string_to_int(q + 6);
                  curSt.set_end(end_sync);

                  *p = '\0';
                  trail_space(tmptext);

                  // finalize the end sync of current line
                  if (tmptext[0] != '\0')
                     curSt.set_text(tmptext);

                  // an important check if this is end sync.
                  // Is there any delimiter "&nbsp;" in this line?
                  // If not, then this line is not a end sync, but a start sync.
                  if (!strstr(q, "nbsp;")) {
                     // start again from the beginning
                     state = SAMI_STATE_INIT;
                     continue;
                  }

                  if (file.getline(line)) {
                     inptr = (char*)(line.c_str());
                     p = tmptext;
                     // FIXME: assigning to 'char *' from incompatible type 'char'
                     // p = '\0';

                     state = SAMI_STATE_INIT;
                     continue;
                  } else {
                     state = SAMI_STATE_FORCE_QUIT;
                     break;
                  }
               } else {
                  end_sync = SAMISYNC_MAXVAL;
                  curSt.set_end(end_sync);

                  *p = '\0';
                  trail_space(tmptext);

                  // finalize the end sync of current line
                  if (tmptext[0] != '\0')
                     curSt.set_text(tmptext);

                  state = SAMI_STATE_FORCE_QUIT;
                  break;
               }

               inptr = strchr(inptr, '>');
               if (inptr) {
                  inptr++;
                  state = SAMI_STATE_P_CLOSE;
                  continue;
               }
               break;
         }  // end of switch

         // read next line
         if (state != SAMI_STATE_FORCE_QUIT && !file.getline(line))
            return;

         inptr = (char*)(line.c_str());

      } while (state != SAMI_STATE_FORCE_QUIT);
   }

   // write_subtitle
   //  : write each line of subtitle
   void write_subtitle(Writer& file) {
      for (Subtitle sub = document()->subtitles().get_first(); sub; ++sub) {
         Glib::ustring text = sub.get_text();
         Glib::ustring start_sync = time_to_sami(sub.get_start());
         Glib::ustring end_sync = time_to_sami(sub.get_end());

         utility::replace(text, "\n", "<br>");

         Glib::ustring final_text = Glib::ustring::compose(
            "<SYNC Start=%1><P Class=ENCC>\n"
            "%2\n"
            "<SYNC Start=%3><P Class=ENCC>&nbsp;\n",
            start_sync,
            text,
            end_sync);

         file.write(final_text);
      }
   }
};

class SamiPlugin : public SubtitleFormat {
  public:
   SubtitleFormatInfo get_info() {
      SubtitleFormatInfo info;
      info.name = "Sami";
      info.extension = "smi";
      info.pattern = "^<SAMI>";

      return info;
   }

   SubtitleFormatIO* create() {
      Sami* sf = new Sami();
      return sf;
   }
};

REGISTER_EXTENSION(SamiPlugin)
