#! /bin/bash

# The purpose of this script is to automatically generate the 1x resolution images from the 2x images
# It assumes ImageMagick is installed and available from the path
# It needs to be run from the directory where it lives
# It displays the entries for CMakeLists.txt so that it can be copy/pasted

RESIZE="-resize"
#RESIZE="-adaptive-resize"

for filename in *_background_2x.png; do
  [ -e "$filename" ] || continue
  base=${filename%_2x.png}
#  echo "Processing $filename => ${base}.png"
  convert "$filename" $RESIZE "50%" "${base}.png"
done

# Display CMakeLists.txt so that it can be copy/pasted
for filename in *_2x.png; do
  [ -e "$filename" ] || continue
  base=${filename%_2x.png}
  echo "jamba_add_vst3_resource(\${target} PNG \"${base}.png\")"
  echo "jamba_add_vst3_resource(\${target} PNG \"$filename\")"
done