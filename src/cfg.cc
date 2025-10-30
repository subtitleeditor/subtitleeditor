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
#include "cfg.h"
#include "defaultcfg.h"
#include "utility.h"

namespace cfg {

// FIXME: C++17 variant, optional

using Glib::Error;
using Glib::KeyFileError;
using std::cerr;
using std::endl;
using std::map;

class Configuration {
  typedef signal<void, ustring, ustring> SignalChanged;
  typedef map<ustring, SignalChanged> SignalGroup;

 public:
  Configuration() {
  }

  ~Configuration() {
    save();
  }

  SignalGroup &signals() {
    return m_signals;
  }

  Glib::KeyFile &keyfile() {
    if (!m_keyfile_initialized) {
      load();
    }
    return m_keyfile;
  }

 protected:
  void load() {
    auto file = get_config_dir("config");
    try {
      m_keyfile.load_from_file(file);
    } catch (const Error &ex) {
      cerr << "failed to read config: " << ex.what() << endl;
    }
    load_defaults_on_missing_values();
    m_keyfile_initialized = true;
  }

  void load_defaults_on_missing_values() {
    map<ustring, map<ustring, ustring>> defaults;
    get_default_config(defaults);

    for (const auto &gkv : defaults) {
      auto group = gkv.first;
      auto has_group = m_keyfile.has_group(group);
      // for each key/value of the group
      for (const auto &kv : gkv.second) {
        // if the group or the key doesn't exist, create
        if (!has_group || !m_keyfile.has_key(group, kv.first)) {
          m_keyfile.set_string(group, kv.first, kv.second);
        }
      }
    }
  }

  void save() {
    try {
      auto filename = get_config_dir("config");
      m_keyfile.save_to_file(filename);
    } catch (const Error &ex) {
      cerr << "failed to save the configuration" << ex.what() << endl;
    }
  }

 protected:
  bool m_keyfile_initialized{false};
  Glib::KeyFile m_keyfile;
  SignalGroup m_signals;
};

Configuration &configuration() {
  static Configuration m_config;
  return m_config;
}

Glib::KeyFile &keyfile() {
  return configuration().keyfile();
}

// connect a signal to the group, notify when a key change
sigc::signal<void, ustring, ustring> &signal_changed(const ustring &group) {
  return configuration().signals()[group];
}

// emit a signal on the only to the group
void emit_signal_changed(const ustring &g, const ustring &k, const ustring &v) {
  configuration().signals()[g](k, v);
}

// check if a key exists on the group
bool has_key(const ustring &group, const ustring &key) {
  try {
    return keyfile().has_key(group, key);
  } catch (const KeyFileError &ex) {
    cerr << ex.what() << std::endl;
  }
  return false;
}

// return the keys of the group
vector<ustring> get_keys(const ustring &group) {
  return keyfile().get_keys(group);
}

// check if a group exists
bool has_group(const ustring &group) {
  return keyfile().has_group(group);
}

// remove the group and associated keys
void remove_group(const ustring &group) {
  keyfile().remove_group(group);
}

// set a comment to the key
void set_comment(const ustring &g, const ustring &k, const ustring &v) {
  keyfile().set_comment(g, k, v);
}

// set the string value to the key
void set_string(const ustring &g, const ustring &k, const ustring &v) {
  keyfile().set_string(g, k, v);
  emit_signal_changed(g, k, v);
}

// return a string value of the key
ustring get_string(const ustring &group, const ustring &key) {
  try {
    return keyfile().get_string(group, key);
  } catch (const KeyFileError &ex) {
    cerr << ex.what() << std::endl;
  }
  return ustring();
}

// set the string values to the key
void set_string_list(const ustring &g, const ustring &k,
                     const vector<ustring> &v) {
  keyfile().set_string_list(g, k, v);
  // FIXME: join strings and emit signal
  // emit_signal_changed(g, k, v);
}

// return a strings value of the key
vector<ustring> get_string_list(const ustring &group, const ustring &key) {
  try {
    return keyfile().get_string_list(group, key);
  } catch (const KeyFileError &ex) {
    cerr << ex.what() << std::endl;
  }
  return vector<ustring>();
}

// set the boolean value to the key
void set_boolean(const ustring &g, const ustring &k, const bool &v) {
  keyfile().set_boolean(g, k, v);
  emit_signal_changed(g, k, to_string(v));
}

// return a boolean value of the key
bool get_boolean(const ustring &group, const ustring &key) {
  try {
    return keyfile().get_boolean(group, key);
  } catch (const KeyFileError &ex) {
    cerr << ex.what() << std::endl;
  }
  return false;
}

// set the integer value to the key
void set_int(const ustring &g, const ustring &k, const int &v) {
  keyfile().set_integer(g, k, v);
  emit_signal_changed(g, k, to_string(v));
}

// return a integer value of the key
int get_int(const ustring &group, const ustring &key) {
  try {
    return keyfile().get_integer(group, key);
  } catch (const KeyFileError &ex) {
    cerr << ex.what() << std::endl;
  }
  return 0;
}

// set the double value to the key
void set_double(const ustring &g, const ustring &k, const double &v) {
  keyfile().set_double(g, k, v);
  emit_signal_changed(g, k, to_string(v));
}

// return a double value of the key
double get_double(const ustring &group, const ustring &key) {
  try {
    return keyfile().get_double(group, key);
  } catch (const KeyFileError &ex) {
    cerr << ex.what() << std::endl;
  }
  return 0.0;
}

// FIXME: DELETE ME
float get_float(const ustring &group, const ustring &key) {
  return static_cast<float>(get_double(group, key));
}

ustring get_default(const ustring &group, const ustring &key) {
  map<ustring, map<ustring, ustring>> defaults;
  get_default_config(defaults);
  // find the group
  auto g = defaults.find(group);
  if (g == defaults.end()) {
    return ustring();
  }
  const auto &key_values = g->second;
  // find the key
  auto k = key_values.find(key);
  if (k == key_values.end()) {
    return ustring();
  }
  return k->second;
}

}  // namespace cfg
