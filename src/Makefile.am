AUTOMAKE_OPTIONS = subdir-objects

PACKAGE_DIRECTORY = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SHARE_DIR=\""$(PACKAGE_SHARE_DIR)"\" \
	-DPACKAGE_SHARE_DIR_DEV=\""share"\" \
	-DPACKAGE_UI_DIR=\""$(PACKAGE_SHARE_DIR)/ui"\" \
	-DPACKAGE_UI_DIR_DEV=\""share/ui"\" \
	-DPACKAGE_PLUGIN_LIB_DIR=\""$(PACKAGE_PLUGIN_LIB_DIR)"\" \
	-DPACKAGE_PLUGIN_DESCRIPTION_DIR=\""$(PACKAGE_PLUGIN_DESCRIPTION_DIR)"\" \
	-DPACKAGE_PLUGIN_DESCRIPTION_DIR_DEV=\""plugins"\" \
	-DPACKAGE_PLUGIN_DIR_DEV=\""plugins"\" \
	-DDEFAULT_PLAYER_VIDEO_SINK=\""$(DEFAULT_PLAYER_VIDEO_SINK)"\" \
	-DDEFAULT_PLAYER_AUDIO_SINK=\""$(DEFAULT_PLAYER_AUDIO_SINK)"\"


LIB_CORE_FILES = \
	documents.cc \
	documents.h

## libsubtitleeditor
LIB_SUBTITLEEDITOR_FILES = \
	color.cc \
	color.h \
	command.cc \
	command.h \
	commandsystem.cc \
	commandsystem.h \
	cfg.cc \
	cfg.h \
	debug.cc \
	debug.h \
	defaultcfg.h \
	defaultcfg.cc \
	document.cc \
	document.h \
	encodings.cc \
	encodings.h \
	error.h \
	extension/action.cc \
	extension/action.h \
	extension/subtitleformat.h \
	extension.cc \
	extension.h \
	extensioninfo.cc \
	extensioninfo.h \
	extensionmanager.cc \
	extensionmanager.h \
	filereader.cc \
	filereader.h \
	filewriter.cc \
	filewriter.h \
	gtkmm_utility.cc \
	gtkmm_utility.h \
	i18n.h \
	isocodes.cc \
	isocodes.h \
	keyframes.cc \
	keyframes.h \
	player.cc \
	player.h \
	reader.cc \
	reader.h \
	scriptinfo.cc \
	scriptinfo.h \
	spellchecker.cc \
	spellchecker.h \
	style.cc \
	style.h \
	stylemodel.cc \
	stylemodel.h \
	styles.cc \
	styles.h \
	subtitle.cc \
	subtitleeditorwindow.cc \
	subtitleeditorwindow.h \
	subtitleformatio.cc \
	subtitleformatio.h \
	subtitleformatsystem.cc \
	subtitleformatsystem.h \
	subtitle.h \
	subtitlemodel.cc \
	subtitlemodel.h \
	subtitles.cc \
	subtitles.h \
	subtitletime.cc \
	subtitletime.h \
	subtitleview.cc \
	subtitleview.h \
	timeutility.cc \
	timeutility.h \
	utility.cc \
	utility.h \
	waveform.cc \
	waveform.h \
	waveformmanager.h \
	widget_config_utility.cc \
	widget_config_utility.h \
	writer.cc \
	writer.h

LIB_GUI_FILES = \
	gui/automaticspellchecker.cc \
	gui/automaticspellchecker.h \
	gui/cellrenderercustom.cc \
	gui/cellrenderercustom.h \
	gui/comboboxencoding.cc \
	gui/comboboxencoding.h \
	gui/comboboxframerate.cc \
	gui/comboboxframerate.h \
	gui/comboboxnewline.cc \
	gui/comboboxnewline.h \
	gui/comboboxsubtitleformat.cc \
	gui/comboboxsubtitleformat.h \
	gui/comboboxtextcolumns.h \
	gui/comboboxvideo.cc \
	gui/comboboxvideo.h \
	gui/dialogcharactercodings.cc \
	gui/dialogcharactercodings.h \
	gui/dialogfilechooser.cc \
	gui/dialogfilechooser.h \
	gui/dialogutility.cc \
	gui/dialogutility.h \
	gui/spinbuttontime.cc \
	gui/spinbuttontime.h \
	gui/textviewcell.cc \
	gui/textviewcell.h \
	gui/treeviewextensionmanager.cc \
	gui/treeviewextensionmanager.h

lib_LTLIBRARIES = libsubtitleeditor.la

libsubtitleeditor_la_LDFLAGS = -export-dynamic -no-undefined

libsubtitleeditor_la_SOURCES = \
	$(LIB_SUBTITLEEDITOR_FILES) \
	$(LIB_CORE_FILES) \
	$(LIB_GUI_FILES)

libsubtitleeditor_la_LIBADD = \
	$(GTKMM_LIBS) \
	$(LIBXML_LIBS) \
	$(ENCHANT_LIBS)

libsubtitleeditor_la_CXXFLAGS = \
	$(GTKMM_CFLAGS) \
	$(LIBXML_CFLAGS) \
	$(ENCHANT_CFLAGS) \
	$(PACKAGE_DIRECTORY)


## subtitleeditor
APPLICATION_FILES = \
	gui/application.cc \
	gui/application.h \
	gui/menubar.cc \
	gui/menubar.h \
	gui/statusbar.cc \
	gui/statusbar.h \
	gstreamer_utility.cc \
	gstreamer_utility.h \
	main.cc \
	options.cc \
	options.h \
	vp/gstplayer.cc \
	vp/gstplayer.h \
	vp/videoplayer.cc \
	vp/videoplayer.h \
	we/waveformeditor.cc \
	we/waveformeditor.h \
	we/waveformrenderercairo.cc \
	we/waveformrenderer.cc \
	we/waveformrenderergl.cc \
	we/waveformrenderer.h

bin_PROGRAMS = subtitleeditor

subtitleeditor_SOURCES = \
	$(APPLICATION_FILES)

subtitleeditor_LDADD = \
	$(GTKMM_LIBS) \
	$(GSTREAMER_LIBS) \
	$(GTKGLEXT_LIBS) \
	$(GL_LIBS) \
	$(LIBXML_LIBS) \
	libsubtitleeditor.la

subtitleeditor_CXXFLAGS = \
	$(GTKMM_CFLAGS) \
	$(GSTREAMER_CFLAGS) \
	$(LIBXML_CFLAGS) \
	$(GTKGLEXT_CFLAGS) \
	$(GL_CFLAGS) \
	$(PACKAGE_DIRECTORY)


CLEANFILES = Makefile.am~ *.cc~ *.h~ *.in~
