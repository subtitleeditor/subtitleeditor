plugin_name = substationalpha
pluginlibdir = $(PACKAGE_PLUGIN_LIB_DIR)/subtitleformats
plugindescriptiondir = $(PACKAGE_PLUGIN_DESCRIPTION_DIR)/subtitleformats
subtitleformatplugindevdir = plugins/subtitleformats/$(plugin_name)
uidir = $(PACKAGE_PLUGIN_SHARE_DIR)/$(plugin_name)

pluginlib_LTLIBRARIES = \
	libsubstationalpha.la

libsubstationalpha_la_CXXFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	$(SUBTITLEEDITOR_CFLAGS) \
	-DSE_PLUGIN_PATH_UI_DEV=\"$(subtitleformatplugindevdir)\" \
	-DSE_PLUGIN_PATH_UI=\"$(uidir)\"

libsubstationalpha_la_SOURCES = \
	substationalpha.cc

libsubstationalpha_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libsubstationalpha_la_LIBADD = $(SUBTITLEEDITOR_LIBS) -L$(top_srcdir)/src -lsubtitleeditor

plugindescription_in_files = substationalpha.se-plugin.in
plugindescription_DATA = $(plugindescription_in_files:.se-plugin.in=.se-plugin)

@INTLTOOL_SE_PLUGIN_RULE@

ui_DATA = dialog-substationalpha-preferences.ui

EXTRA_DIST = $(plugindescription_in_files) $(ui_DATA)

CLEANFILES = $(plugindescription_DATA) Makefile.am~ *.cc~ *.h~ *.in~
