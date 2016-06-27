# PhDoom

PhDoom is a Doom source port designed to run under the Photon microGUI on a QNX 4.25 OS.

# Important notes

* No sound support.
* PhDoom will try and set system tick size to 2 msec in order to achieve
acceptable frame rate. Therefore, you have to run the program as root,
otherwise, set tick size manually.

# Using

To see a full list of all available command line options type
```
use phdoom
```
 To play, an IWAD file is needed. Place your IWAD file in in one of the following locations:

 1. Directory listed in the **DOOMWADDIR** environment  variable
 2. **/usr/local/share/doom**
 3. **Current working directory**
