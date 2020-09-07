#!/bin/bash
remote_user=$PRODUCTION_USER
remote_server=$PRODUCTION_IP

echo "param 1" $1
echo "param 2" $2
fetch_target=$1

echo "will use params: " $remote_user $remote_server
echo "creating fetch folder: " $fetch_target
mkdir -p $fetch_target
echo "stopping remote bot"
ssh $remote_user@$remote_server 'bash --login -s' < ./remote_stop_bot.sh
echo "fetching the database"
scp $remote_user@$remote_server:/discord/database/DiscordDB.sqlite $fetch_target/DiscordDB.sqlite$2
echo "starting remote bot"
ssh $remote_user@$remote_server 'bash --login -s' < ./remote_start_bot.sh

