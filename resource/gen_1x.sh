#! /bin/bash

RESIZE="-resize"
#RESIZE="-adaptive-resize"

for filename in *_2x.png; do
  [ -e "$filename" ] || continue
  base=${filename%_2x.png}
#  echo "Processing $filename => ${base}.png"
  convert "$filename" $RESIZE "50%" "${base}.png"
  echo "jamba_add_vst3_resource(\${target} PNG \"${base}.png\")"
  echo "jamba_add_vst3_resource(\${target} PNG \"$filename\")"
done