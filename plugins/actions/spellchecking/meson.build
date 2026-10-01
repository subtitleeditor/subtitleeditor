shared_module('spellchecking', ['spellchecking.cc'],
  cpp_args: ['-DSE_PLUGIN_PATH_UI_DEV="@0@"'.format(meson.current_source_dir()), '-DSE_PLUGIN_PATH_UI="@0@"'.format(plugin_share_dir / 'spellchecking')],
  include_directories: root_inc,
  dependencies: [gtkmm, enchant],
  link_with: libsubtitleeditor,
  install: true,
  install_dir: plugin_lib_dir / 'actions')

i18n.merge_file(input: 'spellchecking.se-plugin.desktop.in', output: 'spellchecking.se-plugin', type: 'desktop', po_dir: po_dir, args: ['--keyword=Description'],
  install: true, install_dir: plugin_description_dir / 'actions')

install_data(['dialog-spell-checking.ui'], install_dir: plugin_share_dir / 'spellchecking')
