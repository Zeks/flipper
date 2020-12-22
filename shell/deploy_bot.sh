#!/bin/bash
dirname=$PWD
echo "in dir " $dirname
deployfolder="deployment"

CopyFilesToFolder () {
	arrayName=$1[@]
	actualArray=("${!arrayName}")
	for filename in "${actualArray[@]}"
	do
		cp -f $2/$filename$4 $3/$filename$4
	done
}

source $dirname/shell/deploy_entry_check.sh $dirname/release/discord

if [ $entryResult == false ]
then
	echo 'Entry check failed, exiting'
	exit 1
fi

rm -rf $dirname/$deployfolder 
mkdir -p $dirname/$deployfolder/dbcode
mkdir -p $dirname/$deployfolder/libs
mkdir -p $dirname/$deployfolder/settings
mkdir -p $dirname/$deployfolder/scripts
mkdir -p $dirname/$deployfolder/libs/plugins


#settingFiles=( "settings_discord" )
executableFiles=( discord )
dbcode=( discord_init dbinit pagecacheinit tasksinit )
jsscripts=( flare_post )
shellscripts=( page_fixer )

CopyFilesToFolder settingFiles Run/settings $dirname/$deployfolder/settings  ".ini"
CopyFilesToFolder executableFiles release $dirname/$deployfolder  
CopyFilesToFolder dbcode Run/dbcode $dirname/$deployfolder/dbcode  ".sql"
CopyFilesToFolder shellscripts shell $dirname/$deployfolder/scripts  ".sh"
CopyFilesToFolder jsscripts shell $dirname/$deployfolder/scripts  ".js"

#copying .so dependencies to libs folder
for filename in "${executableFiles[@]}"
do
	./shell/cpld.sh $dirname/$deployfolder/$filename $dirname/$deployfolder/libs
done

#copying qt dependencies
echo "Operation: Copying Qt Dependencies"
echo "plugins"
cp -rf $QT_BASE_DIR/plugins $dirname/$deployfolder/libs/plugins
cp -rf $QT_BASE_DIR/plugins $dirname/$deployfolder/plugins
cp -rf $QT_BASE_DIR/plugins/sqldrivers $dirname/$deployfolder

