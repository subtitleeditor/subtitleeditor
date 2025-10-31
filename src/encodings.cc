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

#include "cfg.h"
#include "encodings.h"
#include "error.h"
#include "utility.h"

static EncodingInfo encodings_info[] = {
    {"ISO-8859-1", N_("Western")},
    {"ISO-8859-2", N_("Central European")},
    {"ISO-8859-3", N_("South European")},
    {"ISO-8859-4", N_("Baltic")},
    {"ISO-8859-5", N_("Cyrillic")},
    {"ISO-8859-6", N_("Arabic")},
    {"ISO-8859-7", N_("Greek")},
    {"ISO-8859-8", N_("Hebrew Visual")},
    {"ISO-8859-8-I", N_("Hebrew")},
    {"ISO-8859-9", N_("Turkish")},
    {"ISO-8859-10", N_("Nordic")},
    {"ISO-8859-13", N_("Baltic")},
    {"ISO-8859-14", N_("Celtic")},
    {"ISO-8859-15", N_("Western")},
    {"ISO-8859-16", N_("Romanian")},

    {"UTF-8", N_("Unicode")},
    {"UTF-8-BOM", N_("Unicode (with BOM)")},
    {"UTF-7", N_("Unicode")},
    {"UTF-16", N_("Unicode")},
    {"UCS-2", N_("Unicode")},
    {"UCS-4", N_("Unicode")},

    {"ARMSCII-8", N_("Armenian")},
    {"BIG5", N_("Chinese Traditional")},
    {"BIG5-HKSCS", N_("Chinese Traditional")},
    {"CP866", N_("Cyrillic/Russian")},

    {"EUC-JP", N_("Japanese")},
    {"EUC-KR", N_("Korean")},
    {"EUC-TW", N_("Chinese Traditional")},

    {"GB18030", N_("Chinese Simplified")},
    {"GB2312", N_("Chinese Simplified")},
    {"GBK", N_("Chinese Simplified")},
    {"GEORGIAN-ACADEMY", N_("Georgian")},
    {"HZ", N_("Chinese Simplified")},

    {"IBM850", N_("Western")},
    {"IBM852", N_("Central European")},
    {"IBM855", N_("Cyrillic")},
    {"IBM857", N_("Turkish")},
    {"IBM862", N_("Hebrew")},
    {"IBM864", N_("Arabic")},

    {"ISO2022JP", N_("Japanese")},
    {"ISO2022KR", N_("Korean")},
    {"ISO-IR-111", N_("Cyrillic")},
    {"JOHAB", N_("Korean")},
    {"KOI8R", N_("Cyrillic")},
    {"KOI8U", N_("Cyrillic/Ukrainian")},

    {"SHIFT_JIS", N_("Japanese")},
    {"TCVN", N_("Vietnamese")},
    {"TIS-620", N_("Thai")},
    {"UHC", N_("Korean")},
    {"VISCII", N_("Vietnamese")},

    {"WINDOWS-1250", N_("Central European")},
    {"WINDOWS-1251", N_("Cyrillic")},
    {"WINDOWS-1252", N_("Western")},
    {"WINDOWS-1253", N_("Greek")},
    {"WINDOWS-1254", N_("Turkish")},
    {"WINDOWS-1255", N_("Hebrew")},
    {"WINDOWS-1256", N_("Arabic")},
    {"WINDOWS-1257", N_("Baltic")},
    {"WINDOWS-1258", N_("Vietnamese")},

    {NULL, NULL}};

bool Encodings::is_initialized = false;

bool Encodings::initialize() {
  if (is_initialized)
    return true;

  for (unsigned int i = 0; encodings_info[i].name != NULL; ++i) {
    encodings_info[i].name = _(encodings_info[i].name);
  }

  is_initialized = true;
  return true;
}

EncodingInfo *Encodings::get_from_charset(const Glib::ustring &charset) {
  initialize();

  for (unsigned int i = 0; encodings_info[i].name != NULL; ++i) {
    if (charset == encodings_info[i].charset)
      return &encodings_info[i];
  }
  return nullptr;
}

// Return a human readable string or empty string, ex:
// "name (charset)"
// "Unicode (UTF-8)"
Glib::ustring Encodings::get_label_from_charset(const Glib::ustring &charset) {
  EncodingInfo *info = get_from_charset(charset);
  if (info == NULL)
    return Glib::ustring();

  Glib::ustring label;

  label += info->name;
  label += " (";
  label += info->charset;
  label += ")";

  return label;
}

EncodingInfo *Encodings::get_encodings_info() {
  return encodings_info;
}

