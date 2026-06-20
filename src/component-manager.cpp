#include "component-manager.hpp"
#include "config-manager.hpp"
#include <iostream>
#include <algorithm>
#include <systemd/sd-bus.h>
#include <systemd/sd-journal.h>
#include <wayfire/config/config-manager.hpp>

/*
 *  Track a collection of components, action changes to enabled.
 *
 *
 */

ComponentManager::ComponentManager(const std::string& instance_name) :
    instance_id(instance_name)
{}

ComponentManager::~ComponentManager()
{}

void ComponentManager::toggle(const std::string& name, bool requested_start)
{
    auto it = components.find(name);
    if (it == components.end())
    {
        return;
    }

    const auto& component = *(it->second);
    std::string unit_name = component.get_unit_name();

    /* systemctl --user ... */
    sd_bus *bus = nullptr;
    if (sd_bus_default_user(&bus) < 0)
    {
        return;
    }

    sd_bus_error error = SD_BUS_ERROR_NULL;

    if (requested_start)
    {
        if (!component.is_currently_running())
        {
            std::cout << "Creating service : " << unit_name << std::endl;

            sd_bus_call_method(
                bus, "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
                "org.freedesktop.systemd1.Manager",
                "ResetFailedUnit", &error, nullptr, "s", unit_name.c_str());
            sd_bus_error_free(&error);

            start(component);
        }
    } else
    {
        std::cout << "Stopping service : " << unit_name << std::endl;
        sd_bus_call_method(
            bus, "org.freedesktop.systemd1", "/org/freedesktop/systemd1", "org.freedesktop.systemd1.Manager",
            "StopUnit", &error, nullptr, "ss", unit_name.c_str(), "replace");
    }

    if (error.message)
    {
        std::cerr << "Dbus error : " << error.message <<
            std::endl;
    }

    sd_bus_error_free(&error);
    sd_bus_unref(bus);
}

/* Since we need to catch new & remove sections, we can't use WfOption */
void ComponentManager::reload_configuration()
{
    auto sections = ConfigManager::get().get_all_sections();
    std::vector<std::string> active_in_config;

    for (const auto& section : sections)
    {
        std::string raw_name = section->get_name();
        if (raw_name.rfind("component:", 0) != 0)
        {
            continue;
        }

        std::string clean_name = raw_name.substr(10);
        if (!section->get_option("command"))
        {
            continue;
        }

        active_in_config.push_back(clean_name);

        auto it = components.find(clean_name);

        if (it == components.end())
        {
            auto comp = std::make_unique<Component>(section, instance_id);

            comp->signal_state_changed.connect([this] (const std::string& name, bool requested_start)
            {
                toggle(name, requested_start);
            });

            components[clean_name] = std::move(comp);

            const auto& added_comp = *(components[clean_name]);
            if (added_comp.is_enabled())
            {
                toggle(clean_name, true);
            }
        } else
        {
            const auto& comp   = *(it->second);
            bool wants_running = comp.is_enabled();
            bool currently_running = comp.is_currently_running();

            if (!wants_running && currently_running)
            {
                toggle(clean_name, false);
            } else if (wants_running && !currently_running)
            {
                toggle(clean_name, true);
            }
        }
    }

    for (auto it = components.begin(); it != components.end();)
    {
        std::string name = it->first;
        if (std::find(active_in_config.begin(), active_in_config.end(), name) == active_in_config.end())
        {
            stop(name);
            it = components.erase(it);
        } else
        {
            ++it;
        }
    }
}

/* Create and start a transient unit in the user systemd.
 *
 * This is an abomination and I love it
 */
