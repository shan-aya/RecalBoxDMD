; RecalBoxDMD Toolkit - Inno Setup installer script
; Compile with: ISCC.exe RecalBoxDMD_Setup.iss
; Requires dist\RecalBoxDMD_GUI.exe to already be built (PyInstaller, see RecalBoxDMD_GUI.spec).

#define MyAppName "RecalBoxDMD Toolkit"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Shan_ayA"
#define MyAppURL "https://github.com/shan-aya/RecalBoxDMD"
#define MyAppExeName "RecalBoxDMD_GUI.exe"

[Setup]
AppId={{6E9F6E7A-7C7B-4F5E-9B0D-3A2C6E8F1D44}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=..\..\LICENSE
OutputDir=dist_installer
OutputBaseFilename=RecalBoxDMD_Toolkit_Setup
SetupIconFile=assets\recalboxdmd_icon.ico
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "dist\RecalBoxDMD_GUI.exe"; DestDir: "{app}"; DestName: "{#MyAppExeName}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
