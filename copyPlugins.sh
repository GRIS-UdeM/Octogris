#!/bin/bash

VERSION="$1"

pushd ~/Library/Audio/Plug-Ins/Components/
zip -r ~/Desktop/Octogris$VERSION.zip ./Octogris2.component 
popd

pushd ~/Library/Audio/Plug-Ins/VST/
zip -r ~/Desktop/Octogris$VERSION.zip ./Octogris2.vst 
popd

#zip -rj ~/Desktop/ZirkOSC.zip ~/Library/Audio/Plug-Ins/Components/ZirkOSC3.component ~/Library/Audio/Plug-Ins/VST/ZirkOSC3.vst 

echo "Created zip file Octogris$VERSION.zip"
