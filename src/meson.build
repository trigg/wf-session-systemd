sources = [
    'main.cpp',
    'session-manager.cpp',
    'component.cpp',
    'gtk-manager.cpp',
    'component-manager.cpp',
]

executable(
    'wf-session-systemd',
    sources,
    dependencies: [wf_config, libsystemd, wayland, gtkmm],
    install: true,
    install_dir: bindir_abs,
)