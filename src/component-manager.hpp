#pragma once

#include <memory>
#include <map>
#include <string>
#include <wayfire/config/config-manager.hpp>
#include "component.hpp"

class ComponentManager
{
  public:
    ComponentManager(const std::string& instance_name);
    ~ComponentManager();

    void reload_configuration();

    void toggle(const std::string& name, bool requested_start);

  private:
    std::string instance_id;
    std::map<std::string, std::unique_ptr<Component>> components;

    void start(const Component& app);
    void stop(const std::string& name);
};
