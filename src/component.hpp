#pragma once

#include "wf-option-wrap.hpp"
#include <string>
#include <memory>
#include <sigc++/sigc++.h>
#include <wayfire/config/section.hpp>

class Component
{
  public:
    Component(std::shared_ptr<wf::config::section_t> section, const std::string& instance_id);
    ~Component();

    std::string get_name() const;
    std::string get_command() const;
    bool is_enabled() const;
    bool should_restart_on_crash() const;
    bool should_restart_on_graceful() const;
    bool is_fork_on_start() const;
    bool is_session() const;

    std::string get_unit_name() const;
    std::string get_unit_path() const;

    bool is_currently_running() const;

    sigc::signal<void(const std::string&, bool)> signal_state_changed;

  private:
    std::string name;
    WfOption<std::string> command;
    std::string instance_id;
    WfOption<bool> restart_on_crash;
    WfOption<bool> restart_on_graceful;
    WfOption<bool> fork_on_start;
    WfOption<bool> enabled;
    WfOption<bool> session;

    std::shared_ptr<wf::config::section_t> config_section;
    wf::config::option_base_t::updated_callback_t on_enabled_changed_cb;

    void on_enabled_config_changed();
};
