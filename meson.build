project(
    'wf-session-systemd',
    'cpp',
    version: '0.1.0',
    license: 'MIT',
    default_options: ['cpp_std=c++17', 'warning_level=2', 'werror=false'],
)

wf_config = dependency('wf-config')
libsystemd = dependency('libsystemd')
wayland = dependency('wayland-client')
gtkmm = dependency('gtkmm-4.0', version: '>=4.12')

bindir_abs = get_option('prefix') / get_option('bindir')
metadata_dir = get_option('prefix') / get_option('datadir') / 'wf-session' / 'metadata'

add_project_arguments('-DMETADATA_DIR="' + metadata_dir + '"', language: 'cpp')

subdir('metadata')
subdir('data/systemd')
subdir('src')