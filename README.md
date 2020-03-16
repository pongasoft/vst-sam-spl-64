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
### 2020-03-16 - `v1.2.0`
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
Check the Jamba [README](https://github.com/pongasoft/jamba/blob/master/README.md) file for instructions on how to install and configure the VST3 SDK.

Build this project
------------------

The following steps describes (for each platform) how to build the plugin.

### macOS

- For simplicity I am creating the build at the root of the source tree, but can obviously be *outside* the source tree entirely by running the script from anywhere

        ./configure.sh
        cd build

- In order to build, test, validate, etc... simply use the `jamba.sh` script (use `-h` for details):

         ./jamba.sh -h

### Windows

- For simplicity I am creating the build at the root of the source tree, but can obviously be *outside* the source tree entirely by running the script from anywhere. Note that PowerShell is highly recommended.

        .\configure.bat
        cd build

- In order to build, test, validate, etc... simply use the `jamba.bat` script (use `-h` for details):

         .\jamba.bat -h

Misc
----

- This project uses [loguru](https://github.com/emilk/loguru) for logging.
- This project uses [libsndfile](https://github.com/erikd/libsndfile) for sound file loading/saving.
- This project uses [r8brain-free-src](https://github.com/avaneev/r8brain-free-src) for rate converter/resampling (designed by Aleksey Vaneev of Voxengo)
- This project uses [Abduction 2002 font](https://www.pizzadude.dk) by Jakob Fischer.

Licensing
---------
GPL version 3
