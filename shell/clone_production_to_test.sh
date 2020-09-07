#!/bin/bash
echo "fetching remote database"
./remote_fetch_user_database.sh $PRODUCTION_USER $PRODUCTION_IP $BOT_TRANSIENT_FOLDER

ssh $TEST_USER@$TEST_IP 'bash --login -s' < ./remote_stop_bot.sh
ssh $TEST_USER@$TEST_IP cp /discord/database/DiscordDB.sqlite /discord/database/DiscordDB.sqlite.$(date +%s)
scp $BOT_TRANSIENT_FOLDER/DiscordDB.sqlite $TEST_USER@$TEST_IP:/discord/database/DiscordDB.sqlite
ssh $TEST_USER@$TEST_IP 'bash --login -s' < ./remote_start_bot.sh
