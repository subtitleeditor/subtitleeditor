plugin_name = configurekeyboardshortcuts
pluginlibdir = $(PACKAGE_PLUGIN_LIB_DIR)/actions
plugindescriptiondir = $(PACKAGE_PLUGIN_DESCRIPTION_DIR)/actions
actionplugindevdir = plugins/actions/$(plugin_name)
uidir = $(PACKAGE_PLUGIN_SHARE_DIR)/$(plugin_name)

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	$(SUBTITLEEDITOR_CFLAGS) \
	-DSE_PLUGIN_PATH_UI_DEV=\"$(actionplugindevdir)\" \
	-DSE_PLUGIN_PATH_UI=\"$(uidir)\"

pluginlib_LTLIBRARIES = \
	libconfigurekeyboardshortcuts.la

libconfigurekeyboardshortcuts_la_SOURCES = \
	configurekeyboardshortcuts.cc

libconfigurekeyboardshortcuts_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libconfigurekeyboardshortcuts_la_LIBADD = $(SUBTITLEEDITOR_LIBS) -L$(top_srcdir)/src -lsubtitleeditor

plugindescription_in_files = configurekeyboardshortcuts.se-plugin.in
plugindescription_DATA = $(plugindescription_in_files:.se-plugin.in=.se-plugin)

@INTLTOOL_SE_PLUGIN_RULE@

ui_DATA = dialog-configure-keyboard-shortcuts.ui

EXTRA_DIST = $(plugindescription_in_files) $(ui_DATA)

CLEANFILES = $(plugindescription_DATA) Makefile.am~ *.cc~ *.h~ *.ui~ *.uip *.uip.bak *.in~
