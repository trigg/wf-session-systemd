#pragma once
#include <cassert>
#include <wayfire/config/config-manager.hpp>
#include <wayfire/config/file.hpp>
/* An extrapolation of wf-shell core config into a class.
 * I might PR something like this in to wf-shell to avoid the strange
 * "No WfOption in specific classes" side effect */
class ConfigManager
{
  private:
    ConfigManager()
    {}

    wf::config::config_manager_t config;
    std::string path = "";

    static ConfigManager& instance()
    {
        static ConfigManager instance;
        return instance;
    }

  public:
    ConfigManager(ConfigManager const&)   = delete;
    void operator =(ConfigManager const&) = delete;


    static void reload()
    {
        assert(instance().path != "");
        wf::config::load_configuration_options_from_file(instance().config, instance().path);
    }

    static wf::config::config_manager_t & get()
    {
        return (instance().config);
    }

    static void load(std::vector<std::string> xml, std::string in_path)
    {
        instance().path   = in_path;
        instance().config = wf::config::build_configuration(xml, "", in_path);
    }
};
