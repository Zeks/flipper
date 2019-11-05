#!/bin/bash
entryResult=true

command="stat -c %Y $1"
cmdResult=$($command)
echo "result of time compute: " $cmdResult
entryResult=true
#timeDiff=$(( `date +%s` - cmdResult))
#if [ $timeDiff -gt 600 ]
#then
	#echo 'File you are deploying is too old.'
#	entryResult=false
#fi
