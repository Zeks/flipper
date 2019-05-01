#!/bin/bash
function evil_git_dirty {
	 expr `git diff --shortstat 2> /dev/null | wc -l`
}

function evil_git_num_untracked_files {
  expr `git status --porcelain 2>/dev/null| grep "^??" | wc -l` 
}

if [ "$2" != "ignore"  ] && [ $(evil_git_num_untracked_files) -ne "0" ]
then
	echo "untracked uncommitted files"	
	exit 1
fi

if [ "$2" != "ignore" ] && [ $(evil_git_dirty) -ne "0" ]
then
	echo "git index is dirty"	
	exit 1 
fi   
 
echo "success, git index is OK, going further"

timeDiff=$(( `date +%s` - `stat -c %Y release/feed_server`))

if [ $timeDiff -gt 600 ]
then
	echo 'File you are deploying is too old.'
	exit 1
fi




#making folders
deployfolder="deployment"
rm -rf $deployfolder 
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
fpm -s dir -t deb -n $packageName -v $ident --workdir ~/m_client --verbose --debug --deb-compression gz -C $unpackpath 


