project('subtitleeditor', 'cpp',
  version: '0.56.2',
  meson_version: '>= 0.60',
  default_options: ['cpp_std=c++17', 'warning_level=2', 'buildtype=debugoptimized'])

gnome = import('gnome')
i18n = import('i18n')

add_project_arguments('-DHAVE_CONFIG_H', '-Wconversion', '-Werror=reorder', '-Werror=float-conversion', language: 'cpp')
if get_option('debug')
  add_project_arguments('-DDEBUG', language: 'cpp')
endif

gtkmm = dependency('gtkmm-3.0', version: '>= 3.10.0')
enchant = dependency('enchant-2', version: '>= 2.2.0', required: false)
if not enchant.found()
  enchant = dependency('enchant', version: '>= 1.4.0')
endif
libxml = dependency('libxml++-5.0', 'libxml++-4.0', 'libxml++-3.0', required: false)
libxml_new = libxml.found()
if not libxml_new
  libxml = dependency('libxml++-2.6', version: '>= 2.20.0')
endif
gstreamer = [
  dependency('gstreamer-1.0'),
  dependency('gstreamer-base-1.0'),
  dependency('gstreamer-video-1.0'),
  dependency('gstreamer-audio-1.0'),
  dependency('gstreamer-pbutils-1.0'),
]
iso_codes = dependency('iso-codes', required: false)

prefix = get_option('prefix')
share_dir = prefix / get_option('datadir') / meson.project_name()
plugin_lib_dir = prefix / get_option('libdir') / meson.project_name() / 'plugins'
plugin_share_dir = share_dir / 'plugins-share'
plugin_description_dir = share_dir / 'plugins-description'

conf = configuration_data()
conf.set_quoted('VERSION', meson.project_version())
conf.set_quoted('PACKAGE', meson.project_name())
conf.set_quoted('PACKAGE_VERSION', meson.project_version())
conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf.set('ENABLE_NLS', 1)
conf.set('HAVE_LIBXMLXX_NEW', libxml_new)
if iso_codes.found()
  conf.set('HAVE_ISO_CODES', 1)
  conf.set_quoted('ISO_CODES_PATH', iso_codes.get_variable('prefix') / 'share' / 'xml' / 'iso-codes')
endif
configure_file(output: 'config.h', configuration: conf)

root_inc = include_directories('.', 'src')
po_dir = meson.project_source_root() / 'po'

subdir('src')
subdir('plugins')
subdir('share')
subdir('po')

install_man('docs/subtitleeditor.1')
