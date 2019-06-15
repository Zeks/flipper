#!/bin/bash


#making folders
deployfolder="flipper_deploy"
rm -rf $deployfolder 
rm -rf fpmtmp
mkdir $deployfolder
mkdir $deployfolder/libs

workdir=$PWD
cd $workdir
echo 'Deploying executables'

chmod +x shell/deploy_client.sh 
./shell/deploy_client.sh 
if [ $? -ne "0" ]
then 
	exit 1
fi

dirname=$workdir
unpackpath=$dirname/fpmtmp
unpackdir="fclient"
scriptfolder="shell/"
rm -rf $unpackpath
now=$(date)

mkdir -p $unpackpath/$unpackdir


deployfolder=$PWD/"deployment"


curr_path=`dirname $0`
libsFolder=$deployfolder/libs

cp -rf $deployfolder/* $unpackpath/$unpackdir

echo 'Copying dependencies'
gitversion=$(git rev-parse --short=number HEAD)
branch=$(git rev-parse --abbrev-ref HEAD)
separator="-"
ident=$gitversion$separator$branch
#echo "ident: "$ident
if [ ! -z "$removefile" ]; then
    rm -f $removefile
fi
mkdir $dirname/fpmtmp
echo 'creating rpm'
echo $unpackpath
packageName="feeder"
#fpm -s dir -t deb -n $packageName -v $ident --workdir ~/m_client --verbose --debug --deb-compression gz -C $unpackpath 
#mv deployment flipper_deploy
tar -czvf flipper.tar.gz flipper_deploy
mv -f flipper.tar.gz ~/releases



