# Русский перевод интерфейса. Заполняет i18n/ProcessLauncher_ru.ts:
#   cmake --build build --target update_translations   # собрать новые строки
#   python -I i18n/ru.py                               # вписать переводы
# Строки без перевода скрипт перечисляет — их нужно добавить сюда.
# Ключ — английская строка; кавычки «“”», многоточие и тире при сравнении
# не важны. Перевод для отдельного контекста: ключ "Контекст|строка".
import os
import sys
import xml.etree.ElementTree as ET

RU = {
    # Общие
    "Save": "Сохранить",
    "Cancel": "Отмена",
    "Remove": "Удалить",
    "Rename": "Переименовать",
    "Add": "Добавить",
    "Reset": "Сбросить",
    "Open": "Открыть",
    "Start": "Запустить",
    "Stop": "Остановить",
    "Restart": "Перезапустить",
    "Start all": "Запустить все",
    "Stop all": "Остановить все",
    "Process Launcher": "Process Launcher",
    "Process Launcher %1": "Process Launcher %1",
    "Profiles": "Профили",
    "Schedule": "Расписание",
    "Startup": "Автозагрузка",
    "Log": "Журнал",
    "Settings": "Настройки",
    "Profile": "Профиль",
    "Program": "Программа",
    "Color": "Цвет",
    "Icon": "Иконка",
    "current": "текущий",
    "None": "Нет",

    # HotkeyDialog
    "Remove hotkey": "Убрать клавишу",
    "Press a key or a combination": "Нажмите клавишу или сочетание",
    "Letters, digits and F1–F20, alone or with Cmd, Ctrl, Option or Shift. The hotkey works even when the launcher is in the background.":
        "Буквы, цифры и F1–F20 — отдельно или с Cmd, Ctrl, Option или Shift. Клавиша работает, даже когда лаунчер в фоне.",
    "Letters, digits and F1–F24, alone or with Ctrl, Alt, Shift or Win. The hotkey works even when the launcher is in the background.":
        "Буквы, цифры и F1–F24 — отдельно или с Ctrl, Alt, Shift или Win. Клавиша работает, даже когда лаунчер в фоне.",

    # LogPage
    "Programs that stopped and errors, newest first.": "Остановившиеся программы и ошибки, новые сверху.",
    "Clear log": "Очистить журнал",
    "Clear the log?": "Очистить журнал?",
    "All recorded events are deleted. This cannot be undone.": "Все записанные события будут удалены. Отменить это нельзя.",
    "Error": "Ошибка",
    "Stopped": "Остановлена",
    "Nothing has stopped or failed yet.": "Пока ничего не останавливалось и не падало.",

    # Main
    "Program stopped": "Программа остановилась",
    "%1 (%2) is no longer running.": "%1 (%2) больше не работает.",
    "Add link": "Добавить ссылку",
    "A website or an app link (steam://, discord://…). It opens in its app; the launcher doesn't track it.":
        "Сайт или ссылка на приложение (steam://, discord://…). Она откроется в своём приложении; лаунчер за ней не следит.",
    "That doesn't look like a link": "Это не похоже на ссылку",
    "Hotkey for %1": "Клавиша для %1",
    "Pressing it starts this program.": "По нажатию запускается эта программа.",
    "Hotkey for “%1”": "Клавиша для «%1»",
    "Pressing it starts every program in this profile.": "По нажатию запускаются все программы профиля.",
    "Launch arguments": "Аргументы запуска",
    "Passed to the program on its command line every time it starts.": "Передаются программе в командной строке при каждом запуске.",
    "--fullscreen --profile dev": "--fullscreen --profile dev",
    "Rename program": "Переименовать программу",
    "Leave empty to use the file name (%1).": "Оставьте пустым, чтобы взять имя файла (%1).",
    "Start delay for %1": "Задержка запуска: %1",
    "Seconds to wait after its profile starts, e.g. to let a VPN connect first. 0 — start right away. Starting the program on its own is never delayed.":
        "Сколько секунд ждать после запуска профиля — например, пока подключится VPN. 0 — сразу. Если запускать программу отдельно, задержки нет.",
    "Enter a number of seconds from 0 to 3600": "Введите число секунд от 0 до 3600",
    "Note": "Заметка",
    "Shown under the program name in the list.": "Показывается в списке под названием программы.",
    "Start after the VPN is up": "Запускать после VPN",
    "Remove %1 from the list?": "Убрать %1 из списка?",
    "Its hotkey, arguments and note are forgotten. The program itself stays on your computer.":
        "Её клавиша, аргументы и заметка забудутся. Сама программа останется на компьютере.",
    "New profile": "Новый профиль",
    "A profile is a group of programs you start together.": "Профиль — это группа программ, которые запускаются вместе.",
    "Work, Games, Streaming": "Работа, Игры, Стрим",
    "Create profile": "Создать профиль",
    "A profile named “%1” already exists": "Профиль «%1» уже есть",
    "Rename profile": "Переименовать профиль",
    "Delete the profile “%1”?": "Удалить профиль «%1»?",
    "Its programs are not removed — they move to “%1”.": "Его программы не удалятся — они перейдут в «%1».",
    "Delete profile": "Удалить профиль",
    "Rename…": "Переименовать…",
    "Color and icon…": "Цвет и иконка…",
    "Hotkey…": "Горячая клавиша…",
    "Create desktop shortcut": "Ярлык на рабочем столе",
    "Shortcut “%1” is on the desktop": "Ярлык «%1» на рабочем столе",
    "Could not create the shortcut": "Не удалось создать ярлык",
    "Stop programs in all profiles": "Остановить программы всех профилей",
    "Choose a program": "Выберите программу",
    "Programs, scripts and shortcuts (*.exe *.bat *.cmd *.ps1 *.lnk *.url)": "Программы, скрипты и ярлыки (*.exe *.bat *.cmd *.ps1 *.lnk *.url)",
    "Executables (*.exe)": "Программы (*.exe)",
    "Scripts (*.bat *.cmd *.ps1)": "Скрипты (*.bat *.cmd *.ps1)",
    "Shortcuts (*.lnk *.url)": "Ярлыки (*.lnk *.url)",
    "All files (*)": "Все файлы (*)",
    "Choose an icon": "Выберите иконку",
    "Images and programs (*.png *.jpg *.jpeg *.ico *.bmp *.exe)": "Картинки и программы (*.png *.jpg *.jpeg *.ico *.bmp *.exe)",
    "Images (*.png *.jpg *.jpeg *.ico *.bmp *.icns)": "Картинки (*.png *.jpg *.jpeg *.ico *.bmp *.icns)",
    "Choose a folder": "Выберите папку",
    "Program or script…": "Программу или скрипт…",
    "Folder…": "Папку…",
    "Link…": "Ссылку…",
    "Working folder": "Рабочая папка",
    "Export configuration": "Экспорт настроек",
    "Config files (*.json)": "Файлы настроек (*.json)",
    "Configuration exported": "Настройки сохранены в файл",
    "Could not write the file. Check that the folder is writable.": "Не удалось записать файл. Проверьте, что в папку можно писать.",
    "Import configuration": "Импорт настроек",
    "Configuration imported": "Настройки загружены",
    "This file is not a Process Launcher configuration. Nothing was changed.": "Это не файл настроек Process Launcher. Ничего не изменилось.",
    "Move to profile": "Перенести в профиль",
    "Color and icon for “%1”": "Цвет и иконка для «%1»",
    "Version %1 is available": "Доступна версия %1",
    "Download failed: %1": "Не удалось скачать: %1",
    "You have version %1.": "У вас версия %1.",
    "Downloading… %1%": "Скачивание… %1%",
    "Downloaded. Installing closes the launcher and the programs it started, then opens it again.":
        "Скачано. Установка закроет лаунчер и запущенные им программы, а потом откроет его снова.",
    "Downloaded. Installing closes the launcher and opens it again; your programs keep running.":
        "Скачано. Установка закроет лаунчер и откроет его снова; программы продолжат работать.",
    "Later": "Позже",
    "Open download page": "Открыть страницу загрузки",
    "Download": "Скачать",
    "Install": "Установить",

    # ProcessManager
    "Couldn't find an icon for %1": "Не нашлась иконка для %1",
    "Invalid hotkey: %1": "Неверная клавиша: %1",
    "Could not register profile hotkey \"%1\"": "Не удалось назначить клавишу профиля «%1»",
    "Could not resolve shortcut: %1": "Не удалось прочитать ярлык: %1",
    "Icons can only be found for websites and Steam games": "Иконку можно найти только для сайтов и игр Steam",
    "%1: could not open link — %2": "%1: не удалось открыть ссылку — %2",
    "%1: not found — %2": "%1: не найдено — %2",
    "%1: could not open folder": "%1: не удалось открыть папку",
    "%1: working folder not found — %2": "%1: рабочая папка не найдена — %2",
    "Failed to start": "Не удалось запустить",
    "%1: launch as administrator was cancelled": "%1: запуск от администратора отменён",
    "%1: failed to start as administrator": "%1: не удалось запустить от администратора",
    "Can't stop %1: it runs with administrator rights. Close it yourself.":
        "Не получается остановить %1: она запущена с правами администратора. Закройте её сами.",
    "%1 (copy)": "%1 (копия)",
    "%1 — restarting": "%1 — перезапуск",
    "Could not register hotkey \"%1\"": "Не удалось назначить клавишу «%1»",

    # ProcessRow
    "%1 GB": "%1 ГБ",
    "%1 MB": "%1 МБ",
    "CPU %1%": "ЦП %1%",
    "%1 h %2 min": "%1 ч %2 мин",
    "%1 min %2 s": "%1 мин %2 с",
    "%1 s": "%1 с",
    "Drag to reorder": "Перетащите, чтобы изменить порядок",
    "Change icon…": "Сменить иконку…",
    "Get icon from link": "Взять иконку по ссылке",
    "Reset icon": "Вернуть иконку",
    "Launch arguments…": "Аргументы запуска…",
    "Start delay…": "Задержка запуска…",
    "Working folder…": "Рабочая папка…",
    "custom": "своя",
    "Reset working folder": "Вернуть рабочую папку",
    "Edit note…": "Изменить заметку…",
    "Add note…": "Добавить заметку…",
    "Restart if it stops": "Перезапускать, если закроется",
    "Run as administrator": "Запускать от администратора",
    "Duplicate": "Дублировать",
    "Move to profile…": "Перенести в профиль…",
    "Open folder": "Открыть папку",
    "Open file location": "Открыть расположение файла",
    "Copy link": "Копировать ссылку",
    "Copy path": "Копировать путь",
    "Remove from list": "Убрать из списка",
    "Running for %1": "Работает %1",
    "Running": "Работает",
    "Memory and CPU, including child processes": "Память и процессор вместе с дочерними процессами",
    "+%1 s": "+%1 с",
    "Starts %1 s after the profile starts": "Запускается через %1 с после старта профиля",
    "Admin": "Админ",
    "Auto-restart": "Автоперезапуск",
    "More actions": "Ещё",

    # ProgramsPage
    "No programs yet": "Программ пока нет",
    # Без числительных: «Запущено: 3 из 5», «Программ: 5» — так не нужны
    # формы множественного числа.
    "%1 of %2 running": "Запущено: %1 из %2",
    "1 program": "Программ: 1",
    "%1 programs": "Программ: %1",
    "Profile actions": "Действия с профилем",
    "Search in this profile": "Поиск в профиле",
    "Add…": "Добавить…",
    "Path copied": "Путь скопирован",
    "Nothing matches “%1”": "По запросу «%1» ничего нет",
    "“%1” is empty": "В «%1» пусто",
    "Add a program, script, folder or link — or drop it onto this window. Then start it with one click or a hotkey.":
        "Добавьте программу, скрипт, папку или ссылку — или перетащите её в это окно. Потом запускайте одним щелчком или клавишей.",
    "Drop to add to “%1”": "Отпустите, чтобы добавить в «%1»",

    # main.cpp
    "No profile named “%1”": "Нет профиля «%1»",
    "No program named “%1”": "Нет программы «%1»",

    # ScheduleModel / SchedulePage
    "every day": "каждый день",
    "weekdays": "по будням",
    "weekends": "по выходным",
    "Mon": "Пн",
    "Tue": "Вт",
    "Wed": "Ср",
    "Thu": "Чт",
    "Fri": "Пт",
    "Sat": "Сб",
    "Sun": "Вс",
    "Stop whole profile": "Остановить весь профиль",
    "Start whole profile": "Запустить весь профиль",
    "Stop program": "Остановить программу",
    "Start program": "Запустить программу",
    "%1, once": "%1, один раз",
    "%1, %2": "%1, %2",
    "Start or stop a profile or a single program at a set time. The launcher has to be running at that moment.":
        "Запуск или остановка профиля либо одной программы в заданное время. В этот момент лаунчер должен работать.",
    "Choose a profile": "Выберите профиль",
    "SchedulePage|Choose a program": "Выберите программу",
    "at": "в",
    "Repeat": "Повторять",
    "Once": "Один раз",
    "Add rule": "Добавить правило",
    "Delete rule": "Удалить правило",
    "No rules yet. Pick what to start and when, then add a rule.": "Правил пока нет. Выберите, что и когда запускать, и добавьте правило.",

    # SettingsPage
    "You have the latest version": "У вас последняя версия",
    "Could not check for updates: %1": "Не удалось проверить обновления: %1",
    "Appearance": "Оформление",
    "Dark theme": "Тёмная тема",
    "Language": "Язык",
    "As in the system": "Как в системе",
    "Behavior": "Поведение",
    "Open the launcher at login": "Открывать лаунчер при входе в систему",
    "Open the launcher when Windows starts": "Открывать лаунчер при запуске Windows",
    "Start a profile when the launcher opens": "Запускать профиль при открытии лаунчера",
    "Every program in the chosen profile starts right after the launcher does.": "Все программы выбранного профиля запустятся сразу после лаунчера.",
    "Keep running in the tray when the window is closed": "Оставаться в трее, когда окно закрыто",
    "Hotkeys, the schedule and auto-restart only work while the launcher is running.": "Горячие клавиши, расписание и автоперезапуск работают, только пока лаунчер запущен.",
    "Close launched programs when the launcher quits": "Закрывать запущенные программы при выходе из лаунчера",
    "Turn this off to leave your programs running after you quit or update the launcher.": "Выключите, чтобы программы продолжали работать после выхода или обновления лаунчера.",
    "Timing": "Время",
    "Pause between programs when starting a profile": "Пауза между программами при запуске профиля",
    "In milliseconds. 0 starts everything at once; 1000 starts one program per second.": "В миллисекундах. 0 — всё сразу; 1000 — по программе в секунду.",
    "How often to check whether programs are still running": "Как часто проверять, работают ли программы",
    "In milliseconds, from 200 to 10000. Lower values notice a stopped program sooner.": "В миллисекундах, от 200 до 10000. Чем меньше, тем быстрее заметна остановка программы.",
    "Backup": "Резервная копия",
    "Programs, profiles and hotkeys": "Программы, профили и клавиши",
    "Export them to a file to move to another computer. Importing replaces the current list and stops running programs.":
        "Сохраните их в файл, чтобы перенести на другой компьютер. Импорт заменит текущий список и остановит запущенные программы.",
    "Export…": "Экспорт…",
    "Import…": "Импорт…",
    "About": "О программе",
    "Updates are checked every time the launcher starts.": "Обновления проверяются при каждом запуске лаунчера.",
    "Check for updates": "Проверить обновления",

    # StartupModel / StartupPage
    "Could not change startup state for %1": "Не удалось изменить автозагрузку для %1",
    "Could not remove %1": "Не удалось удалить %1",
    "Registry (HKCU)": "Реестр (HKCU)",
    "Startup folder": "Папка автозагрузки",
    "LaunchAgent": "LaunchAgent",
    "Launch agents that start when you log in. Login Items from System Settings are not listed here.":
        "Агенты запуска, которые стартуют при входе. Объекты входа из Системных настроек здесь не показаны.",
    "Programs that start together with Windows. Turning one off keeps it in the list.":
        "Программы, которые запускаются вместе с Windows. Выключенная программа остаётся в списке.",
    "Refresh": "Обновить",
    "Remove from startup": "Убрать из автозагрузки",
    "Remove “%1” from startup?": "Убрать «%1» из автозагрузки?",
    "Its launch agent file goes to the Trash. The program itself stays installed.": "Файл агента запуска отправится в Корзину. Сама программа останется установленной.",
    "It will no longer start with Windows. The program itself stays installed.": "Она больше не будет запускаться с Windows. Сама программа останется установленной.",
    "Nothing starts with the system.": "С системой ничего не запускается.",

    # TrayController
    "Show": "Показать",
    "No programs": "Нет программ",
    "Stop all programs": "Остановить все программы",
    "Quit": "Выйти",
    "Click to open": "Нажмите, чтобы открыть",
    "Running — click to stop": "Работает — нажмите, чтобы остановить",
    "Click to start": "Нажмите, чтобы запустить",

    # UpdateChecker
    "Invalid response": "Непонятный ответ сервера",
    "No release found": "Релиз не найден",
    "No installer available for this OS": "Для этой системы нет установщика",
    "Cannot write to Downloads folder": "Не удалось записать в папку «Загрузки»",
}


