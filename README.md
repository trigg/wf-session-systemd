# wf-session-systemd

A project to attempt to glue Wayfire to the user systemd session. While not strictly necessary in order to get Wayfire to work in a systemd environment, the list of steps the user is expected to take keeps growing.

Also has component lifecycle features.

## Using

First, work out what parts of your autostart could be system components

```
[autostart]
panel=wf-panel
background=wf-background
mako=mako
```

And write a new autostart
```
[autostart]
session=wf-session-systemd-startup
```

Note that if you have another wayfire-nested.ini for a nested instance you should use
```
session = systemd-run --user --unit=wf-session-nested-instance wf-session
```

and finally .config/wf-session/systemd.ini
```
[component:panel]
command=wf-panel
enabled = true
restart_on_crash = true
restart_on_graceful = false
fork_on_start = false
```

example.ini has a lot more examples.

## Differences from autostart

More writing, for one thing. Components can be set to restart under certain circumstances. If a process automatically forks on start it can be accounted for and still tracked by the session. Changing the command while it is running will do nothing, but if you then proceed to disabld & enable it, it will start again with its new command. 

Because it uses transient SystemD units under the hood, these (tab completed!) commands can be used:

`systemctl --user restart wf-session-wayland-1-wf-panel.service` Restart the panel!

`journalctl --user -xef wf-session-wayland-1-wf-background.service` Read through the log for background

`journalctl --user -xefu wf-session-wayland-1-mako.service` Follow log lines being written about mako in real time

Also notable is that changing a component from `enabled=true` to `false` will actively stop the component running and the inverse will actively start it, unlike Wayfire's autostart which does not start newly added items until next start
