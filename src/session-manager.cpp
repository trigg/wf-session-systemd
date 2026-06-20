#include "session-manager.hpp"
#include "config-manager.hpp"
#include "glib.h"
#include <iostream>
#include <utility>
#include <vector>
#include <filesystem>
#include <fcntl.h>
#include <memory>
#include <unistd.h>
#include <sys/inotify.h>
#include <glib-unix.h>
#include <wayfire/config/file.hpp>
#include <gtk/gtk.h>
/*
 *   Session Manager
 *
 *   Reads in .config/wf-session/systemd.ini (or similar) and sets out the users session as required.
 *
 *   See example.ini for possibilities
 */

SessionManager::SessionManager()
{
    const char *env_wl_display = std::getenv("WAYLAND_DISPLAY");
    if (env_wl_display)
    {
        instance_id = std::string(env_wl_display);
    } else
    {
        std::cerr << "No WAYLAND_DISPLAY. Bailing" << std::endl;
        std::exit(0);
    }

    app = Gtk::Application::create("org.wayfire.session.systemd." + instance_id,
        Gio::Application::Flags::DEFAULT_FLAGS);
}

int SessionManager::run()
{
    std::cout << "Running" << std::endl;
    app->hold();
    return app->run(0, nullptr);
}

SessionManager::~SessionManager()
{
    if (file_monitor)
    {
        g_file_monitor_cancel(file_monitor);
        g_object_unref(file_monitor);
    }
}

bool SessionManager::init(const std::string& custom_config_path)
{
    config_path = custom_config_path;
    if (config_path.empty())
    {
        const char *xdg_config = std::getenv("XDG_CONFIG_HOME");
        config_path = xdg_config ? std::string(xdg_config) + "/wf-session/systemd.ini" :
            std::string(std::getenv("HOME")) + "/.config/wf-session/systemd.ini";
    }

    std::filesystem::path full_config_path(config_path);
    std::filesystem::path parent_dir = full_config_path.parent_path();
    try {
        std::filesystem::create_directories(parent_dir);
    } catch (...)
    {
        return false;
    }

    harvest_env();

    std::vector<std::string> xml_directories = {METADATA_DIR};
    if (std::filesystem::exists("metadata"))
    {
        xml_directories.push_back("metadata");
    }

    ConfigManager::load(xml_directories, config_path);

    gtk_manager = std::make_unique<GtkManager>();
    component_manager = std::make_unique<ComponentManager>(instance_id);

    std::cout << "Watching directory : " << parent_dir << std::endl;
    GFile *dir_to_watch = g_file_new_for_path(parent_dir.c_str());
    if (dir_to_watch)
    {
        file_monitor = g_file_monitor_directory(dir_to_watch, G_FILE_MONITOR_NONE, nullptr, nullptr);

        if (file_monitor)
        {
            g_signal_connect(file_monitor, "changed", G_CALLBACK(SessionManager::on_file_changed), this);
        }

        g_object_unref(dir_to_watch);
    }

    component_manager->reload_configuration();

    return true;
}

void SessionManager::on_file_changed(GFileMonitor *_monitor, GFile *file, GFile *_other_file,
    GFileMonitorEvent event_type, gpointer userdata)
{
    auto *self = static_cast<SessionManager*>(userdata);
    if (!self)
    {
        return;
    }

    char *basenm = g_file_get_basename(file);
    if (!basenm)
    {
        return;
    }

    std::string filename(basenm);
    g_free(basenm);
    /* TODO Basename from -c */
    if (filename == "systemd.ini")
    {
        if ((event_type == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT) ||
            (event_type == G_FILE_MONITOR_EVENT_CREATED))
        {
            self->reload_configuration();
        }
    }
}

void SessionManager::reload_configuration()
{
    ConfigManager::reload();
    component_manager->reload_configuration();
}

void SessionManager::update_environments(const std::vector<std::pair<std::string, std::string>>& env_pairs)
{
    if (env_pairs.empty())
    {
        return;
    }

    sd_bus *bus = nullptr;
    if (sd_bus_default_user(&bus) < 0)
    {
        return;
    }

    sd_bus_message *m  = nullptr;
    sd_bus_error error = SD_BUS_ERROR_NULL;
    int r;

    /* Send to SystemD environment */
    r = sd_bus_message_new_method_call(
        bus, &m,
        "org.freedesktop.systemd1",
        "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager",
        "SetEnvironment");

    if (r >= 0)
    {
        sd_bus_message_open_container(m, 'a', "s");
        for (const auto& [key, value] : env_pairs)
        {
            std::string pair_str = key + "=" + value;
            sd_bus_message_append(m, "s", pair_str.c_str());
        }

        sd_bus_message_close_container(m);

        r = sd_bus_call(bus, m, 0, &error, nullptr);
        if (r < 0)
        {
            std::cerr << "Failed writing systemd env: " <<
                    (error.message ? error.message : "Unknown error") << std::endl;
        }

        sd_bus_message_unref(m);
        sd_bus_error_free(&error);
    }

    /* Send to DBus environment */
    r = sd_bus_message_new_method_call(
        bus, &m,
        "org.freedesktop.DBus",
        "/org/freedesktop/DBus",
        "org.freedesktop.DBus",
        "UpdateActivationEnvironment");

    if (r >= 0)
    {
        sd_bus_message_open_container(m, 'a', "{ss}");
        for (const auto& [key, value] : env_pairs)
        {
            sd_bus_message_open_container(m, 'e', "ss");
            sd_bus_message_append(m, "s", key.c_str());
            sd_bus_message_append(m, "s", value.c_str());
            sd_bus_message_close_container(m);
        }

        sd_bus_message_close_container(m);

        r = sd_bus_call(bus, m, 0, &error, nullptr);
        if (r < 0)
        {
            std::cerr << "Failed writing D-Bus env: " << (error.message ? error.message : "Unknown error") <<
                std::endl;
        }

        sd_bus_message_unref(m);
        sd_bus_error_free(&error);
    }

    sd_bus_unref(bus);
}

void SessionManager::harvest_env()
{
    auto envs = {
        "DISPLAY",
        "WAYLAND_DISPLAY",
        "WAYFIRE_SOCKET",
        "XAUTHORITY",
    };

    std::vector<std::pair<std::string, std::string>> env_pairs;
    env_pairs.push_back({"XDG_CURRENT_DESKTOP", "wayfire"});

    for (auto env_key : envs)
    {
        const char *value = std::getenv(env_key);
        if (value)
        {
            env_pairs.push_back({std::string(env_key), value});
        } else
        {
            std::cout << "Env missing " << env_key << std::endl;
        }
    }

    update_environments(env_pairs);
}
