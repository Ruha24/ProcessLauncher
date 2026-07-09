# Process Launcher

A lightweight launcher for Windows and macOS: add your programs once
and start them with a single click, a hotkey, or in whole groups.
The app tracks running processes, can restart crashed ones, and lives
in the system tray.

*(Русская версия: [README.ru.md](README.ru.md))*

---

## Installation

**Windows.** Download `ProcessLauncher-Setup-1.0.exe` from the
[releases page](https://github.com/Ruha24/ProcessLauncher/releases),
run it and follow the wizard. It creates a Start Menu shortcut (and
optionally a desktop one). No administrator rights required.

**macOS.** Download `ProcessLauncher.dmg`, open it and drag the app
into your Applications folder. On first launch, if macOS warns about
an unidentified developer — right-click the app → "Open".

---

## How to use

**Add a program.** Click "+ Add" and pick an .exe, or just drag a
file (or an .lnk/.url shortcut) onto the window. Shortcuts are
resolved automatically — the actual program is added, not the shortcut.

**Start and stop.** Use the "Start" button next to a program,
double-click the row, or assign a hotkey with "Bind". Right-click a
program for more: restart, auto-restart, open file location, copy
path, add a note.

**Profiles.** Group programs into tabs (e.g. "Work" and "Games").
Each profile can have a color and icon ("Style" button), its own
hotkey, and a "Start all" button to launch everything at once.

**Autostart.** The "Start with Windows" checkbox makes the launcher
start with your system. In Settings you can also pick a profile to
launch automatically when the launcher opens.

**Crash watching.** Enable "Auto-restart" on a program via right-click
— if it exits unexpectedly, the launcher brings it back. All crashes
and errors are recorded in the log ("Log" button).

**Startup manager (Windows).** The "Startup" button lists every
program that launches with Windows and lets you enable, disable or
remove them — just like Task Manager.

**Settings** ("Settings" button): dark/light theme, close behavior
(minimize to tray or quit), status refresh rate, delay between
launches in a profile, and export/import of all settings to a file —
handy for moving to another computer.

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
| Global hotkeys               | yes     | no    |
| Startup manager              | yes     | no    |
| .lnk / .url resolution       | yes     | no    |

Some features are Windows-only for now — on macOS they require special
system permissions and are planned for later.

---

## Updates

The app checks for a new version on startup and, if one is available,
offers to open the download page. Installing is up to you — the app
only notifies.

License: see the LICENSE file (if you add one).
