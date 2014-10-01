#!/bin/bash

rejuce()
{
rm -rf Builds;
./Introjucer.app/Contents/MacOS/Introjucer --resave "$1".jucer;
}

build()
{
xcrun xcodebuild -project Builds/MacOSX/"$1".xcodeproj -configuration Release;
mv ~/Library/Audio/Plug-Ins/Components/"$name".component Plugins/AUs;
mv ~/Library/Audio/Plug-Ins/VST/"$name".vst Plugins/VSTs;
}

rm -rf Plugins;
mkdir Plugins;
mkdir Plugins/AUs;
mkdir Plugins/VSTs;

#variants=( 1x2 2x2 1x4 2x4 4x4 1x6 2x6 6x6 1x8 2x8 8x8 1x12 2x12 12x12 1x32 2x32 6x32 32x32 );
#variants=( 2x2 1x8 2x8 8x8 );
variants=( 1x2 2x2 1x4 2x4 4x4 1x6 2x6 4x6 6x6 1x8 2x8 4x8 6x8 8x8 1x16 2x16 4x16 6x16 8x16 );

for i in "${variants[@]}"
do
   :
   params=( $( echo "$i" | tr "x" " " ) );
   sources=${params[0]}
   speakers=${params[1]}
   name="Octogris ${sources}x${speakers}";
   echo "Processing variant: $name";

   cp Octogris.jucer "$name".jucer;
   
   sed -e s/'pluginChannelConfigs="{2,8}"'/'pluginChannelConfigs="{'"$sources"','"$speakers"'}"'/g -i "" "$name".jucer;
   sed -e s/'pluginName="Octogris"'/'pluginName="'"$name"'"'/g -i "" "$name".jucer;
   sed -e s/'name="Octogris"'/'name="'"$name"'"'/g -i "" "$name".jucer;
   sed -e s/'targetName="Octogris"'/'targetName="'"$name"'"'/g -i "" "$name".jucer;
   sed -e s/'pluginCode="UOct"'/'pluginCode="U'"$sources"'x'"$speakers"'"'/g -i "" "$name".jucer;
   
   rejuce "$name";
   build "$name";

done
