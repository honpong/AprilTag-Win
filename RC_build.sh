#!/bin/bash
xcodebuild -workspace RC.xcworkspace -list | tail +3 | sed -e 's/^ *//' -e '/^$/d' | while read s; do
    echo -n $'\e[33m'"$s"$'\e[m' $'\e[34mBuilding\e[m' ...
    xcodebuild -workspace RC.xcworkspace -scheme "$s" -configuration Release \
               'GCC_PREPROCESSOR_DEFINITIONS=${GCC_PREPROCESSOR_DEFINITIONS} SDK_LICENSE_KEY=@\"C4c5C2Ae73eD8c3EdAAFDC75df12DC\"' > "/tmp/build-$s.out" && \
       echo $'\e[33m'"$s"$'\e[m' $'\e[32mBuilt\e[m' && rm "/tmp/build-$s.out"  || \
       echo $'\e[33m'"$s"$'\e[m' $'\e[31mFailed\e[m'. See "/tmp/build-$s.out" for details.;
done
