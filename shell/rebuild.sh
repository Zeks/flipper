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

