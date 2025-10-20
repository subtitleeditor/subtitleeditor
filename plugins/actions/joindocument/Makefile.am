plugin_name = joindocument
pluginlibdir = $(PACKAGE_PLUGIN_LIB_DIR)/actions
plugindescriptiondir = $(PACKAGE_PLUGIN_DESCRIPTION_DIR)/actions
actionplugindevdir = plugins/actions/$(plugin_name)
uidir = $(PACKAGE_PLUGIN_SHARE_DIR)/$(plugin_name)

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	$(SUBTITLEEDITOR_CFLAGS) \
	-DSE_PLUGIN_PATH_DEV=\"$(actionplugindevdir)\" \
	-DSE_PLUGIN_PATH_UI=\"$(uidir)\"

pluginlib_LTLIBRARIES = \
	libjoindocument.la

libjoindocument_la_SOURCES = \
	joindocument.cc

libjoindocument_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libjoindocument_la_LIBADD = $(SUBTITLEEDITOR_LIBS) -L$(top_srcdir)/src -lsubtitleeditor

plugindescription_in_files = joindocument.se-plugin.in
plugindescription_DATA = $(plugindescription_in_files:.se-plugin.in=.se-plugin)

@INTLTOOL_SE_PLUGIN_RULE@

ui_DATA = dialog-join-offset.ui

EXTRA_DIST = $(plugindescription_in_files) $(ui_DATA)

CLEANFILES = $(plugindescription_DATA) Makefile.am~ *.cc~ *.h~ *.in~
