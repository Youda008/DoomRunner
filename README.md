# Doom Runner
Doom Runner is yet another graphical interface launcher of ZDoom and derivatives, written in C++ using Qt-5.12 framework and Qt Creator. It is designed around the idea of presets for various multi-file modifications (Brutal Doom with mutators, Project Brutality with UDV, Complex Doom Clusterfuck, ...) to allow one-click switching between them and minimize any repetitive work.

First you perform an initial setup, setting up the paths and adding all your Doom engines and IWADs. 
![](Screenshots/1-InitialSetup.png "Initial setup")

Then you create named presets for all your favourite modifications and assign them an engine, an IWAD and mod files.
![](Screenshots/2-MainScreen.png "Main screen")

And if you wish to play multiplayer or jump into a map directly, you can do so in the second tab.
![](Screenshots/3-LaunchOptions.png "Launch options")

You can even configure gameplay flags and compatibility flags.
![](Screenshots/4-DMflags.png "DM flags")

The map list and IWAD list are automatically updated from selected directory, so everytime you download a new WAD, you don't need to manually add it to the launcher. The mod list supports drag&drop from directory window and internal drag&drop for reordering the files.

## Status
Currently the launcher is still work-in-progress. Some features are not finished yet and the build is tested only on Windows. A testing version is available below and I will be glad for any feedback, suggestions and bug reports. You can either create an issue here on Github, or write me an email youda008@seznam.cz or contact me on Discord as Youda#0008.

## Download (beta version)
Currently i have pre-built executables only for Windows. There is a statically linked version and dynamically linked version. The static one is bigger, but it's a standalone portable executable that should run on any computer. Download the dynamic version ONLY if you already have Msys2 with Qt installed, otherwise grab the static one.

Download it [here](https://github.com/Youda008/DoomRunner/releases)
