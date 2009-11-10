; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{204D48C5-6231-4955-83EC-623DCB437FD9}
;AppMutex=SecondLifeAppMutex
AppName=GreenLife Emerald Viewer Skins Pack v2
AppVerName=GreenLife Emerald Skins v2
AppPublisher=ModularSystems.sl
AppPublisherURL=http://modularsystems.sl/
AppSupportURL=http://modularsystems.sl/
AppUpdatesURL=http://modularsystems.sl/
DefaultDirName={pf}\GreenLife Emerald Viewer
DefaultGroupName=GreenLife Emerald Viewer
OutputDir=C:\emerald-setup
OutputBaseFilename=GreenLife_SKINS_v2-lgg
Compression=lzma
SolidCompression=yes
PrivilegesRequired=poweruser
LicenseFile=C:\emerald-setup\skinslicense.txt
MinVersion=0,5.00sp4
SetupIconFile=C:\emerald-setup\Icons\EmeraldInstallIcon3.ico
DirExistsWarning=no
Uninstallable=no



[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Messages]
WinVersionTooLowError=This program requires Windows 2000 SP4 or later.
WizardSelectDir=Please Select Your Green Life Root Folder
SelectDirDesc=Where should [name] be installed?  (If you already installed GreenLife, this should already be set correctly)


[Tasks]
;Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "C:\emerald-run-skins\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs
;Source: "C:\emerald-run-skins\skins\*"; DestDir: "{app}\skins"; Flags: ignoreversion recursesubdirs sortfilesbyextension

[Registry]
;Root: HKLM; Subkey: "Software\ModularSystems.sl"; Flags: uninsdeletekeyifempty
;Root: HKLM; Subkey: "Software\ModularSystems.sl\GreenLife Emerald Viewer Skins"; Flags: uninsdeletekey
;Root: HKLM; Subkey: "Software\ModularSystems.sl\GreenLife Emerald Viewer\Settings"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"

[InstallDelete]
;Type: files; Name: "{app}\fmod.dll"
;Type: files; Name: "{app}\llkdu.dll"
;Type: files; Name: "{app}\SLVoice.exe"
;Type: files; Name: "{app}\vivoxsdk.dll"
;Type: files; Name: "{app}\alut.dll"
;Type: files; Name: "{app}\ortp.dll"
;Type: files; Name: "{app}\wrap_oal.dll"

[UninstallDelete]
;Type: files; Name: "{app}\skins\*"

[Icons]
;Name: "{commondesktop}\GreenLife Emerald (white skin)"; Parameters: "--skin white_emerald --channel ""GreenLife Emerald Viewer"" --settings settings_emerald.xml -set SystemLanguage en-us"; Filename: "{app}\GreenLife.exe"; WorkingDir: "{app}"; Tasks: desktopicon;  IconFilename: "{app}\GreenLife.exe"

[Run]
;Filename: "{tmp}\vcredist_x86.exe"; Parameters: "/Q"; Description: "Visual C++ 2005 SP1 Runtime"; StatusMsg: "Installing Visual C++ runtime";
;Filename: "{app}\propcopy.exe"; Description: "Copy proprietary files from existing Second Life installation"; WorkingDir: "{app}"; Flags: skipifsilent runascurrentuser
;Filename: "{app}\GreenLife.exe"; Parameters: "--channel ""GreenLife Emerald Viewer"" --settings settings_emerald.xml -set SystemLanguage en-us"; Description: "{cm:LaunchProgram,GreenLife Emerald Viewer}"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent

