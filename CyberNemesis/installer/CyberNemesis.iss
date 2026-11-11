#if !FileExists(SourcePath + "..\release\CyberNemesis.exe")
  #error Falta release\CyberNemesis.exe. Ejecuta primero build_windows.bat (o descarga el .exe desde GitHub Actions y copialo en la carpeta release).
#endif

[Setup]
AppName=CYBER-NEMESIS
AppVersion=1.0.0
DefaultDirName={autopf}\CyberNemesis
DefaultGroupName=CYBER-NEMESIS
OutputBaseFilename=CyberNemesis-Setup
OutputDir=..\release
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
SetupIconFile=..\assets\windows\icon.ico
UninstallDisplayIcon={app}\CyberNemesis.exe
WizardStyle=modern

[Files]
Source: "..\release\CyberNemesis.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\LEEME.txt"; DestDir: "{app}"; Flags: ignoreversion isreadme

[Icons]
Name: "{group}\CYBER-NEMESIS"; Filename: "{app}\CyberNemesis.exe"
Name: "{autodesktop}\CYBER-NEMESIS"; Filename: "{app}\CyberNemesis.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Crear acceso directo en el escritorio"

[Run]
Filename: "{app}\CyberNemesis.exe"; Description: "Jugar ahora"; Flags: nowait postinstall skipifsilent
