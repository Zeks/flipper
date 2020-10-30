#!/bin/bash
botname=$1
remote_user=""
remote_server=""
branch="discord_new"

if [ -z $2 ]; then 
    remote_user=$PRODUCTION_USER
else 
    remote_user=$2
fi

if [ -z $3 ]; then 
    remote_server=$PRODUCTION_IP
else 
    remote_server=$3
fi


if [ ! -z $4 ]; then 
    branch=$4
fi

echo "will use params: " $remote_user $remote_server
echo "building bot"
DOCKER_BUILDKIT=1 docker build --build-arg CACHEBUSTER=$(date +%s) --build-arg BRANCH=$branch -f $FLIPPER_REPO_FOLDER/dockerfiles/builder.ficrec_bot --target bot_runner -t $botname .
echo "saving bot"
docker save -o $BOT_SAVE_FOLDER/$botname.tar.gz $botname
echo "copying bot files to remote server"
scp $BOT_SAVE_FOLDER/$botname.tar.gz  $remote_user@$remote_server:/discord/$botname.tar.gz
echo "stopping remote bot"
ssh $remote_user@$remote_server 'bash --login -s' <  remote_stop_bot.sh
echo "copying template file"
cp $FLIPPER_REPO_FOLDER/shell/push_bot.template $FLIPPER_REPO_FOLDER/shell/push_bot.sh
echo "updating bot name"
chmod +x  $FLIPPER_REPO_FOLDER/shell/push_bot.sh
sed -i 's/BOTNAME/'$botname'/g' $FLIPPER_REPO_FOLDER/shell/push_bot.sh
sed -i 's/PRODUCTIONUSER/'$remote_user'/g' $FLIPPER_REPO_FOLDER/shell/push_bot.sh
echo "executing over ssh"
ssh $remote_user@$remote_server 'bash --login -s' < ./push_bot.sh
rm $FLIPPER_REPO_FOLDER/shell/push_bot.sh


