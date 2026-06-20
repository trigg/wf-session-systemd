#pragma once

#include <string>
#include <memory>
#include <glib.h>
#include <gio/gio.h>
#include <gtkmm.h>
#include <systemd/sd-bus.h>
#include <wayfire/config/config-manager.hpp>

#include "gtk-manager.hpp"
#include "component-manager.hpp"

class SessionManager
{
  public:
    SessionManager();
    ~SessionManager();

    bool init(const std::string& custom_config_path);
    int run();
    void reload_configuration();
    void update_environments(const std::vector<std::pair<std::string, std::string>>& env_pairs);
    void harvest_env();

  private:
    std::string config_path;
    std::string instance_id = "wayland-0";
    Glib::RefPtr<Gtk::Application> app;

    std::unique_ptr<GtkManager> gtk_manager;
    std::unique_ptr<ComponentManager> component_manager;

    GFileMonitor *file_monitor = nullptr;

    static void on_file_changed(GFileMonitor *monitor, GFile *file, GFile *other_file,
        GFileMonitorEvent event_type, gpointer userdata);
};
