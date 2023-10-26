#!/bin/bash

if [[ $1 = "-i" || $1 = "--interactive" ]]
then
  # Mode interactif
  ./img-search $2
elif [[ $1 = "-a" || $1 = "--automatic" ]]
then
  # Mode automatique
  ./list-file.sh $3 | ./img-search $2
fi
