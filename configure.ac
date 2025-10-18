#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# AC_PREREQ(2.59)
AC_INIT(subtitleeditor, 0.55.0)
AM_INIT_AUTOMAKE([tar-ustar])

AC_CONFIG_SRCDIR([Makefile.am])
AM_CONFIG_HEADER([config.h])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE

# Checks for programs.
AC_LANG([C++])
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX([14], [noext], [mandatory])

# Checks for library functions.
AC_FUNC_SELECT_ARGTYPES
AC_CHECK_FUNCS([select])

# make it autoconf-2.59a<=compatible
AC_CHECK_PROG(EGREP, egrep, egrep) # AC_PROG_EGREP
AC_CHECK_PROG(SED, sed, sed) # AC_PROG_SED

AC_CONFIG_MACRO_DIR([m4])
AC_SUBST([ACLOCAL_AMFLAGS], ["-I $ac_macro_dir \${ACLOCAL_FLAGS}"])

# =========================================================================
# libtool

AC_DISABLE_STATIC
AC_PROG_LIBTOOL
PLUGIN_LIBTOOL_FLAGS="-module -avoid-version"
AC_SUBST(PLUGIN_LIBTOOL_FLAGS)

# =========================================================================
# localization

GETTEXT_PACKAGE=$PACKAGE_NAME
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],["$GETTEXT_PACKAGE"],[Package name for gettext])

AC_ARG_ENABLE(nls, , [
if test "A$enableval" = "Ayes"; then
	AM_GLIB_GNU_GETTEXT
else
	USE_NLS="no"
fi
], [
	AM_GLIB_GNU_GETTEXT
])

IT_PROG_INTLTOOL
# extend intltool's macros
IT_SE_PLUGIN_INTLTOOL
IT_SE_PATTERN_INTLTOOL

# =========================================================================
# check gtkmm

PKG_CHECK_MODULES(GTKMM, gtkmm-3.0 >= 3.10.0) # glibmm-2.4 >= 2.16.3)

AC_SUBST(GTKMM_CFLAGS)
AC_SUBST(GTKMM_LIBS)

# =========================================================================
# check enchant

PKG_CHECK_MODULES(ENCHANT2, enchant-2 >= 2.2.0, have_enchant_2=yes, have_enchant_2=no)

if test "x$have_enchant_2" = "xyes"; then
	ENCHANT_CFLAGS=$ENCHANT2_CFLAGS
	ENCHANT_LIBS=$ENCHANT2_LIBS
	AC_SUBST(ENCHANT_CFLAGS)
	AC_SUBST(ENCHANT_LIBS)

else
	PKG_CHECK_MODULES(ENCHANT, enchant >= 1.4.0)
fi

# =========================================================================
# check libxml++

PKG_CHECK_MODULES(LIBXML, libxml++-2.6 >= 2.20)
AC_SUBST(LIBXML_CFLAGS)
AC_SUBST(LIBXML_LIBS)

# =========================================================================
# check gstreamer

PKG_CHECK_MODULES(
					GSTREAMER,
					gstreamer-1.0 gstreamer-base-1.0 gstreamer-plugins-base-1.0,,
					[echo "***"
					 echo "*** Error! you need to have : "
					 echo "*** gstreamer-1.0"
					 echo "*** gstreamer-base-1.0"
					 echo "*** gstreamer-plugins-base-1.0"
					 echo "***"
					 exit -1])

AC_SUBST(GSTREAMER_CFLAGS)
AC_SUBST(GSTREAMER_LIBS)

GSTREAMER_LIBS="$GSTREAMER_LIBS -lgstvideo-1.0 -lgstaudio-1.0 -lgstpbutils-1.0"

#SE_GST_ELEMENT_CHECK_REQUIRED([0.10], [level], [gstreamer0.10-plugins-good])

# =========================================================================
# check iso-codes 639, 3166 and 15924

have_iso_code_639=no
have_iso_code_3166=no
have_iso_code_15924=no

AC_MSG_CHECKING([whether iso-codes are available on this system])