def norm(s):
    for a, b in (("“", '"'), ("”", '"'), ("«", '"'), ("»", '"'),
                 ("…", "..."), ("—", "-"), ("–", "-")):
        s = s.replace(a, b)
    return s


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    ts = os.path.join(here, "ProcessLauncher_ru.ts")
    table = {norm(k): v for k, v in RU.items()}

    tree = ET.parse(ts)
    missing = []
    for ctx in tree.getroot().iter("context"):
        name = ctx.find("name").text
        for msg in ctx.iter("message"):
            src = msg.find("source").text or ""
            tr = table.get(norm(name + "|" + src), table.get(norm(src)))
            t = msg.find("translation")
            if t is None:
                t = ET.SubElement(msg, "translation")
            if tr is None:
                missing.append(f"{name}: {src}")
                t.text = None
                t.set("type", "unfinished")
            else:
                t.text = tr
                t.attrib.pop("type", None)

    tree.getroot().set("language", "ru_RU")
    ET.indent(tree, "    ")
    with open(ts, "w", encoding="utf-8", newline="\n") as f:
        f.write('<?xml version="1.0" encoding="utf-8"?>\n<!DOCTYPE TS>\n')
        f.write(ET.tostring(tree.getroot(), encoding="unicode"))
        f.write("\n")

    if missing:
        print("No translation for:")
        for m in missing:
            print("  " + m)
        sys.exit(1)
    print("All strings translated")


main()
