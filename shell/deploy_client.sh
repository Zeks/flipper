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

source $dirname/shell/deploy_entry_check.sh $dirname/release/flipper

if [ $entryResult == false ]
then
	echo 'Entry check failed, exiting'
	exit 1
fi


rm -rf $dirname/$deployfolder 
mkdir -p $dirname/$deployfolder/dbcode
mkdir -p $dirname/$deployfolder/settings
mkdir -p $dirname/$deployfolder/libs
mkdir -p $dirname/$deployfolder/libs/qml
mkdir -p $dirname/$deployfolder/libs/plugins


settingFiles=( "settings" "ui" )
executableFiles=( flipper )
runFiles=( flipper )
scripts=( dbinit pagecacheinit tasksinit user_db_init )

CopyFilesToFolder settingFiles Run/defaults $dirname/$deployfolder/settings  ".ini"
CopyFilesToFolder runFiles shell $dirname/$deployfolder  ".run"
CopyFilesToFolder executableFiles release $dirname/$deployfolder  
CopyFilesToFolder scripts Run/dbcode $dirname/$deployfolder/dbcode  ".sql"

	cp -f $QT_BASE_DIR/plugins/platforms/libqxcb.so $deployfolder/libs

export LD_LIBRARY_PATH=/usr/local/lib:$QT_BASE_DIR/lib
#copying .so dependencies to libs folder
for filename in "${executableFiles[@]}"
do
	./shell/cpld.sh $dirname/$deployfolder/$filename $dirname/$deployfolder/libs
done
	./shell/cpld.sh $dirname/$deployfolder/libs/libgrpc++.so.1 $dirname/$deployfolder/libs
	./shell/cpld.sh $QT_BASE_DIR/lib/libQt5Charts.so $dirname/$deployfolder/libs
echo 'iterating'
for filename in $dirname/$deployfolder/libs/*; do
	./shell/cpld.sh $filename $dirname/$deployfolder/libs
done
for filename in $QT_BASE_DIR/qml/QtQuick.2/*; do
	./shell/cpld.sh $filename $dirname/$deployfolder/libs
done


#copying qt dependencies
echo "Operation: Copying Qt Dependencies"
echo "plugins"
cp -rf $QT_BASE_DIR/plugins $dirname/$deployfolder/libs/plugins
cp -rf $QT_BASE_DIR/plugins $dirname/$deployfolder/plugins


	echo "copying necessary .so files"
	cp -f $QT_BASE_DIR/plugins/platforms/libqxcb.so $deployfolder/libs
    mkdir -p $deployfolder/sqldrivers

	cp -rf $QT_BASE_DIR/plugins/sqldrivers/libqsqlite.so $deployfolder/sqldrivers
	cp -f $QT_BASE_DIR/plugins/platformthemes/libqgtk2.so $deployfolder/libs
	cp -f /usr/local/lib/libprotobuf.so.3.13* $deployfolder/libs
	cp -f /usr/local/lib/libssl.so $deployfolder/libs
	cp -f /usr/local/lib/libcrypto.so $deployfolder/libs
	cp -f /usr/local/lib/libre2.so $deployfolder/libs
	cp -f /usr/local/lib/libgrpc.so.13* $deployfolder/libs

    cp -rf $QT_BASE_DIR/qml/QtCharts/*.so $deployfolder/libs/qml/
    cp -f $QT_BASE_DIR/lib/libQt5Charts.so $deployfolder/libs/libQt5Charts.so.5
    cp -f $QT_BASE_DIR/lib/libQt5QuickTemplates2.so.5 $deployfolder/libs
    cp -f $QT_BASE_DIR/lib/libQt5QuickControls2.so.5 $deployfolder/libs


    
    cp -rf $QT_BASE_DIR/plugins/platforms $deployfolder
    cp -rf $QT_BASE_DIR/plugins $deployfolder/libs    
    cp -rf $QT_BASE_DIR/qml $deployfolder/libs

find $dirname/$deployfolder -name "*.so.debug" -type f -delete
