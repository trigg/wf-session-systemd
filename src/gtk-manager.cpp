#include "gtk-manager.hpp"
#include <iostream>
/* Maintains GTK Settings per users preferences. */

GtkManager::GtkManager()
{
    g_settings = Gio::Settings::create("org.gnome.desktop.interface");
    if (!g_settings)
    {
        std::cerr << "Failed to get gsettings" <<
            std::endl;
        return;
    }

    auto set_theme = [this] ()
    {
        g_settings->set_string("gtk-theme", theme.value());
    };
    auto set_icons = [this] ()
    {
        g_settings->set_string("icon-theme", icons.value());
    };
    auto set_scrolling_overlay = [this] ()
    {
        g_settings->set_boolean("overlay-scrolling", scrolling.value());
    };
    auto set_font = [this] ()
    {
        g_settings->set_string("font-name", font.value());
    };
    auto set_mode = [this] ()
    {
        g_settings->set_string("color-scheme", mode.value());
    };

    set_theme();
    set_icons();
    set_scrolling_overlay();
    set_font();
    set_mode();

    theme.set_callback(set_theme);
    icons.set_callback(set_icons);
    scrolling.set_callback(set_scrolling_overlay);
    font.set_callback(set_font);
    mode.set_callback(set_mode);
}