if $PKG_CONFIG iso-codes; then
	AC_MSG_RESULT([yes])

	# iso-639
	AC_MSG_CHECKING([whether iso-codes has iso-639 domain])
	if $PKG_CONFIG --variable=domains iso-codes | grep -q 639; then
		AC_MSG_RESULT([yes])
		have_iso_code_639=yes
	else
		AC_MSG_RESULT([no])
	fi

	# iso-3166
	AC_MSG_CHECKING([whether iso-codes has iso-3166 domain])
	if $PKG_CONFIG --variable=domains iso-codes | grep -q 3166; then
		AC_MSG_RESULT([yes])
		have_iso_code_3166=yes
	else
		AC_MSG_RESULT([no])
	fi

	# iso-15924
	AC_MSG_CHECKING([whether iso-codes has iso-15924 domain])
	if $PKG_CONFIG --variable=domains iso-codes | grep -q 15924; then
		AC_MSG_RESULT([yes])
		have_iso_code_15924=yes
	else
		AC_MSG_RESULT([no])
	fi

else
	AC_MSG_RESULT([no])
fi

if test "$have_iso_code_639" == "yes" || test "$have_iso_code_3166" == "yes" || test "$have_iso_code_15924" == "yes" ; then
	AC_DEFINE(HAVE_ISO_CODES, 1, [Define to 1 if iso-codes is supported.])

	ISO_CODES_PREFIX=`$PKG_CONFIG --variable=prefix iso-codes`
	ISO_CODES_PATH="$ISO_CODES_PREFIX/share/xml/iso-codes"

	AC_SUBST([ISO_CODES_PATH])
	AC_DEFINE_UNQUOTED([ISO_CODES_PATH], ["$ISO_CODES_PATH"], [iso-codes path for files])
fi

# =========================================================================
# check opengl & gtkglextmm only if the option gl is enabled (default no)

AC_ARG_ENABLE(gl,
							[AC_HELP_STRING([--enable-gl],
							[OpenGL waveform renderer [default=no]])],,
							[enable_gl=no])

if test "$enable_gl" = "yes"; then
	AX_CHECK_GL
	CXXFLAGS="-DENABLE_GL $CXXFLAGS"

	PKG_CHECK_MODULES(GTKGLEXT, gtkglextmm-1.2)

	AC_SUBST(GTKGLEXT_CFLAGS)
	AC_SUBST(GTKGLEXT_LIBS)
fi

# =========================================================================
# Set default video player output

AC_ARG_WITH(default-video-sink,
	[AC_HELP_STRING([--with-default-video-sink=@<:@autovideosink/xvimagesink/ximagesink/sdlvideosink/gconfvideosink/glimagesink/osxvideosink@:>@],
		[change default video output preference])],,)
AC_MSG_CHECKING([whether to change the video output preference])
if test "x$with_default_video_sink" = "x"; then
	DEFAULT_PLAYER_VIDEO_SINK="xvimagesink"
	AC_MSG_RESULT([no, $DEFAULT_PLAYER_VIDEO_SINK])
else
	# check against available sinks
	for sink in autovideosink xvimagesink ximagesink sdlvideosink gconfvideosink glimagesink osxvideosink;
	do
		if test "x$with_default_video_sink" = "x$sink"; then
			DEFAULT_PLAYER_VIDEO_SINK="$with_default_video_sink"
			break
		fi
	done
	AC_MSG_RESULT([yes, $with_default_video_sink])
	if test "x$DEFAULT_PLAYER_VIDEO_SINK" = "x"; then
		AC_MSG_ERROR([unknown sink $with_default_video_sink])
	fi
fi
AC_SUBST(DEFAULT_PLAYER_VIDEO_SINK)

AC_ARG_WITH(default-audio-sink,
	[AC_HELP_STRING([--with-default-audio-sink=@<:@autoaudiosink/pulsesink/alsasink/esdsink/osssink/sdlaudiosink/gconfaudiosink/osxaudiosink@:>@],
		[change default audio output preference])],,)
AC_MSG_CHECKING([whether to change the audio output preference])
if test "x$with_default_audio_sink" = "x"; then
	DEFAULT_PLAYER_AUDIO_SINK="autoaudiosink"
	AC_MSG_RESULT([no, $DEFAULT_PLAYER_AUDIO_SINK])
