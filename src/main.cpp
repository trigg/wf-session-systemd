#include <iostream>
#include <unistd.h>
#include "config-manager.hpp"
#include "session-manager.hpp"
/*
 *  Wayfire Session Manager SystemD
 *
 *  Main just preps config & starts SessionManager
 */

int main(int argc, char *argv[])
{
    std::string custom_config_path = "";
    int opt;

    while ((opt = getopt(argc, argv, "c:h")) != -1)
    {
        if (opt == 'c')
        {
            custom_config_path = optarg;
        } else
        {
            std::cout << "Usage: wf-session-systemd [-c /path/to/config.ini]\n";
            return 0;
        }
    }

    ConfigManager::get();
    SessionManager session;

    if (!session.init(custom_config_path))
    {
        return 1;
    }

    session.run();
    return 0;
}
