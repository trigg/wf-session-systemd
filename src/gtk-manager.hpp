#pragma once

#include "wf-option-wrap.hpp"
#include <string>
#include <giomm.h>
#include <glibmm.h>
#include <wayfire/config/config-manager.hpp>
#include <wayfire/config/option-wrapper.hpp>

class GtkManager
{
  public:
    GtkManager();

  private:
    Glib::RefPtr<Gio::Settings> g_settings;
    WfOption<std::string> theme{"systemd/gtk_theme"};
    WfOption<std::string> icons{"systemd/icon_theme"};
    WfOption<std::string> font{"systemd/font_name"};
    WfOption<std::string> mode{"systemd/color_scheme"};
    WfOption<bool> scrolling{"systemd/overlay_scrolling"};
};
