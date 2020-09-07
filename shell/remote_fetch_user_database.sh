#!/bin/bash
remote_user=$1
remote_server=$2
fetch_target=$3

echo "will use params: " $remote_user $remote_server
echo "creating fetch folder: " $fetch_target
mkdir -p $fetch_target
echo "stopping remote bot"
ssh $remote_user@$remote_server 'bash --login -s' < ./remote_stop_bot.sh
echo "fetching the database"
scp $remote_user@$remote_server:/discord/database/DiscordDB.sqlite $fetch_target/DiscordDB.sqlite$4
echo "starting remote bot"
ssh $remote_user@$remote_server 'bash --login -s' < ./remote_start_bot.sh

