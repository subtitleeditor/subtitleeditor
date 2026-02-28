plugin_name = textcorrection
pluginlibdir = $(PACKAGE_PLUGIN_LIB_DIR)/actions
plugindescriptiondir = $(PACKAGE_PLUGIN_DESCRIPTION_DIR)/actions
patterndir = $(PACKAGE_PLUGIN_SHARE_DIR)/$(plugin_name)
actionplugindevdir = plugins/actions/$(plugin_name)
uidir = $(PACKAGE_PLUGIN_SHARE_DIR)/$(plugin_name)

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	$(LIBXML_CFLAGS) \
	$(SUBTITLEEDITOR_CFLAGS) \
	-DSE_PLUGIN_PATH_UI_DEV=\"$(actionplugindevdir)\" \
	-DSE_PLUGIN_PATH_UI=\"$(uidir)\" \
	-DSE_PLUGIN_PATH_PATTERN_DEV=\"$(actionplugindevdir)\" \
	-DSE_PLUGIN_PATH_PATTERN=\"$(patterndir)\"

pluginlib_LTLIBRARIES = \
	libtextcorrection.la

libtextcorrection_la_SOURCES = \
	capitalizationpage.h \
	commonerrorpage.h \
	confirmationpage.h \
	hearingimpairedpage.h \
	page.h \
	pattern.cc \
	pattern.h \
	patternmanager.cc \
	patternmanager.h \
	patternspage.h \
	taskspage.h \
	textcorrection.cc

libtextcorrection_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libtextcorrection_la_LIBADD = $(SUBTITLEEDITOR_LIBS) $(LIBXML_LIBS) -L$(top_srcdir)/src -lsubtitleeditor

plugindescription_in_files = textcorrection.se-plugin.in
plugindescription_DATA = $(plugindescription_in_files:.se-plugin.in=.se-plugin)

@INTLTOOL_SE_PLUGIN_RULE@

pattern_in_files = \
	Latn.capitalization.se-pattern.in \
	Latn.common-error.se-pattern.in \
	Latn-en.common-error.se-pattern.in \
	Latn-en.capitalization.se-pattern.in \
	Latn-en.hearing-impaired.se-pattern.in \
	Latn-en-US.common-error.se-pattern.in \
	Latn-fi.common-error.se-pattern.in \
	Latn-fr.common-error.se-pattern.in \
	Latn.hearing-impaired.se-pattern.in \
	Zyyy.common-error.se-pattern.in

pattern_DATA = $(pattern_in_files:.se-pattern.in=.se-pattern)
@INTLTOOL_SE_PATTERN_RULE@

ui_DATA = assistant-text-correction.ui

EXTRA_DIST = $(plugindescription_in_files) $(pattern_in_files) $(ui_DATA)

CLEANFILES = $(plugindescription_DATA) $(pattern_DATA) Makefile.am~ *.cc~ *.h~ *.ui~ *.uip *.uip.bak *.in~
