# Mario Maker 2 RNG Mod
A mod for MM2 version 3.0.3 to display and manipulate RNG. Built using [LibHakkun](https://github.com/fruityloops1/LibHakkun).
## Features
 - Log all RNG events with the format \[{subworld}\] \{X value\} \(\{RNG value\}\) \{ID\}
     - E.g. \[0\] 2 \(4807714d\) Blooper
 - Set the initial X value of the RNG using a config file
 - A lookup table that converts raw IDs like "096057a8" to nice names like "Blooper" and can be changed using a config file
 - Ability to reload config files using button combos
## Installation
Follow the typical installation process for ExeFS mods. Usually involves copying main.npdm and subsdk4 to the correct directory. A prebuilt version of the mod can be found in Releases. To install the config files see the below config section.
## Config Files
All config files should be created in the root directory of the SD card.
### rng.txt
The content of this file should be the X value to initialize the RNG with. If it is missing it will start at X = 1. For the RNG to start at X = 42:
```
42
```
### rngnames.txt
This file is used as a lookup table to convert raw IDs to nicer names. The format is multiple lines of {raw ID}={nice name}. The maximum length of a name is 20 characters. If this file is missing all IDs will be displayed in raw format. An example table:
```
09af6520=Mario Neutral
09af6a08=Mario Walking
0978b12c=Load 1
0978b184=Load 2
0978b1ac=Load 3
096057a8=Blooper
0986da70=Coin Fall
0986dbf8=Coin Bounce
```
## Controls
For these button combos to work you may need to be using a Pro Controller.
 - Left stick button \+ Right stick button: Reload rng.txt
 - Plus button \+ Minus button: Reload rngnames.txt
## Build
See [LibHakkun](https://github.com/fruityloops1/LibHakkun) for the prerequisites and how to set up the library.
 - `mkdir build`
 - `cd build`
 - `cmake .. -DCMAKE_BUILD_TYPE=Release -GNinja` for Ninja or `cmake .. -DCMAKE_BUILD_TYPE=Release` for make
 - `ninja` or `make` respectively
 ## License
 The LICENSE file applies to all files unless otherwise noted.