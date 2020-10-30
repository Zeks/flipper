#!/bin/bash

function env_or_default()
{
    local env_var=$1
    local entity_name=$2
    local default_value=$3
    if [ -z "${!env_var}" ]
    then
          echo $env_var" is empty. using default value of: "$default_value
    else
          echo $env_var" is NOT empty. setting "$entity_name" to: "${!env_var}
          eval $entity_name=${!env_var}
    fi
}
