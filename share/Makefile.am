SUBDIRS = metainfo icons ui

uidir = $(PACKAGE_SHARE_DIR)
ui_DATA = menubar.xml

desktopdir = $(datadir)/applications
desktop_in_files = org.kitone.subtitleeditor.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

EXTRA_DIST = $(ui_DATA) $(desktop_in_files)

CLEANFILES = Makefile.am~ *.c~ *.h~ *.ui~ *.uip *.uip.bak $(desktop_DATA)
