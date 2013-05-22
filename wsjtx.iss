[Setup]
AppName=wsjtx
AppVerName=wsjtx Version 0.99 r3295
AppCopyright=Copyright (C) 2001-2013 by Joe Taylor, K1JT
DefaultDirName=c:\wsjtx
DefaultGroupName=wsjtx

[Files]
Source: "c:\Users\joe\wsjt\wsjtx_install\wsjtx.exe";         DestDir: "{app}"
Source: "c:\Users\joe\wsjt\wsjtx_install\jt9.exe";           DestDir: "{app}"
Source: "c:\Users\joe\wsjt\wsjtx\lib\jt9code.exe";           DestDir: "{app}"
Source: "c:\Users\joe\wsjt\wsjtx_install\rigctl.exe";        DestDir: "{app}"
Source: "c:\Users\joe\wsjt\wsjtx_install\wsjt.ico";          DestDir: "{app}"
Source: "c:\Users\joe\wsjt\wsjtx_install\afmhot.dat";        DestDir: "{app}";
Source: "c:\Users\joe\wsjt\wsjtx_install\blue.dat";          DestDir: "{app}";
Source: "c:\Users\joe\wsjt\wsjtx_install\CALL3.TXT";         DestDir: "{app}";  Flags: onlyifdoesntexist
Source: "c:\Users\joe\wsjt\QtSupport\*.dll";                 DestDir: "{app}";
Source: "c:\Users\joe\wsjt\wsjtx\shortcuts.txt";             DestDir: "{app}"
Source: "c:\Users\joe\wsjt\wsjtx\mouse_commands.txt";        DestDir: "{app}"
Source: "c:\Users\joe\wsjt\wsjtx\WSJT-X_Users_Guide.pdf";    DestDir: "{app}"
Source: "c:\Users\joe\wsjt\wsjtx_install\save\Samples\130418_1742.wav";   DestDir: "{app}\save\Samples";

[Icons]
Name: "{group}\wsjtx";        Filename: "{app}\wsjtx.exe";   WorkingDir: {app}; IconFilename: {app}\wsjt.ico
Name: "{userdesktop}\wsjtx";  Filename: "{app}\wsjtx.exe";   WorkingDir: {app}; IconFilename: {app}\wsjt.ico

