# UMD File Renamer

## Qt rebuild
This repository now includes an in-progress C++/Qt rebuild under `qt/` for the PSP ISO renaming workflow. The Qt app reads `.iso` images, extracts metadata from `PSP_GAME/PARAM.SFO`, optionally loads `PSP_GAME/ICON0.PNG`, and renames files to a normalized title plus disc ID format.

The Qt version currently targets `.iso` files only. Legacy `.cso` handling from the original Java application is intentionally not carried over.

### Windows build
Validated toolchain:
- CMake: `C:/Program Files/CMake/bin/cmake.exe`
- Qt: `C:/Qt/6.10.2/msvc2022_64`
- Visual Studio 2022 Build Tools: `C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools`

Quick build from the repo root:

```powershell
.\build.ps1
```

Or double-click `build.bat` to build and open the `qt\build` folder when it succeeds.

Build from PowerShell with a clean rebuild:

```powershell
Set-Location qt
.\build.ps1 -Clean
```

To deploy the Qt runtime next to the executable:

```powershell
Set-Location qt
.\build.ps1 -Clean -Deploy
```

The executable is produced at `qt/build/UMD_FileRenamer.exe`.

## Original description
This application reads ISO or CSO files and extract their information to generate a recognized and standard file name, without changing the properties inside the PSP games. Some classes of jpcsp project are used, jpcsp is an open source Java emulator of PlayStation Portable console system. See http://jpcsp.org/ for more information, code can be downloaded from http://code.google.com/p/jpcsp/. This application is partially based and inspired on [UmdBrowser](http://code.google.com/p/jumdbrowser/), an application to list and navigate into UMD images.

The code is written in Java using the latest Swing Layout library, so this desktop application can use the OS native interface (tested on Mac OSX, Windows and Linux/Ubuntu) and run on all of them. It comes in 2 languages: English and Spanish, you are able to choose it when the app starts. The IDE used to create this project was Netbeans 7.1, but it is a Maven project so you are able to open this project in any IDE that supports Maven. If you want to see a detailed log of the actions of the app, run it from a terminal (or console) and check the outputs (log4j was implemented for this purpose).

Java JRE 6+ is required. All required libraries are included to be more portable.
