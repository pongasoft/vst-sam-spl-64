Install instructions
====================

macOS (64 bits)
---------------
For VST2, copy SAM-SPL 64.vst3 and RENAME into SAM-SPL 64.vst under
  - $HOME/Library/Audio/Plug-ins/VST for a single user
  - or /Library/Audio/Plug-ins/VST for all users (may require admin access)
  - or any DAW specific path (64bits)
MAKE SURE TO RENAME the file otherwise it will not work

For VST3, copy SAM-SPL 64.vst3 under
  - $HOME/Library/Audio/Plug-ins/VST3 for a single user
  - or /Library/Audio/Plug-ins/VST3 for all users (may require admin access)
  - or any DAW specific path (64bits)

For Audio Unit, copy SAM-SPL 64.component under
  - $HOME/Library/Audio/Plug-ins/Components for a single user
  - or /Library/Audio/Plug-ins/Components for all users (may require admin access)
  - Note: you may have to reboot your computer for the Audio Unit to appear in Logic
          (or kill the AudioComponentRegistrar process(es))

!!!!!!!!!!!!!!! WARNING WARNING WARNING !!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!! WARNING WARNING WARNING !!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!! WARNING WARNING WARNING !!!!!!!!!!!!!!!!

This is a demo/debug build and is being released for feedback and testing. There is no guarantee that future versions
of this plugin will remain compatible with this one (meaning you could loose the plugin settings entirely). Do not
rely on it for actual production work.

Note: Because it is a debug build, logging is turned on. In order to see the logs, you can start your DAW from a shell
terminal, for example /Applications/Reason\ 9/Reason.app/Contents/MacOS/Reason to start Reason.

!!!!!!!!!!!!!!! WARNING WARNING WARNING !!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!! WARNING WARNING WARNING !!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!! WARNING WARNING WARNING !!!!!!!!!!!!!!!!