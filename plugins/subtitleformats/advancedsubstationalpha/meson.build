shared_module('advancedsubstationalpha', ['advancedsubstationalpha.cc'],
  cpp_args: ['-DSE_PLUGIN_PATH_UI_DEV="@0@"'.format(meson.current_source_dir()), '-DSE_PLUGIN_PATH_UI="@0@"'.format(plugin_share_dir / 'advancedsubstationalpha')],
  include_directories: root_inc,
  dependencies: [gtkmm],
  link_with: libsubtitleeditor,
  install: true,
  install_dir: plugin_lib_dir / 'subtitleformats')

i18n.merge_file(input: 'advancedsubstationalpha.se-plugin.desktop.in', output: 'advancedsubstationalpha.se-plugin', type: 'desktop', po_dir: po_dir, args: ['--keyword=Description'],
  install: true, install_dir: plugin_description_dir / 'subtitleformats')

install_data(['dialog-advancedsubstationalpha-preferences.ui'], install_dir: plugin_share_dir / 'advancedsubstationalpha')
