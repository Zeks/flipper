#!/bin/bash
remote_user=""
remote_server=""
fetch_target=$1

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
echo "will use params: " $remote_user $remote_server
echo "creating fetch folder: " $fetch_target
mkdir -p $fetch_target
echo "stopping remote bot"
ssh $remote_user@$remote_server 'bash --login -s' < ./remote_stop_bot.sh
echo "fetching the database"
scp $remote_user@$remote_server:/discord/database/DiscordDB.sqlite $fetch_target/DiscordDB.sqlite$2
echo "starting remote bot"
ssh $remote_user@$remote_server 'bash --login -s' < ./remote_start_bot.sh
