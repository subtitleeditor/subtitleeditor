shared_module('timedtextauthoringformat1', ['timedtextauthoringformat1.cc'],
  cpp_args: [],
  include_directories: root_inc,
  dependencies: [gtkmm, libxml],
  link_with: libsubtitleeditor,
  install: true,
  install_dir: plugin_lib_dir / 'subtitleformats')

i18n.merge_file(input: 'timedtextauthoringformat1.se-plugin.desktop.in', output: 'timedtextauthoringformat1.se-plugin', type: 'desktop', po_dir: po_dir, args: ['--keyword=Description'],
  install: true, install_dir: plugin_description_dir / 'subtitleformats')
