shared_module('waveformmanagement', ['waveformgenerator.cc', 'waveformmanagement.cc'],
  cpp_args: [],
  include_directories: root_inc,
  dependencies: [gtkmm, gstreamer],
  link_with: libsubtitleeditor,
  install: true,
  install_dir: plugin_lib_dir / 'actions')

i18n.merge_file(input: 'waveformmanagement.se-plugin.desktop.in', output: 'waveformmanagement.se-plugin', type: 'desktop', po_dir: po_dir, args: ['--keyword=Description'],
  install: true, install_dir: plugin_description_dir / 'actions')
