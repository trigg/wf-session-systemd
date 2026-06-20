#pragma once

#include <wayfire/config/option-wrapper.hpp>
#include "config-manager.hpp"

/**
 * An implementation of wf::base_option_wrapper_t taken from wf-shell
 */
template<class Type>
class WfOption : public wf::base_option_wrapper_t<Type>
{
  public:
    WfOption(const std::string& option_name)
    {
        this->load_option(option_name);
    }

  protected:
    std::shared_ptr<wf::config::option_base_t> load_raw_option(const std::string& name) override
    {
        return ConfigManager::get().get_option(name);
    }
};
