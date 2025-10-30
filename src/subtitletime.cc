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

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "subtitletime.h"
#include "utility.h"

SubtitleTime::SubtitleTime() {
}

SubtitleTime::SubtitleTime(const long &total) : totalmsecs(total) {
}

SubtitleTime::SubtitleTime(const Glib::ustring &str) : totalmsecs(0) {
  try {
    int hours, mins, secs, msecs;

    Glib::ustring::size_type pos = 0, end = 0;

    // hours
    end = str.find(":", pos);
    from_string(str.substr(pos, end), hours);
    pos = end + 1;

    if (hours < 0)
      hours = -hours;

    // minutes
    end = str.find(":", pos);
    from_string(str.substr(pos, end), mins);
    pos = end + 1;

    // seconds
    double ms = 0.0;

    end = str.size();

    from_string(str.substr(pos, end), ms);

    // split into seconds and fraction
    secs = static_cast<int>(ms);

    msecs = static_cast<int>((ms - secs) * 1000 + 0.5);

    set(hours, mins, secs, msecs);

    if (str.find("-") != Glib::ustring::npos)
      totalmsecs = -1 * totalmsecs;
  } catch (...) {
    g_warning("Could not initialize time from string :'%s'", str.c_str());
  }
}

SubtitleTime::SubtitleTime(const int &h, const int &m, const int &s,
                           const int &ms)
    : totalmsecs(0) {
  set(h, m, s, ms);
}

void SubtitleTime::set(const int &h, const int &m, const int &s,
                       const int &ms) {
  totalmsecs = h * 3600000 + m * 60000 + s * 1000 + ms;
}

int SubtitleTime::hours() const {
  return totalmsecs / 3600000;
}

void SubtitleTime::set_hours(int value) {
  totalmsecs += (value - hours()) * 3600000;
}

int SubtitleTime::minutes() const {
  return (totalmsecs % 3600000) / 60000;
}

void SubtitleTime::set_minutes(int value) {
  totalmsecs += (value - minutes()) * 60000;
}

int SubtitleTime::seconds() const {
  return (totalmsecs % 60000) / 1000;
}

void SubtitleTime::set_seconds(int value) {
  totalmsecs += (value - seconds()) * 1000;
}

int SubtitleTime::mseconds() const {
  return totalmsecs % 1000;
}

void SubtitleTime::set_mseconds(int value) {
  totalmsecs += value - mseconds();
}

SubtitleTime SubtitleTime::operator-(const SubtitleTime &b) const {
  long total = totalmsecs - b.totalmsecs;
  return SubtitleTime(total);
}

SubtitleTime SubtitleTime::operator+(const SubtitleTime &b) const {
  long total = totalmsecs + b.totalmsecs;
  return SubtitleTime(total);
}

double SubtitleTime::operator/(const SubtitleTime &b) const {
  double total = totalmsecs / b.totalmsecs;
  return total;
}

SubtitleTime SubtitleTime::operator/(const double &div) const {
  double total = static_cast<double>(totalmsecs) / div;
  return SubtitleTime(static_cast<long>(total));
}

SubtitleTime SubtitleTime::operator/(const long &div) const {
  double total = static_cast<double>(totalmsecs) / static_cast<double>(div);
  return SubtitleTime(static_cast<long>(total));
}

SubtitleTime SubtitleTime::operator*(const double &mult) const {
  double total = static_cast<double>(totalmsecs * mult);
  return SubtitleTime(static_cast<long int>(total));
}

bool SubtitleTime::operator>(const SubtitleTime &time) const {
  return totalmsecs > time.totalmsecs;
}

bool SubtitleTime::operator==(const SubtitleTime &time) const {
  return totalmsecs == time.totalmsecs;
}

bool SubtitleTime::operator!=(const SubtitleTime &time) const {
  return totalmsecs != time.totalmsecs;
}

bool SubtitleTime::operator>=(const SubtitleTime &time) const {
  return totalmsecs >= time.totalmsecs;
}

bool SubtitleTime::operator<(const SubtitleTime &time) const {
  return totalmsecs < time.totalmsecs;
}

bool SubtitleTime::operator<=(const SubtitleTime &time) const {
  return totalmsecs <= time.totalmsecs;
}

Glib::ustring SubtitleTime::str() const {
  std::string sign;
  long t = totalmsecs;

  if (t < 0) {
    sign = "-";
    t = -t;
  }

  int hours = t / 3600000;
  int minutes = (t % 3600000) / 60000;
  int seconds = (t % 60000) / 1000;
  int mseconds = t % 1000;

  gchar *tmp = g_strdup_printf("%s%01d:%02d:%02d.%03d", sign.c_str(), hours,
                               minutes, seconds, mseconds);

  std::string str(tmp);

  g_free(tmp);

  return str;
}

long getMSecs(const SubtitleTime &time) {
  return 3600000 * time.hours() + 60000 * time.minutes() +
         1000 * time.seconds() + time.mseconds();
}

// Check if the string has the good format 'H:MM:SS.MS'
bool SubtitleTime::validate(const Glib::ustring &str) {
  int h, m, s, ms;

  if (sscanf(str.c_str(), "%d:%d:%d.%d", &h, &m, &s, &ms) == 4)
    return true;
  return false;
}

// Return "0:00:00.000"
Glib::ustring SubtitleTime::null() {
  return "0:00:00.000";
}

// Convert the time to a frame using a framerate
// e.g (450, 23.976)
SubtitleTime SubtitleTime::frame_to_time(const long int &frame,
                                         const float &framerate) {
  // secs = frame / framerate
  // msecs = secs * 1000

  return SubtitleTime((long)((frame / framerate) * 1000));
}

// Convert the frame to the time using a framerate
long int SubtitleTime::time_to_frame(const SubtitleTime &time,
                                     const float &framerate) {
  return (long int)round((time.totalmsecs * framerate) / 1000);
}
