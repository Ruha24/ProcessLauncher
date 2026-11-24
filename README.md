# Process Launcher

A lightweight launcher for Windows and macOS: add your programs once
and start them with a single click, a hotkey, or in whole groups.
The app tracks running processes, can restart crashed ones, and lives
in the system tray.

*(Русская версия: [README.ru.md](README.ru.md))*

---

## Installation

**Windows.** Download the latest `ProcessLauncher-Setup-x.y.exe` from the
[releases page](https://github.com/Ruha24/ProcessLauncher/releases),
run it and follow the wizard. It creates a Start Menu shortcut (and
optionally a desktop one). No administrator rights required.

**macOS.** Download `ProcessLauncher.dmg`, open it and drag the app
into your Applications folder. On first launch, if macOS warns about
an unidentified developer — right-click the app → "Open".

---

## How to use

The window has a sidebar on the left: your profiles on top, then
Schedule, Startup, Log and Settings.

**Add a program.** Click "Add…" and choose what to add:

- **Program or script** — an .exe or a script (.bat, .cmd, .ps1; .sh on
  macOS). Scripts are started and tracked just like programs.
- **Folder** — opens in Explorer (Finder) when you start it.
- **Link** — a website or an app link (`https://…`, `steam://…`); it
  opens in its own app.

You can also drag a file, folder or .lnk/.url shortcut onto the window.
Shortcuts are resolved automatically — the actual program (or website)
is added, not the shortcut. Folders and links are only opened: they
have no "running" state, so their button says "Open" instead of
"Start".

**Start and stop.** Use the "Start" button next to a program or
double-click the row. The "⋯" button (or a right-click) opens the rest:

- **Rename…** — your own name instead of the file name (leave it empty
  to go back to the file name);
- **Hotkey…**, **Launch arguments…**, note;
- **Start delay…** — how many seconds after its profile starts the
  program is launched (e.g. VPN first, the client 5 seconds later).
  It only applies when the whole profile starts;
- **Working folder…** — the folder the program starts in (by default,
  the folder it lives in);
- **Restart if it stops** — auto-restart;
- **Run as administrator** (Windows) — starts it through the UAC
  prompt; such programs show an "Admin" badge and can still be stopped
  with "Stop";
- move to another profile, open file location, copy path, remove.

**Tray.** Right-click the tray icon for a menu with all your profiles:
each has "Start all", "Stop all" and its programs. Click a program to
start or stop it; running ones have a green dot on their icon, and each
profile shows how many of its programs are running.

**Hotkeys.** Give a program or a whole profile a global hotkey — it
works while the launcher sits in the background. On macOS use Cmd,
Ctrl, Option and Shift as modifiers.

**Profiles.** Group programs (e.g. "Work" and "Games"). The number next
to a profile shows how many of its programs are running. Use "Start
all" to launch a whole profile; the "⋯" button next to it renames the
profile, sets its color, icon and hotkey, creates a desktop shortcut
(Windows), or deletes it.

**Schedule.** Start or stop a profile or a single program at a set
time: once, or on chosen days of the week (e.g. start work apps on
weekdays at 9:00 and close games at 23:00). If the computer was asleep
at that minute, the rule still fires when it wakes up within 5 minutes.

**Crash watching.** Turn on "Restart if it stops" for a program — if it
exits, the launcher brings it back. Everything that stopped or failed
is listed on the Log page.

**Startup.** Lists what launches together with your system and lets you
turn entries off or remove them. On Windows it is the same list Task
Manager shows; on macOS these are your launch agents.

**Settings.** Dark/light theme, opening the launcher with the system,
a profile to start when the launcher opens, what happens on close
(stay in the tray or quit), whether quitting the launcher also closes
the programs it started, timing, and export/import of everything to a
file — handy for moving to another computer.

**Command line.** Control the launcher from shortcuts and scripts:

```
ProcessLauncher --profile "Work"        start a profile
ProcessLauncher --stop-profile "Games"  stop a profile
ProcessLauncher --start "Telegram"      start a program by name
ProcessLauncher --stop "Telegram"       stop a program
ProcessLauncher --hidden                start straight to the tray
ProcessLauncher --show                  show the window
```

If the launcher is already running, the command is passed to it. With
commands the window doesn't open — everything happens in the
background; an unknown profile or program name shows a notification.
The profile menu's "Create desktop shortcut" uses `--profile`.

---

## Feature availability

| Feature                      | Windows | macOS |
|------------------------------|:-------:|:-----:|
| Start/stop programs          | yes     | yes   |
| Process-tree tracking        | yes     | yes   |
| Profiles, notes, styling     | yes     | yes   |
| System tray                  | yes     | yes   |
| Start with system            | yes     | yes   |
| Event log, update checks     | yes     | yes   |
| Global hotkeys               | yes     | yes   |
| Startup manager              | yes     | yes*  |
| Tray menu with programs      | yes     | yes   |
| Scripts                      | .bat .cmd .ps1 | .sh |
| Folders and links            | yes     | yes   |
| Run as administrator         | yes     | no    |
| Command line                 | yes     | yes   |
| Profile desktop shortcut     | yes     | no    |
| .lnk / .url resolution       | yes     | no    |

\* On macOS the startup manager lists launch agents from
`~/Library/LaunchAgents`; Login Items from System Settings are not shown.

.lnk and .url shortcuts are a Windows format, so resolving them is
Windows-only.

---

## Updates

The app checks for a new version on startup and, if one is available,
offers to open the download page. Installing is up to you — the app
only notifies.

License: Apache 2.0, see the LICENSE file.
