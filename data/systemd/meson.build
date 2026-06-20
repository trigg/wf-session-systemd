systemd_user_unit_dir = libsystemd.get_variable(
    pkgconfig: 'systemduserunitdir',
    default_value: get_option('prefix') / 'lib' / 'systemd' / 'user',
)

conf_data = configuration_data()
conf_data.set('bindir', bindir_abs)

configure_file(
    input: 'wf-session.service.in',
    output: 'wf-session.service',
    configuration: conf_data,
    install_dir: systemd_user_unit_dir,
    install: true,
)

configure_file(
    input: 'wf-session@.target.in',
    output: 'wf-session@.target',
    configuration: conf_data,
    install_dir: systemd_user_unit_dir,
    install: true,
)