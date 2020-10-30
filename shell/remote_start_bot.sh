#!/bin/bash
bot_name=$RECS_BOT_NAME
docker run -d --mount type=bind,source=/discord/database,target=/root/database --mount type=bind,source=/discord/settings,target=/root/settings $bot_name
