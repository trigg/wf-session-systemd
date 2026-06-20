# wf-session-systemd

A project to attempt to glue Wayfire to the user systemd session. While not strictly necessary in order to get Wayfire to work in a systemd environment, the list of steps the user is expected to take keeps growing.

Also has component lifecycle features.

## Using

First, work out what parts of your autostart could be system elements

```
[autostart]
panel=wf-panel
background=wf-background
mako=mako
```

And write a new autostart
```
[autostart]
session=systemctl --user start wf-session
```

Note that if you have another wayfire-nested.ini for a nested instance you should use
```
session = systemd-run --user --unit=wf-session-nested-instance wf-session
```

and finally .config/wf-session/systemd.ini
```
[component:panel]
commane=wf-panel
enabled = true
restart_on_crash = true
restart_on_graceful = false
fork_on_start = false
```

example.ini has a lot more examples.