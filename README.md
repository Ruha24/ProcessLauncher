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
Schedule, Startup, Statistics, Log and Settings.

**Add a program.** Click "Add…" and choose what to add:

- **Program or script** — an .exe or a script (.bat, .cmd, .ps1; .sh on
  macOS). Scripts are started and tracked just like programs.
- **Folder** — opens in Explorer (Finder) when you start it.
- **Link** — a website or an app link (`https://…`, `steam://…`); it
  opens in its own app.

You can also drag a file, folder or .lnk/.url shortcut onto the window.
Shortcuts are resolved automatically — the actual program (or website)
is added, not the shortcut; Steam game shortcuts keep their name and
icon. Folders and links are only opened: they have no "running" state,
so their button says "Open" instead of "Start". Links get their icon
automatically: a website's icon, or a Steam game's
(`steam://rungameid/…`) icon from Steam's cache.

**Start and stop.** Use the "Start" button next to a program or
double-click the row. A running program shows how long it has been
running and how much memory and CPU it uses, counting its child
processes too. The "⋯" button (or a right-click) opens the rest:

- **Rename…** — your own name instead of the file name (leave it empty
  to go back to the file name);
- **Change icon…** — your own icon: an image (.png, .jpg, .ico) or the
  icon of any .exe; links also have "Get icon from link";
- **Hotkey…**, **Launch arguments…**, note;
- **Start delay…** — how many seconds after its profile starts the
  program is launched (e.g. VPN first, the client 5 seconds later).
  It only applies when the whole profile starts;
- **Working folder…** — the folder the program starts in (by default,
  the folder it lives in);
- **Window…** (Windows) — open the program's window as usual, minimized
  or maximized. Some programs choose their own window size and ignore it;
- **Environment variables…** — extra `NAME=value` lines for this
  program only; `%NAME%` inserts the current value
  (`JAVA_OPTS=%JAVA_OPTS% -Xmx2g`), an empty value removes a variable;
- **Restart if it stops** — auto-restart;
- **Run as administrator** (Windows) — starts it through the UAC
  prompt; such programs show an "Admin" badge and can still be stopped
  with "Stop";
- **Duplicate** — a copy of the program right below it, e.g. to run
  it with other arguments or keep it in another profile;
- move to another profile, open file location, copy path, remove.

**Already running programs.** If a program from the list is already
open — you started it yourself, or it was running before the launcher —
its row shows it as running within a few seconds, and "Start" doesn't
open a second copy. Its running time counts from when it really
started. Scripts can't be recognized this way.

**Quick search.** Press Ctrl+Alt+Space anywhere (change or turn it off
in Settings → Quick search). A search box opens over all windows: type
a few letters of a program or profile from any profile and press Enter
to start it, or Ctrl+Enter (Cmd+Enter on macOS) to stop it. Letters
typed in the wrong keyboard layout (Russian ↔ English) still match.

**Statistics.** The Statistics page shows how long each program has
been running today, in the last 7 days and in total, with the biggest
first. Time is counted while a program runs, including programs
started outside the launcher.

**Order.** Drag a program by the handle on the left of its row to move
it up or down. A profile starts its programs from top to bottom.

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

**Settings.** Dark/light theme, interface language (English or
Russian; by default, the system's), the quick search hotkey, opening the
launcher with the system,
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
| Memory and CPU of programs   | yes     | yes   |
| English and Russian UI       | yes     | yes   |
| Already running programs     | yes     | yes   |
| Quick search hotkey          | yes     | yes   |
| Time statistics              | yes     | yes   |
| Environment variables        | yes     | yes   |
| Window state on start        | yes     | no    |
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
