shared_module('adobeencoredvdntsc', ['adobeencoredvdntsc.cc'],
  cpp_args: [],
  include_directories: root_inc,
  dependencies: [gtkmm],
  link_with: libsubtitleeditor,
  install: true,
  install_dir: plugin_lib_dir / 'subtitleformats')

shared_module('adobeencoredvdpal', ['adobeencoredvdpal.cc'],
  cpp_args: [],
  include_directories: root_inc,
  dependencies: [gtkmm],
  link_with: libsubtitleeditor,
  install: true,
  install_dir: plugin_lib_dir / 'subtitleformats')

i18n.merge_file(input: 'adobeencoredvdntsc.se-plugin.desktop.in', output: 'adobeencoredvdntsc.se-plugin', type: 'desktop', po_dir: po_dir, args: ['--keyword=Description'],
  install: true, install_dir: plugin_description_dir / 'subtitleformats')

i18n.merge_file(input: 'adobeencoredvdpal.se-plugin.desktop.in', output: 'adobeencoredvdpal.se-plugin', type: 'desktop', po_dir: po_dir, args: ['--keyword=Description'],
  install: true, install_dir: plugin_description_dir / 'subtitleformats')
