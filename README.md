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

**Add a program.** Click "Add program" and pick an .exe, or just drag a
file (or an .lnk/.url shortcut) onto the window. Shortcuts are
resolved automatically — the actual program is added, not the shortcut.

**Start and stop.** Use the "Start" button next to a program or
double-click the row. The "⋯" button (or a right-click) opens the rest:
restart, hotkey, launch arguments, note, auto-restart, move to another
profile, open file location, copy path, remove.

**Hotkeys.** Give a program or a whole profile a global hotkey — it
works while the launcher sits in the background. On macOS use Cmd,
Ctrl, Option and Shift as modifiers.

**Profiles.** Group programs (e.g. "Work" and "Games"). The number next
to a profile shows how many of its programs are running. Use "Start
all" to launch a whole profile; the "⋯" button next to it renames the
profile, sets its color, icon and hotkey, or deletes it.

**Schedule.** Start a profile or a single program at a set time, once
or every day.

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
