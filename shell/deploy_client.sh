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
mkdir -p $dirname/$deployfolder/libs
mkdir -p $dirname/$deployfolder/libs/plugins


settingFiles=( "settings" )
executableFiles=( flipper )
scripts=( dbinit pagecacheinit tasksinit user_db_init )

CopyFilesToFolder settingFiles Run $dirname/$deployfolder  ".ini"
CopyFilesToFolder executableFiles release $dirname/$deployfolder  
CopyFilesToFolder scripts Run/dbcode $dirname/$deployfolder/dbcode  ".sql"

	cp -f $QT_BASE_DIR/plugins/platforms/libqxcb.so $deployfolder/libs

#copying .so dependencies to libs folder
for filename in "${executableFiles[@]}"
do
	./shell/cpld.sh $dirname/$deployfolder/$filename $dirname/$deployfolder/libs
done
	./shell/cpld.sh $dirname/$deployfolder/libs/libqxcb.so $dirname/$deployfolder/libs

#copying qt dependencies
echo "Operation: Copying Qt Dependencies"
echo "plugins"
cp -rf $QT_BASE_DIR/plugins $dirname/$deployfolder/libs/plugins
cp -rf $QT_BASE_DIR/plugins $dirname/$deployfolder/plugins


	echo "copying necessary .so files"
	cp -f $QT_BASE_DIR/plugins/platforms/libqxcb.so $deployfolder/libs
	cp -f $QT_BASE_DIR/plugins/platformthemes/libqgtk2.so $deployfolder/libs
    cp -rf /home/zekses/Qt5.12.1/5.12.1/gcc_64/qml/QtCharts/*.so $deployfolder/libs/qml/
    cp -f /home/zekses/Qt5.12.1/5.12.1/gcc_64/lib/libQt5Charts.so $deployfolder/libs
    cp -f /home/zekses/Qt5.12.1/5.12.1/gcc_64/lib/libQt5QuickTemplates2.so.5 $deployfolder/libs
    cp -f /home/zekses/Qt5.12.1/5.12.1/gcc_64/lib/libQt5QuickControls2.so.5 $deployfolder/libs


    
    cp -rf $QT_BASE_DIR/plugins/platforms $deployfolder
    cp -rf $QT_BASE_DIR/plugins $deployfolder/libs    
    cp -rf $QT_BASE_DIR/qml $deployfolder/libs
