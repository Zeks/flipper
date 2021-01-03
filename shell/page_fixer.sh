#!/bin/bash
sed -i 's/\\n/\n/g' $1
sed -i 's/\\"/"/g' $1

