# Doom Runner

Doom Runner is yet another launcher of common Doom source ports (like GZDoom, Zandronum, Crispy Doom, PrBoom, ...) with graphical user interface. It is written in C++ and Qt, and it is designed around the idea of presets for various multi-file modifications (Brutal Doom with mutators, Project Brutality with UDV, Complex Doom Clusterfuck, ...) to allow one-click switching between them and minimize any repetitive work.

First you perform an initial setup, setting up the paths and adding all your Doom engines and IWADs.\
![](Screenshots/1-InitialSetup.png "Initial setup")

Then you create named presets for all your favourite modifications and assign them an engine, an IWAD and mod files.\
![](Screenshots/2-MainScreen.png "Main screen")

If you wish to play multiplayer or jump into a map directly, you can do so in the second tab.\
![](Screenshots/3-LaunchOptions.png "Launch options")

And now the launcher even has a dark theme!
![](Screenshots/6-DarkTheme.png "Dark theme")

### Features

* Unlimited number of named presets switchable with a single-click
* Lists of IWADs and Map packs are automatically synchronized with the content of a selected directory
* Drag&Drop for easy adding of files or re-ordering them
* Ability to quickly tick or untick any files to load without the need to remove and re-add them
* Double-clicking on an WAD file will display a content of an equally named TXT file, if it exists
* Ability to start a multiplayer game on LAN
* Ability to directly start selected map, load selected save or record/replay a demo
* DM flags and Compatibility flags calculator with tooltips from official ZDoom wiki
* Ability to automatically put save files, demo files or screenshots in a directory named after the selected preset
* Ability to filter the saved presets using a search phrase or regular expression
* Choice between light and dark theme that can follow system preferences

### Advantages over other launchers

* Compatible with most commonly used source ports:  
GZDoom, Zandronum, Chocolate Doom, Crispy Doom, PrBoom, Eternity, Woof, DSDA-Doom, ...  
... and support for more can be easily added if requested
* Runs on Windows, Linux and MacOS (it reportedly works even on Steam Deck, although that's not officially supported)
* Does not require any special runtime like .NET or JRE

### Disadvantages over other launchers

* Does not have any browser or downloader of known mods.  
You must find the mods, download them, and add them to the launcher by yourself.
* Cannot show public multiplayer servers.  
You can join a server only by entering its hostname or IP address.

### Project's state

The major development has ended. My passion for this project is dry and i have to sort out other things in my life.
I don't plan to add any big new features or do any major reworks. I'm only going to fix reported bugs and maybe do some small usability improvements.

However, the code is very well documented and anyone is welcome to take over and continue the development on his own.



## Download

Pre-compiled executables for different operating systems can be found on [release page](https://github.com/Youda008/DoomRunner/releases)

32-bit Linux build is currently not supported, because i don't have enough space on disk to install yet another operating system. But you can always build it yourself, on Linux it's pretty easy, see [HowToBuild.md](HowToBuild.md).

For Windows I only provide statically linked executable, because installing the Qt DLLs required to run this program is too much work and it's easier to have everything bundled together.



## How to install and run


### Windows

The `DoomRunner.exe` bundles everything inside itself. Just extract it into some directory and it is good to go. Putting it to `Program Files` is also supported, the data files will then be in `%AppData%\DoomRunner`.

#### Scoop

For users of [Scoop](https://scoop.sh/), thanks to [sitiom](https://github.com/sitiom) it is now available in the games bucket:
```
scoop bucket add games
scoop install doomrunner
```


### Linux

On Linux you have few options.

#### Install manually

Extract the executable in where you usually put them, commonly `/usr/bin` or `/opt/DoomRunner`.

Then you need to install the following shared libraries to make the app run.

* `libqt6core`
* `libqt6gui`
* `libqt6widgets`
* `libqt6network`

Please note that in different distributions the libraries might have slightly different names, for example in Ubuntu 24 LTS they are called `libqt6core6t64`, `libqt6gui6`, `libqt6widgets6`, ...

Use your distribution package manager to find and install those

* Ubuntu: `sudo apt install libname`
* Arch: `pacman -S libname`
* ...
* or use some graphical package manager like for example Synaptic Package Manager

The executable was built in Kubuntu 24.04 LTS so there is a chance that it will not find the libraries in other distributions. In that case you will need to build it yourself.

#### Install via Flatpak

Doom Runner can be installed via [Flatpak](https://flatpak.org/) from this link:

[![Download Doom Runner](https://dl.flathub.org/assets/badges/flathub-badge-en.png 'Download Doom Runner')](https://flathub.org/apps/io.github.Youda008.DoomRunner)

See [Flathub README](https://github.com/flathub/io.github.Youda008.DoomRunner#readme) for more informations.

#### Install from AUR

Arch Linux users can leverage [AUR](https://wiki.archlinux.org/title/Arch_User_Repository) to install Doom Runner via package manager from this [package](https://aur.archlinux.org/packages/doomrunner).
The AUR wiki page contains instructions how to do it.

#### Install on NixOS

Thanks to [Keenan Weaver](https://github.com/keenanweaver), NixOS users can install Doom Runner [package](https://search.nixos.org/packages?channel=unstable&show=doomrunner&from=0&size=50&sort=relevance&type=packages&query=doomrunner) using the following instructions.

##### via the package manager
```
nix-env -iA nixpkgs.doomrunner
```

##### via the configuration file

1. Add one of the following to your configuration.nix
```
  environment.systemPackages = with pkgs; [
    doomrunner
  ];
```
or
```
  users.users.yourUserName.packages = with pkgs; [
    doomrunner
  ];
```

2. Rebuild the system with `nixos-rebuild`



## How to build (and develop)

Instructions for building and developing this application are now in [HowToBuild.md](HowToBuild.md).


## Reporting issues and requesting features

If you encouter a bug or just want the launcher to work differently, you can either create an issue here on github (the preferred way) or reach me on email youda008@gmail.com or on Discord as Youda008.
