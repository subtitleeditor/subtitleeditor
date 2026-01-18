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
#include <sigc++/sigc++.h>

#include <map>
#include <vector>

namespace cfg {

using Glib::ustring;
using sigc::signal;
using std::vector;

// connect a signal to the group, notify when a key change
signal<void, ustring, ustring>& signal_changed(const ustring& group);

// check if a key exists on the group
bool has_key(const ustring& group, const ustring& key);

// return the keys of the group
vector<ustring> get_keys(const ustring& group);

// check if a group exists
bool has_group(const ustring& group);

// remove the group and associated keys
void remove_group(const ustring& group);

// set a comment to the key
void set_comment(const ustring& group, const ustring& key, const ustring& text);

// set the string value to the key
void set_string(const ustring& group, const ustring& key, const ustring& value);

// return a string value of the key
ustring get_string(const ustring& group, const ustring& key);

// set the string values to the key
void set_string_list(const ustring& group, const ustring& key, const vector<ustring>& values);

// return a strings value of the key
vector<ustring> get_string_list(const ustring& group, const ustring& key);

// set the boolean value to the key
void set_boolean(const ustring& group, const ustring& key, const bool& value);

// return a boolean value of the key
bool get_boolean(const ustring& group, const ustring& key);

// set the integer value to the key
void set_int(const ustring& group, const ustring& key, const int& value);

// return a integer value of the key
int get_int(const ustring& group, const ustring& key);

// set the double value to the key
void set_double(const ustring& group, const ustring& key, const double& value);

// return a double value of the key
double get_double(const ustring& group, const ustring& key);

// FIXME: remove me
// return a float value of the key
float get_float(const ustring& group, const ustring& key);

// return the default value of the key
ustring get_default(const ustring& group, const ustring& key);

}  // namespace cfg
