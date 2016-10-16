#pragma once

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef ENABLE_NLS
#include <glib/gi18n.h>
#include <libintl.h>
#else /* NLS is disabled */
// In certain environments, some standard headers like <iomanip>
// and <locale> include libintl.h. If libintl.h is included after
// gettext macros below are defined, it causes a syntax error
// at the declaration of the gettext function in libintl.h.
//
// Fix this by including such a header before defining the macro.
//
// Note that libintl.h cannot be included directly since in the
// ENABLE_NLS=0 case it cannot be assumed that gettext is installed.
#include <locale>
#define _(String) (String)
#define N_(String) (String)
#define textdomain(String) (String)
#define gettext(String) (String)
#define ngettext(String, StringPlural, Number) \
  (((Number) == 1) ? (String) : (StringPlural))
#define dgettext(Domain, String) (String)
#define dcgettext(Domain, String, Type) (String)
#define bindtextdomain(Domain, Directory) (Domain)
#define bind_textdomain_codeset(Domain, Codeset) (Codeset)
#endif /* ENABLE_NLS */