void ComponentManager::start(const Component& app)
{
    sd_bus *bus = nullptr;
    if (sd_bus_default_user(&bus) < 0)
    {
        return;
    }

    std::string unit_name = "wf-session-" + instance_id + "-" + app.get_name() + ".service";
    std::string target_template = "wf-session@" + instance_id + ".target";
    std::string restart_policy  =
        app.should_restart_on_graceful() ? "always" : (app.should_restart_on_crash() ? "on-failure" : "no");
    bool remain_after_exit   = (!app.should_restart_on_graceful() && !app.should_restart_on_crash());
    std::string service_type = app.is_fork_on_start() ? "forking" : "simple";
    std::string wl_env_var   = "WAYLAND_DISPLAY=" + instance_id;

    sd_bus_message *m  = nullptr;
    sd_bus_error error = SD_BUS_ERROR_NULL;

    int r = sd_bus_message_new_method_call(
        bus, &m,
        "org.freedesktop.systemd1",
        "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager",
        "StartTransientUnit");

    if (r >= 0)
    {
        sd_bus_message_append(m, "ss", unit_name.c_str(), "replace");

        sd_bus_message_open_container(m, 'a', "(sv)");
        /* A description of what this unit is */
        sd_bus_message_open_container(m, 'r', "sv");
        sd_bus_message_append(m, "s", "Description");
        sd_bus_message_open_container(m, 'v', "s");
        sd_bus_message_append(m, "s", ("wf-session (" + instance_id + "): " + app.get_name()).c_str());
        sd_bus_message_close_container(m);
        sd_bus_message_close_container(m);
        /* The type, oneshot, simple etc. */
        sd_bus_message_open_container(m, 'r', "sv");
        sd_bus_message_append(m, "s", "Type");
        sd_bus_message_open_container(m, 'v', "s");
        sd_bus_message_append(m, "s", service_type.c_str());
        sd_bus_message_close_container(m);
        sd_bus_message_close_container(m);
        /* Restart policy, necessary for people who keep crashing their panel */
        sd_bus_message_open_container(m, 'r', "sv");
        sd_bus_message_append(m, "s", "Restart");
        sd_bus_message_open_container(m, 'v', "s");
        sd_bus_message_append(m, "s", restart_policy.c_str());
        sd_bus_message_close_container(m);
        sd_bus_message_close_container(m);
        /* Remain policy, If a component should never restart, also retain it until logout */
        sd_bus_message_open_container(m, 'r', "sv");
        sd_bus_message_append(m, "s", "RemainAfterExit");
        sd_bus_message_open_container(m, 'v', "b");
        sd_bus_message_append(m, "b", remain_after_exit);
        sd_bus_message_close_container(m);
        sd_bus_message_close_container(m);
        /* When stopped or failed, destroy this. Allows it to return with changes
         * We don't/can't/shouldn't rewrite the contents while it is running,
         * so we at least guarantee a disable/enable picks up all changes*/
        sd_bus_message_open_container(m, 'r', "sv");
        sd_bus_message_append(m, "s", "CollectMode");
        sd_bus_message_open_container(m, 'v', "s");
        sd_bus_message_append(m, "s", "inactive-or-failed");
        sd_bus_message_close_container(m);
        sd_bus_message_close_container(m);
        /* Link it to the target of our session manager. Session ends properly,
         * so should our components.*/
        sd_bus_message_open_container(m, 'r', "sv");
        sd_bus_message_append(m, "s", "PartOf");
        sd_bus_message_open_container(m, 'v', "as");
        sd_bus_message_open_container(m, 'a', "s");
        sd_bus_message_append(m, "s", target_template.c_str());
        sd_bus_message_close_container(m);
        sd_bus_message_close_container(m);
        sd_bus_message_close_container(m);
        /* TODO Reconsider, feed vars directly into Env higher up */
        sd_bus_message_open_container(m, 'r', "sv");
        sd_bus_message_append(m, "s", "Environment");
        sd_bus_message_open_container(m, 'v', "as");
        sd_bus_message_open_container(m, 'a', "s");
        sd_bus_message_append(m, "s", wl_env_var.c_str());
        sd_bus_message_close_container(m);
        sd_bus_message_close_container(m);
        sd_bus_message_close_container(m);
        /* The actual command at last */
        sd_bus_message_open_container(m, 'r', "sv");
        sd_bus_message_append(m, "s", "ExecStart");
        sd_bus_message_open_container(m, 'v', "a(sasb)");
        sd_bus_message_open_container(m, 'a', "(sasb)");
        sd_bus_message_open_container(m, 'r', "sasb");
        /* It needs the path of the executable alone, first */
        sd_bus_message_append(m, "s", "/bin/sh");
        /* And then an array of argv. Which MUST start with the binary again at argv[0] */
        sd_bus_message_open_container(m, 'a', "s");
        sd_bus_message_append(m, "s", "/bin/sh");
        sd_bus_message_append(m, "s", "-c");
        sd_bus_message_append(m, "s", app.get_command().c_str());
        sd_bus_message_close_container(m);
        /* Fail is non-fatal */
        sd_bus_message_append(m, "b", false);

        sd_bus_message_close_container(m);
        sd_bus_message_close_container(m);
        sd_bus_message_close_container(m);
        sd_bus_message_close_container(m);

        sd_bus_message_close_container(m);

        sd_bus_message_open_container(m, 'a', "(sa(sv))");
        sd_bus_message_close_container(m);

        r = sd_bus_call(bus, m, 0, &error, nullptr);
        if ((r < 0) && error.message)
        {
            std::cerr << "Failed creating unit: " << error.message <<
                std::endl;
        }

        sd_bus_message_unref(m);
    }

    sd_bus_error_free(&error);
    sd_bus_unref(bus);
}

/* Stop the unit */
void ComponentManager::stop(const std::string& name)
{
    sd_bus *bus = nullptr;
    if (sd_bus_default_user(&bus) < 0)
    {
        return;
    }

    std::string unit_name = "wf-session-" + instance_id + "-" + name + ".service";
    sd_bus_error error    = SD_BUS_ERROR_NULL;
    sd_bus_call_method(
        bus, "org.freedesktop.systemd1", "/org/freedesktop/systemd1", "org.freedesktop.systemd1.Manager",
        "StopUnit", &error, nullptr, "ss", unit_name.c_str(), "replace");
    sd_bus_error_free(&error);
    sd_bus_unref(bus);
}
