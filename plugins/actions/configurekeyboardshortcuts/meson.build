shared_module('configurekeyboardshortcuts', ['configurekeyboardshortcuts.cc'],
  cpp_args: ['-DSE_PLUGIN_PATH_UI_DEV="@0@"'.format(meson.current_source_dir()), '-DSE_PLUGIN_PATH_UI="@0@"'.format(plugin_share_dir / 'configurekeyboardshortcuts')],
  include_directories: root_inc,
  dependencies: [gtkmm],
  link_with: libsubtitleeditor,
  install: true,
  install_dir: plugin_lib_dir / 'actions')

i18n.merge_file(input: 'configurekeyboardshortcuts.se-plugin.desktop.in', output: 'configurekeyboardshortcuts.se-plugin', type: 'desktop', po_dir: po_dir, args: ['--keyword=Description'],
  install: true, install_dir: plugin_description_dir / 'actions')

install_data(['dialog-configure-keyboard-shortcuts.ui'], install_dir: plugin_share_dir / 'configurekeyboardshortcuts')
