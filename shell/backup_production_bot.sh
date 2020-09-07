#!/bin/bash
./remote_fetch_user_database.sh $PRODUCTION_USER $PRODUCTION_IP $BOT_DATABASE_ARCHIVE .$(date +%s)
