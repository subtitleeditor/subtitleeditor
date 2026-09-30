shared_module('insertsubtitlefromkeyframe', ['insertsubtitlefromkeyframe.cc'],
  cpp_args: [],
  include_directories: root_inc,
  dependencies: [gtkmm],
  link_with: libsubtitleeditor,
  install: true,
  install_dir: plugin_lib_dir / 'actions')

i18n.merge_file(input: 'insertsubtitlefromkeyframe.se-plugin.desktop.in', output: 'insertsubtitlefromkeyframe.se-plugin', type: 'desktop', po_dir: po_dir, args: ['--keyword=Description'],
  install: true, install_dir: plugin_description_dir / 'actions')
