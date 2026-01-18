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

#include "adobeencoredvd.h"

class AdobeEncoreDVDPALPlugin : public SubtitleFormat {
  public:
   // First line should simply be:
   // number start_time stop_time some_text
   // 1 00:00:00:1 00:00:10:5 text (PAL)
   SubtitleFormatInfo get_info() {
      SubtitleFormatInfo info;

      info.name = "Adobe Encore DVD (PAL)";
      info.extension = "txt";
      info.pattern = "^\\d+\\s(\\d+(:)\\d+\\2\\d+\\2\\d+ ){2}.*?\\R";

      return info;
   }

   SubtitleFormatIO* create() {
      return new AdobeEncoreDVD(FRAMERATE_25);
   }
};

REGISTER_EXTENSION(AdobeEncoreDVDPALPlugin)
