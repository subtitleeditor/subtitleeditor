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

#include "timeutility.h"

#include "i18n.h"

// Return the label of the framerate.
Glib::ustring get_framerate_label(FRAMERATE framerate) {
   Glib::ustring ret;

   switch (framerate) {
      case FRAMERATE_23_976:
         ret = _("23.976 fps");
         break;
      case FRAMERATE_24:
         ret = _("24 fps");
         break;
      case FRAMERATE_25:
         ret = _("25 fps");
         break;
      case FRAMERATE_29_97:
         ret = _("29.97 fps");
         break;
      case FRAMERATE_30:
         ret = _("30 fps");
         break;
      default:
         ret = _("Invalid fps");
         break;
   }

   return ret;
}

// Return the real value of the framerate.
float get_framerate_value(FRAMERATE framerate) {
   float ret = 0;

   switch (framerate) {
      case FRAMERATE_23_976:
         ret = 23.976f;  // 24 / 1.001
         break;
      case FRAMERATE_24:
         ret = 24.0f;
         break;
      case FRAMERATE_25:
         ret = 25.0f;
         break;
      case FRAMERATE_29_97:
         ret = 29.97f;  // 30 / 1.001
         break;
      case FRAMERATE_30:
         ret = 30.0f;
         break;
      default:
         ret = 0.0f;
         break;
   }

   return ret;
}

// Return the framerate from the value.
FRAMERATE get_framerate_from_value(float fps) {
   FRAMERATE framerate;

   int value = static_cast<int>((fps * 1000) + 0.5);

   if (value == 23976)
      framerate = FRAMERATE_23_976;
   else if (value == 24000)
      framerate = FRAMERATE_24;
   else if (value == 25000)
      framerate = FRAMERATE_25;
   else if (value == 29970)
      framerate = FRAMERATE_29_97;
   else if (value == 30000)
      framerate = FRAMERATE_30;
   else
      framerate = FRAMERATE_23_976;

   return framerate;
}
