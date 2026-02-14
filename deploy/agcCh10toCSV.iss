; agcCh10toCSV Inno Setup Script
; Generates an installer for v2.3.0
; Requires Inno Setup 6.x (https://jrsoftware.org/isinfo.php)
;
; Build the installer from the command line:
;   iscc deploy\agcCh10toCSV.iss
;
; Or open this file in Inno Setup Compiler and press Ctrl+F9.

#define MyAppName "agcCh10toCSV"
#define MyAppVersion "2.3.0"
#define MyAppPublisher "agcCh10toCSV"
#define MyAppExeName "agcCh10toCSV.exe"
#define StagingDir "staging\installer"

[Setup]
AppId={{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir=.
OutputBaseFilename=agcCh10toCSV_v{#MyAppVersion}_setup
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
LicenseFile=..\LICENSE
SetupIconFile=..\resources\icon.ico
UninstallDisplayIcon={app}\bin\{#MyAppExeName}
PrivilegesRequiredOverridesAllowed=dialog
WizardStyle=modern
MinVersion=10.0.17763

; Code signing — set SIGN_CERT_PATH / SIGN_CERT_PASS env vars, then uncomment:
; SignTool=signtool sign /f "$SIGN_CERT_PATH" /p "$SIGN_CERT_PASS" /tr http://timestamp.digicert.com /td sha256 /fd sha256 $f

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional icons:"; Flags: unchecked

[Files]
; Application binaries and Qt dependencies (from bin/ staging)
Source: "{#StagingDir}\bin\*"; DestDir: "{app}\bin"; Flags: ignoreversion recursesubdirs createallsubdirs

; Settings files — preserve existing user INI on upgrade
Source: "{#StagingDir}\settings\default.ini"; DestDir: "{app}\settings"; Flags: onlyifdoesntexist uninsneveruninstall
Source: "{#StagingDir}\settings\RASA.ini"; DestDir: "{app}\settings"; Flags: onlyifdoesntexist uninsneveruninstall skipifsourcedoesntexist
Source: "{#StagingDir}\settings\TRC.ini"; DestDir: "{app}\settings"; Flags: onlyifdoesntexist uninsneveruninstall skipifsourcedoesntexist

; New default.ini always installed as default_new.ini for merge comparison
Source: "{#StagingDir}\settings\default.ini"; DestDir: "{app}\settings"; DestName: "default_new.ini"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\bin\{#MyAppExeName}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\bin\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\bin\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent

[Code]
(* ----------------------------------------------------------------
   INI Merge Logic
   Migrates user settings from an existing default.ini into the
   newly installed default_new.ini, then backs up the old file.
   ---------------------------------------------------------------- *)

const
  SETTINGS_SUBDIR = 'settings';
  DEFAULT_INI     = 'default.ini';
  DEFAULT_NEW_INI = 'default_new.ini';
  DEFAULT_OLD_INI = 'default_old.ini';

(* Migrate a single key from old INI to new INI if it exists in the old file. *)
procedure MigrateKey(const Section, Key, OldFile, NewFile: string);
var
  Value: string;
begin
  Value := GetIniString(Section, Key, '', OldFile);
  if Value <> '' then
    SetIniString(Section, Key, Value, NewFile);
end;

(* Return the channel prefix for a given channel index (0=L, 1=R, 2=C, etc.). *)
function ChannelPrefix(Index: Integer): string;
begin
  case Index of
    0: Result := 'L';
    1: Result := 'R';
    2: Result := 'C';
  else
    Result := 'CH' + IntToStr(Index);
  end;
end;

procedure MergeIniFiles();
var
  SettingsDir, OldPath, NewPath, BackupPath: string;
  OldReceiverCount, OldChannelsPerRcvr: Integer;
  I, J: Integer;
  SectionName: string;
begin
  SettingsDir := ExpandConstant('{app}\') + SETTINGS_SUBDIR;
  OldPath     := SettingsDir + '\' + DEFAULT_INI;
  NewPath     := SettingsDir + '\' + DEFAULT_NEW_INI;
  BackupPath  := SettingsDir + '\' + DEFAULT_OLD_INI;

  (* Only merge if both the existing default.ini and the new default_new.ini exist *)
  if (not FileExists(OldPath)) or (not FileExists(NewPath)) then
  begin
    (* Fresh install — just rename default_new.ini to default.ini *)
    if (not FileExists(OldPath)) and FileExists(NewPath) then
      RenameFile(NewPath, OldPath);
    Exit;
  end;

  (* --- Migrate known settings sections --- *)
  MigrateKey('Frame', 'FrameSync', OldPath, NewPath);

  MigrateKey('Parameters', 'Polarity', OldPath, NewPath);
  MigrateKey('Parameters', 'Slope',    OldPath, NewPath);
  MigrateKey('Parameters', 'Scale',    OldPath, NewPath);

  MigrateKey('Time', 'ExtractAllTime', OldPath, NewPath);
  MigrateKey('Time', 'SampleRate',     OldPath, NewPath);

  MigrateKey('Receivers', 'Count',              OldPath, NewPath);
  MigrateKey('Receivers', 'ChannelsPerReceiver', OldPath, NewPath);

  (* --- Migrate parameter sections (e.g., [L_RCVR1], [R_RCVR1], ...) --- *)
  OldReceiverCount   := GetIniInt('Receivers', 'Count', 16, 1, 16, OldPath);
  OldChannelsPerRcvr := GetIniInt('Receivers', 'ChannelsPerReceiver', 3, 1, 48, OldPath);

  for I := 1 to OldReceiverCount do
  begin
    for J := 0 to OldChannelsPerRcvr - 1 do
    begin
      SectionName := ChannelPrefix(J) + '_RCVR' + IntToStr(I);
      MigrateKey(SectionName, 'Word', OldPath, NewPath);
    end;
  end;

  (* --- Back up old INI and activate the merged new one --- *)
  if FileExists(BackupPath) then
    DeleteFile(BackupPath);
  RenameFile(OldPath, BackupPath);
  RenameFile(NewPath, OldPath);

  MsgBox('Your settings have been migrated to the updated default.ini.' + #13#10 +
         'The previous configuration was saved as ' + DEFAULT_OLD_INI + '.',
         mbInformation, MB_OK);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
    MergeIniFiles();
end;