else
	# check against available sinks
	for sink in autoaudiosink pulsesink alsasink esdsink osssink sdlaudiosink gconfaudiosink osxaudiosink;
	do
		if test "x$with_default_audio_sink" = "x$sink"; then
			DEFAULT_PLAYER_AUDIO_SINK="$with_default_audio_sink"
			break
		fi
	done
	AC_MSG_RESULT([yes, $with_default_audio_sink])
	if test "x$DEFAULT_PLAYER_AUDIO_SINK" = "x"; then
		AC_MSG_ERROR([unknown sink $with_default_audio_sink])
	fi

fi
AC_SUBST(DEFAULT_PLAYER_AUDIO_SINK)

# =========================================================================
# enable OSX options (default no)

AC_ARG_ENABLE(osx,
							[AC_HELP_STRING([--enable-osx], [enable OSX support [default=no]])],,
							[enable_osx=no])

if test "$enable_osx" = "yes"; then
  CXXFLAGS="$CXXFLAGS -DUSE_OSX"
fi

# =========================================================================
# debug options (default yes)

AC_ARG_ENABLE(debug,
							[AC_HELP_STRING([--enable-debug], [turn on debugging [default=yes]])],,
							[enable_debug=yes])

if test "$enable_debug" = "yes"; then
  CXXFLAGS="$CXXFLAGS -DDEBUG -g"
  LDFLAGS="$LDFLAGS -g -Wall"
fi

# =========================================================================
# profile options (default no)

AC_ARG_ENABLE(profile,
							[AC_HELP_STRING([--enable-profile], [turn on profiling [default=no]])],,
							[enable_profile=no])

if test "$enable_profile" = "yes"; then
  CXXFLAGS="$CXXFLAGS -pg"
  LDFLAGS="$LDFLAGS -pg -Wall"
fi

# =========================================================================
# gcc flags

CXXFLAGS="$CXXFLAGS -Wall -Wextra -Wconversion -Wreorder -Werror=reorder -Werror=float-conversion" #-Werror

# =========================================================================
# make use of ccache
ACX_CCACHE

# =========================================================================
# Set SUBTITLEEDITOR_LIBS and SUBTITLEEDITOR_CFLAGS

SUBTITLEEDITOR_LIBS="$GTKMM_LIBS"
SUBTITLEEDITOR_CFLAGS="$GTKMM_CFLAGS $INTLCFLAGS"

AC_SUBST(SUBTITLEEDITOR_LIBS)
AC_SUBST(SUBTITLEEDITOR_CFLAGS)

# =========================================================================
# Set package directory VAR

PACKAGE_SHARE_DIR="${datadir}/${PACKAGE_NAME}"
AC_SUBST(PACKAGE_SHARE_DIR)

PACKAGE_PLUGIN_LIB_DIR="${libdir}/${PACKAGE_NAME}/plugins"
AC_SUBST(PACKAGE_PLUGIN_LIB_DIR)

PACKAGE_PLUGIN_SHARE_DIR="${PACKAGE_SHARE_DIR}/plugins-share"
AC_SUBST(PACKAGE_PLUGIN_SHARE_DIR)

PACKAGE_PLUGIN_DESCRIPTION_DIR="${PACKAGE_SHARE_DIR}/plugins-description"
AC_SUBST(PACKAGE_PLUGIN_DESCRIPTION_DIR)

# =========================================================================
# Config Makefile

