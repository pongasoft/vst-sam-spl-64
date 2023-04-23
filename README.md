SAM-SPL 64 Plugin
=================
This VST2, VST3 and AudioUnit plugin lets you easily split a sample in up to 64 slices.

* Use the pads in the plugin or a MIDI keyboard (slices automatically mapped to notes)
* 2 play modes: hold or trigger
* Play a single slice at a time or multiple (polyphonic)
* Each slice can be set to loop and/or play in reverse
* Edit the sample (Trim, Cut, Crop, Normalize (0dB, -3dB, -6dB), Resample)
* Continuous smooth zoom all the way to the sample level
* Show zero crossing
* Load a sample from the file system or directly sample an input (built-in sampler)
* Drag and drop a sample in the waveform view
* Export the sample as a file (wav / aiff supported)

Check the [SAM-SPL 64](https://pongasoft.com/vst/SAM-SPL64.html) documentation for further details.

Release Notes
-------------
### 2023-04-23 - `v1.5.0`
* Added support for MP3, Flag, Ogg Vorbis (in addition to WAV and AIFF)
* Added error message when issue with loading file
* Fixed sound issues with Bitwig Studio
* Fixed drag and drop bug
* Migrated to Jamba 6.2.0 / VST SDK 3.7.5

### 2021-01-04 - `v1.4.3`
* Minor change to ensure memory deletion in UI happens outside the critical section
* Added support for Apple Chipset / universal build on macOS

### 2020-12-07 - `v1.4.2`
* Migrated to Jamba 5.1.1 / VST SDK 3.7.1
* Using shared buffers to minimize memory usage and avoid loading the sample over and over
* Scrollbar overview now displays the stereo sample

### 2020-08-03 - `v1.4.1`
* Improved performance when loading a large file (especially on Windows 10)
* Display a warning dialog if loading a large file to allow to continue or cancel

### 2020-04-27 - `v1.4.0`
* Added slices display in the main waveform (Play tab)
* Improved performances (less disk access)
* Fixed several issues when file sample rate doesn't match DAW sample rate
* Some minor UI tweaks

### 2020-03-30 - `v1.3.0`
* The number of slices can now vary smoothly between 1 and 64 thus allowing higher precision
  * A new LCD window displays the number of slices (and as a shortcut, you can click and drag on the window to change the value)
  * Up and down arrows let you change the number of slices in whole numbers
  * A knob on the edit tab lets you freely change the number of slices
  * Shortcuts for 1/16/32/48/64 slices can be used
  * Thanks to Denis Safiullin for suggesting the feature (based on the behavior of the Akai MPC 1000)

### 2020-03-18 - `v1.2.0`
* Added Settings tab
* Moved Root Key to Settings tab
* Added Cross Fade setting (on by default) to avoid pops and clicks
* Added Routing setting to handle mono file (mono -> mono or mono -> stereo)

Note that Cross Fade and Stereo Routing are on by default because this is what makes the most sense for the vast majority of use cases. As a result, projects using this device may sound slightly different after this update. Disabling cross fade and/or stereo routing will revert to the behavior prior to this update. 

### 2020-02-09 - `v1.1.0`
* Added "follow" option so that the selected slice follows MIDI input (ex: via MIDI keyboard)
* Added "Quick Edit Mode" to be able to quickly and conveniently see/configure the settings of each slice (loop/reverse)
* Added "Loop" action to loop on/off all slices at once (note that if not all slices are looped, then hitting "Loop" will make them all loop)
* Added "Reset" action to resets the settings (loop/reverse) of all slices at once

### 2019-09-19 - `v1.0.0`
* first public release / free / open source

Configuration and requirements
------------------------------
Check the Jamba [requirements](https://jamba.dev/requirements/) for instructions on how to install and configure the VST3 SDK.

Build this project
------------------

The following steps describes how to build the plugin: 

1. Invoke the `configure.py` python script to configure the project
2. Run the `jamba.sh` (resp. `jamba.bat`) command line to build, test validate...

### macOS

- For simplicity I am creating the build at the root of the source tree, but can obviously be *outside* the source tree entirely by running the script from anywhere

        > ./configure.py -h # to see the help
        > ./configure.py
        > cd build
        > ./jamba.sh -h

### Windows

- For simplicity I am creating the build at the root of the source tree, but can obviously be *outside* the source tree entirely by running the script from anywhere. Note that PowerShell is highly recommended.

        > python configure.py -h # to see the help
        > python configure.py
        > cd build
        > .\jamba.bat -h

Misc
----

- This project uses [loguru](https://github.com/emilk/loguru) for logging.
- This project uses [libsndfile](https://github.com/erikd/libsndfile) for sound file loading/saving.
- This project uses [r8brain-free-src](https://github.com/avaneev/r8brain-free-src) for rate converter/resampling (designed by Aleksey Vaneev of Voxengo)
- This project uses [Abduction 2002 font](https://www.pizzadude.dk) by Jakob Fischer.

Licensing
---------
GPL version 3
