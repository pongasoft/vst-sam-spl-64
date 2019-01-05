SAM-SPL 64 Plugin
=================
This VST2/3 (and AudioUnit) plugin lets you easily split a sample in up to 64 slices.

* Use the pads in the plugin or a MIDI keyboard (slices automatically mapped to notes)
* 2 play modes: hold or trigger
* Play a single slice at a time or multiple (polyphonic)
* Each slice can be set to loop and/or play in reverse
* Edit the sample (Trim, Cut, Crop, Normalize (0dB, -3dB, -6dB), Resample)
* Continuous smooth zoom all the way to the sample level
* Show zero crossing
* Load a sample from the file system or directly sample an input (built-in sampler)
* Export the sample as a file (wav / aiff supported)

Release Notes
-------------
At this stage, the plugin is still in development and is released only in Debug build for input and feedback.

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

Licensing
---------
GPL version 3
