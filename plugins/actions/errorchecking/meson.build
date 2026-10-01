shared_module('errorchecking', ['errorcheckingplugin.cc'],
  cpp_args: ['-DSE_PLUGIN_PATH_UI_DEV="@0@"'.format(meson.current_source_dir()), '-DSE_PLUGIN_PATH_UI="@0@"'.format(plugin_share_dir / 'errorchecking')],
  include_directories: root_inc,
  dependencies: [gtkmm],
  link_with: libsubtitleeditor,
  install: true,
  install_dir: plugin_lib_dir / 'actions')

i18n.merge_file(input: 'errorchecking.se-plugin.desktop.in', output: 'errorchecking.se-plugin', type: 'desktop', po_dir: po_dir, args: ['--keyword=Description'],
  install: true, install_dir: plugin_description_dir / 'actions')

install_data(['dialog-error-checking.ui', 'dialog-error-checking-preferences.ui'], install_dir: plugin_share_dir / 'errorchecking')
