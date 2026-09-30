shared_module('subtitleeditorproject', ['subtitleeditorproject.cc'],
  cpp_args: [],
  include_directories: root_inc,
  dependencies: [gtkmm, libxml],
  link_with: libsubtitleeditor,
  install: true,
  install_dir: plugin_lib_dir / 'subtitleformats')

i18n.merge_file(input: 'subtitleeditorproject.se-plugin.desktop.in', output: 'subtitleeditorproject.se-plugin', type: 'desktop', po_dir: po_dir, args: ['--keyword=Description'],
  install: true, install_dir: plugin_description_dir / 'subtitleformats')