AC_CONFIG_FILES([
Makefile
src/Makefile
m4/Makefile
share/Makefile
share/metainfo/Makefile
share/icons/Makefile
share/icons/16x16/Makefile
share/icons/22x22/Makefile
share/icons/24x24/Makefile
share/icons/32x32/Makefile
share/icons/64x64/Makefile
share/icons/128x128/Makefile
share/icons/256x256/Makefile
share/icons/512x512/Makefile
share/icons/1024x1024/Makefile
share/icons/scalable/Makefile
share/ui/Makefile
docs/Makefile
po/Makefile.in
plugins/Makefile
plugins/actions/Makefile
plugins/actions/about/Makefile
plugins/actions/adjusttime/Makefile
plugins/actions/applytranslation/Makefile
plugins/actions/bestfit/Makefile
plugins/actions/changeframerate/Makefile
plugins/actions/clipboard/Makefile
plugins/actions/combinesubtitles/Makefile
plugins/actions/command/Makefile
plugins/actions/configurekeyboardshortcuts/Makefile
plugins/actions/dialoguize/Makefile
plugins/actions/documentmanagement/Makefile
plugins/actions/documentsnavigation/Makefile
plugins/actions/duplicatesubtitle/Makefile
plugins/actions/editcell/Makefile
plugins/actions/errorchecking/Makefile
plugins/actions/extendlength/Makefile
plugins/actions/externalvideoplayer/Makefile
plugins/actions/findandreplace/Makefile
plugins/actions/findbytime/Makefile
plugins/actions/insertsubtitle/Makefile
plugins/actions/insertsubtitlefromkeyframe/Makefile
plugins/actions/italicize/Makefile
plugins/actions/joindocument/Makefile
plugins/actions/keyframesmanagement/Makefile
plugins/actions/minimizeduration/Makefile
plugins/actions/moveafterprecedingsubtitle/Makefile
plugins/actions/movesubtitles/Makefile
plugins/actions/plaintext/Makefile
plugins/actions/preferences/Makefile
plugins/actions/removesubtitle/Makefile
plugins/actions/reversetextandtranslation/Makefile
plugins/actions/scalesubtitles/Makefile
plugins/actions/selection/Makefile
plugins/actions/sortsubtitles/Makefile
plugins/actions/spellchecking/Makefile
plugins/actions/slidetimings/Makefile
plugins/actions/slidewords/Makefile
plugins/actions/splitdocument/Makefile
plugins/actions/splitsubtitle/Makefile
plugins/actions/stacksubtitles/Makefile
plugins/actions/styleeditor/Makefile
plugins/actions/stylize/Makefile
plugins/actions/template/Makefile
plugins/actions/textcorrection/Makefile
plugins/actions/timemodemanagement/Makefile
plugins/actions/timingfromplayer/Makefile
plugins/actions/typewriter/Makefile
plugins/actions/videoplayermanagement/Makefile
plugins/actions/viewmanager/Makefile
plugins/actions/waveformmanagement/Makefile
plugins/subtitleformats/Makefile
plugins/subtitleformats/adobeencoredvd/Makefile
plugins/subtitleformats/advancedsubstationalpha/Makefile
plugins/subtitleformats/avidds/Makefile
plugins/subtitleformats/bitc/Makefile
plugins/subtitleformats/dcsubtitle/Makefile
plugins/subtitleformats/microdvd/Makefile
plugins/subtitleformats/mpl2/Makefile
plugins/subtitleformats/mpsub/Makefile
plugins/subtitleformats/plaintextformat/Makefile
plugins/subtitleformats/sami/Makefile
plugins/subtitleformats/sbv/Makefile
plugins/subtitleformats/sprucestl/Makefile
plugins/subtitleformats/subrip/Makefile
plugins/subtitleformats/substationalpha/Makefile
plugins/subtitleformats/subtitleeditorproject/Makefile
plugins/subtitleformats/subviewer2/Makefile
plugins/subtitleformats/timedtextauthoringformat1/Makefile
])


AC_OUTPUT

AC_MSG_NOTICE([

$PACKAGE_NAME $PACKAGE_VERSION

prefix ............................... : ${prefix}
gcc version .......................... : $(gcc -dumpversion)
gtk+ version ......................... : $(pkg-config --modversion gtk+-3.0)
gtkmm version ........................ : $(pkg-config --modversion gtkmm-3.0)
gstreamer version .................... : $(pkg-config --modversion gstreamer-1.0)

default video output ................. : $DEFAULT_PLAYER_VIDEO_SINK
default audio output ................. : $DEFAULT_PLAYER_AUDIO_SINK

have iso-codes 639, 3166 and 15924 ... : $have_iso_code_639, $have_iso_code_3166, $have_iso_code_15924
OpenGL waveform renderer ............. : $enable_gl
OSX support .......................... : $enable_osx
debug support ........................ : $enable_debug
ccache support ....................... : ${enable_ccache:-no}


Configuration complete. now run make
])
