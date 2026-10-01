shared_module('textcorrection', ['pattern.cc', 'patternmanager.cc', 'textcorrection.cc'],
  cpp_args: ['-DSE_PLUGIN_PATH_UI_DEV="@0@"'.format(meson.current_source_dir()), '-DSE_PLUGIN_PATH_UI="@0@"'.format(plugin_share_dir / 'textcorrection'), '-DSE_PLUGIN_PATH_PATTERN_DEV="@0@"'.format(meson.current_source_dir()), '-DSE_PLUGIN_PATH_PATTERN="@0@"'.format(plugin_share_dir / 'textcorrection')],
  include_directories: root_inc,
  dependencies: [gtkmm, libxml],
  link_with: libsubtitleeditor,
  install: true,
  install_dir: plugin_lib_dir / 'actions')

i18n.merge_file(input: 'textcorrection.se-plugin.desktop.in', output: 'textcorrection.se-plugin', type: 'desktop', po_dir: po_dir, args: ['--keyword=Description'],
  install: true, install_dir: plugin_description_dir / 'actions')

install_data(['assistant-text-correction.ui'], install_dir: plugin_share_dir / 'textcorrection')

# Translated at runtime
install_data(['Latn.capitalization.se-pattern', 'Latn.common-error.se-pattern', 'Latn-en.common-error.se-pattern', 'Latn-en.capitalization.se-pattern', 'Latn-en.hearing-impaired.se-pattern', 'Latn-en-US.common-error.se-pattern', 'Latn-fi.common-error.se-pattern', 'Latn-fr.common-error.se-pattern', 'Latn.hearing-impaired.se-pattern', 'Zyyy.common-error.se-pattern'], install_dir: plugin_share_dir / 'textcorrection')