namespace Encoding {

// Trying to convert from charset to UTF-8.
// Return utf8 string or throw EncodingConvertError exception.
Glib::ustring convert_to_utf8_from_charset(const std::string &content,
                                           const Glib::ustring &charset) {
  se_dbg_msg(SE_DBG_UTILITY, "Trying to convert from %s to UTF-8",
             charset.c_str());

  // glib does not know about BOM,so we need to handle it manually
  Glib::ustring m_charset = charset;
  if (m_charset == "UTF-8-BOM")
    m_charset = "UTF-8";

  Glib::ustring utf8_content;
  // Only if it's UTF-8 to UTF-8
  if (m_charset == "UTF-8") {
    if (Glib::ustring(content).validate() == false)
      throw EncodingConvertError(_("It's not valid UTF-8."));

    utf8_content = content;
  } else {
    try {
      utf8_content = Glib::convert(content, "UTF-8", m_charset);

      if (!utf8_content.validate() || utf8_content.empty())
        throw EncodingConvertError(build_message(
            _("Couldn't convert from %s to UTF-8"), m_charset.c_str()));
    } catch (const Glib::ConvertError &ex) {
      se_dbg_msg(SE_DBG_UTILITY, "Glib::ConvertError: %s", ex.what().c_str());
      throw EncodingConvertError(build_message(
          _("Couldn't convert from %s to UTF-8"), m_charset.c_str()));
    } catch (...) {
      se_dbg_msg(SE_DBG_UTILITY, "Unknow error");
      throw EncodingConvertError(build_message(
          _("Couldn't convert from %s to UTF-8"), m_charset.c_str()));
    }
  }

  // Remove BOM (U+FEFF) if present at the beginning
  // This handles UTF-8 BOM and BOMs from other encodings after conversion
  if (!utf8_content.empty() && utf8_content[0] == 0xFEFF) {
    utf8_content.erase(0, 1);
    se_dbg_msg(SE_DBG_UTILITY, "Removed BOM from beginning of content");
  }

  return utf8_content;
}

// Trying to autodetect the charset and convert to UTF-8.
// 3 steps:
// - Try UTF-8
// - Try with user encoding preferences
// - Try with all encodings
// Return utf8 string and sets charset found
// or throw EncodingConvertError exception.
Glib::ustring convert_to_utf8(const std::string &content,
                              Glib::ustring &charset) {
  if (content.empty())
    return Glib::ustring();

  // First check if it's not UTF-8.
  se_dbg_msg(SE_DBG_UTILITY, "Trying to UTF-8...");

  try {
    Glib::ustring utf8_content =
        Encoding::convert_to_utf8_from_charset(content, "UTF-8");

    if (utf8_content.validate() && utf8_content.empty() == false) {
      charset = "UTF-8";
      return utf8_content;
    }
  } catch (const EncodingConvertError &ex) {
    se_dbg_msg(SE_DBG_UTILITY, "EncodingConvertError: %s", ex.what());
  }

  // Try to automatically detect the encoding

  // With the user charset preferences...
  se_dbg_msg(SE_DBG_UTILITY, "Trying with user encodings preferences...");

  auto user_encodings = cfg::get_string_list("encodings", "encodings");

  for (const auto &enc : user_encodings) {
    try {
      Glib::ustring utf8_content = convert_to_utf8_from_charset(content, enc);

      if (utf8_content.validate() && utf8_content.empty() == false) {
        charset = enc;
        return utf8_content;
      }
    } catch (const EncodingConvertError &ex) {
      // invalid, try with the next...
      se_dbg_msg(SE_DBG_UTILITY, "EncodingConvertError: %s", ex.what());
    }
  }

  // With all charset...
  se_dbg_msg(SE_DBG_UTILITY, "Trying with all encodings...");

  for (unsigned int i = 0; encodings_info[i].name != NULL; ++i) {
    Glib::ustring it = encodings_info[i].charset;

    try {
      Glib::ustring utf8_content =
          Encoding::convert_to_utf8_from_charset(content, it);

      if (utf8_content.validate() && utf8_content.empty() == false) {
        charset = it;
        return utf8_content;
      }
    } catch (const EncodingConvertError &ex) {
      // invalid, try with the next...
      se_dbg_msg(SE_DBG_UTILITY, "EncodingConvertError: %s", ex.what());
    }
  }

  // Failed to determine the encoding...
  throw EncodingConvertError(
      _("subtitleeditor was not able to automatically determine the encoding "
        "of the file you want to open."));
}

// Convert the UTF-8 text to the charset.
// Throw EncodingConvertError exception.
std::string convert_from_utf8_to_charset(const Glib::ustring &utf8_content,
                                         const Glib::ustring &charset) {
  se_dbg_msg(SE_DBG_UTILITY, "Trying to convert from UTF-8 to %s",
             charset.c_str());

  try {
    // glib does not know about BOM, so add it manually and then treat it as UTF-8
    if (charset == "UTF-8-BOM") {
        std::string content = Glib::convert(utf8_content, "UTF-8", "UTF-8");
        // Prepend UTF-8 BOM (EF BB BF)
        static const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
        content = std::string(reinterpret_cast<const char*>(bom), 3) + content;
        return content;
    } else {
    std::string content = Glib::convert(utf8_content, charset, "UTF-8");
    return content;
    }

  } catch (const Glib::ConvertError &ex) {
    throw EncodingConvertError(build_message(
        _("Could not convert the text to the character coding '%s'"),
        charset.c_str()));
  }
}

}  // namespace Encoding
