shared_module('insertsubtitle', ['insertsubtitle.cc'],
  cpp_args: [],
  include_directories: root_inc,
  dependencies: [gtkmm],
  link_with: libsubtitleeditor,
  install: true,
  install_dir: plugin_lib_dir / 'actions')

i18n.merge_file(input: 'insertsubtitle.se-plugin.desktop.in', output: 'insertsubtitle.se-plugin', type: 'desktop', po_dir: po_dir, args: ['--keyword=Description'],
  install: true, install_dir: plugin_description_dir / 'actions')
