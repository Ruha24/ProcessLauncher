; ============================================================
;  Inno Setup script для ProcessLauncher
; ============================================================

#define MyAppName "Process Launcher"
; Версия задаётся в одном месте — в CMakeLists.txt (project VERSION).
; Сборка передаёт её сюда: iscc /DMyAppVersion=1.5 installer.iss
#ifndef MyAppVersion
  #error Version is not set. Run: iscc /DMyAppVersion=<version> installer.iss
#endif
#define MyAppPublisher "ruha42"
#define MyAppURL "https://github.com/Ruha24/ProcessLauncher"
#define MyAppExeName "ProcessLauncher.exe"

[Setup]
AppId={{B7A3F2E1-4C5D-4E6F-9A1B-2C3D4E5F6A7B}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}/issues
AppUpdatesURL={#MyAppURL}/releases
VersionInfoVersion={#MyAppVersion}
UninstallDisplayIcon={app}\{#MyAppExeName}
; При обновлении поверх работающей копии закрываем её (она сидит в трее и
; на обычную просьбу закрыться может просто спрятать окно).
CloseApplications=force
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputBaseFilename=ProcessLauncher-Setup-{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupIconFile={#SourcePath}\..\..\icons\app.ico
PrivilegesRequired=lowest
ArchitecturesInstallIn64BitMode=x64compatible

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourcePath}\dist\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
