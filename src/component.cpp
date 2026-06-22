#include "component.hpp"
#include "wf-option-wrap.hpp"
#include <systemd/sd-bus.h>

/* Maintains the state of exactly one component.
 *
 * We do this by creating a temporary SystemD user unit, populate it with the necessary data
 * and just set it going.
 *
 * Using systemd and not simply firing off execs gives us a lot more advantages that I originally bargained
 * for
 *
 * If the session manager dies and is restarted it can correctly poll the state of components from the
 * previous iteration.
 *
 * We can allow systemd to restart on fail/end without putting lots of signals, polling etc all over this
 * manager
 */

Component::~Component()
{}

std::string Component::get_name() const
{
    return name;
}

std::string Component::get_command() const
{
    return command;
}

bool Component::should_restart_on_crash() const
{
    return restart_on_crash;
}

bool Component::should_restart_on_graceful() const
{
    return restart_on_graceful;
}

bool Component::is_fork_on_start() const
{
    return fork_on_start;
}

bool Component::is_enabled() const
{
    auto opt_en = config_section->get_option("enabled");
    if (!opt_en)
    {
        return false;
    }

    return (opt_en->get_value_str() == "true" || opt_en->get_value_str() == "1");
}

bool Component::is_session() const
{
    return session;
}

std::string Component::get_unit_name() const
{
    return "wf-session-" + instance_id + "-" + name + ".service";
}

/* Get a parsed path for this unit name */
std::string Component::get_unit_path() const
{
    std::string unit_name = get_unit_name();
    size_t pos;
    while ((pos = unit_name.find("-")) != std::string::npos)
    {
        unit_name.replace(pos, 1, "_2d");
    }

    while ((pos = unit_name.find(".")) != std::string::npos)
    {
        unit_name.replace(pos, 1, "_2e");
    }

    while ((pos = unit_name.find("@")) != std::string::npos)
    {
        unit_name.replace(pos, 1, "_40");
    }

    return "/org/freedesktop/systemd1/unit/" + unit_name;
}

bool Component::is_currently_running() const
{
    sd_bus *bus = nullptr;
    if (sd_bus_default_user(&bus) < 0)
    {
        return false;
    }

    std::string unit_path = get_unit_path();

    char *active_state = nullptr;
    sd_bus_error error = SD_BUS_ERROR_NULL;

    int r = sd_bus_get_property_string(
        bus,
        "org.freedesktop.systemd1",
        unit_path.c_str(),
        "org.freedesktop.systemd1.Unit",
        "ActiveState",
        &error,
        &active_state);

    bool running = false;
    if ((r >= 0) && (active_state != nullptr))
    {
        std::string state(active_state);
        if ((state == "active") || (state == "activating"))
        {
            running = true;
        }

        free(active_state);
    }

    sd_bus_error_free(&error);
    sd_bus_unref(bus);
    return running;
}

Component::Component(std::shared_ptr<wf::config::section_t> section,
    const std::string& instance_id) :
    name{section->get_name().substr(10)},
    command{section->get_name() + "/command"},
    instance_id(instance_id),
    restart_on_crash{section->get_name() + "/restart_on_crash"},
    restart_on_graceful{section->get_name() + "/restart_on_graceful"},
    fork_on_start{section->get_name() + "/fork_on_start"},
    enabled{section->get_name() + "/enabled"},
    session(section->get_name() + "/session_service"),
    config_section(section)
{
    enabled.set_callback([this] ()
    {
        signal_state_changed.emit(name, is_enabled());
    });
}
