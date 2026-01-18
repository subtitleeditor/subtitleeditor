#pragma once

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

#include <glibmm.h>
#include <math.h>

#include <string>

/**
 **/
class SubtitleTime {
  public:
   SubtitleTime();

   SubtitleTime(const long& total_msecs);

   // e.g "0:10:50.600"
   SubtitleTime(const Glib::ustring& srt);

   SubtitleTime(const int& h, const int& m, const int& s, const int& ms);

   void set(const int& h, const int& m, const int& s, const int& ms);

   int hours() const;

   void set_hours(int value);

   int minutes() const;

   void set_minutes(int value);

   int seconds() const;

   void set_seconds(int value);

   int mseconds() const;

   void set_mseconds(int value);

   SubtitleTime operator-(const SubtitleTime& b) const;
   SubtitleTime operator+(const SubtitleTime& b) const;
   SubtitleTime operator*(const double& mult) const;
   double operator/(const SubtitleTime& b) const;
   SubtitleTime operator/(const double& div) const;
   SubtitleTime operator/(const long& div) const;

   bool operator==(const SubtitleTime& time) const;
   bool operator!=(const SubtitleTime& time) const;
   bool operator>(const SubtitleTime& time) const;
   bool operator>=(const SubtitleTime& time) const;
   bool operator<(const SubtitleTime& time) const;
   bool operator<=(const SubtitleTime& time) const;

   Glib::ustring str() const;

   // Check if the string has the good format 'H:MM:SS.MS'
   static bool validate(const Glib::ustring& str);

   // Return "0:00:00.000"
   static Glib::ustring null();

   // Convert the time to a frame using a framerate
   // e.g (450, 23.976)
   static SubtitleTime frame_to_time(const long int& frame, const float& framerate);

   // Convert the frame to the time using a framerate
   static long int time_to_frame(const SubtitleTime& time, const float& framerate);

  public:
   long totalmsecs{0};
};

// return hours:mins:secs,msecs
std::string getTime2String(const SubtitleTime& time);

long getMSecs(const SubtitleTime& time);

SubtitleTime getTime2MSecs(const long& total);
