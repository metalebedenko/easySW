#define MyAppName "EasySW"
#define MyAppExeName "EasySW.exe"

[Setup]
AppId={{8E036AA6-B5C0-4B32-BF07-64D08A31FB98}
AppName={#MyAppName}
AppVersion=0.1.0
DefaultDirName={localappdata}\Programs\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
Compression=lzma
SolidCompression=yes
OutputDir=dist
OutputBaseFilename=EasySW-Setup
WizardStyle=modern
UninstallDisplayIcon={app}\{#MyAppExeName}

[Files]
Source: "..\build\Release\EasySW.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\EasySW"; Filename: "{app}\{#MyAppExeName}"
Name: "{userdesktop}\EasySW"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Flags: unchecked

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch EasySW"; Flags: nowait postinstall skipifsilent
