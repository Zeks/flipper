#!/bin/bash
variableName="RECS_BOT_NAME"
botname=${!variableName}
echo "using bot name " $botname
containerName=$(docker ps -q --filter ancestor=$botname)
echo "using container name " $containerName
docker stop $containerName
docker rm $containerName
