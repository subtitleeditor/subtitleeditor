install_data('menubar.xml', install_dir: share_dir)
install_data(files(
  'ui/dialog-character-codings.ui',
  'ui/dialog-export-text.ui',
  'ui/dialog-import-text.ui',
  'ui/dialog-open-document.ui',
  'ui/dialog-save-document.ui',
  'ui/dialog-script-properties.ui',
  'ui/subtitleeditor.ui',
), install_dir: share_dir / 'ui')
install_data('metainfo/org.kitone.subtitleeditor.appdata.xml', install_dir: get_option('datadir') / 'metainfo')
foreach size : ['16x16', '22x22', '24x24', '32x32', '64x64', '128x128', '256x256', '512x512', '1024x1024']
  install_data('icons' / size / 'subtitleeditor.png', install_dir: get_option('datadir') / 'icons' / 'hicolor' / size / 'apps')
endforeach
install_data('icons/scalable/subtitleeditor.svg', install_dir: get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps')

i18n.merge_file(input: 'org.kitone.subtitleeditor.desktop.in', output: 'org.kitone.subtitleeditor.desktop', type: 'desktop', po_dir: po_dir,
  install: true, install_dir: get_option('datadir') / 'applications')
gnome.post_install(gtk_update_icon_cache: true, update_desktop_database: true)
