#!/bin/bash

if [[ -z $1 ]]
then
  echo "Missing directory name."
  exit 1
elif [[ ! -d $1 ]]
then
  echo "The specified path is not a directory"
  exit 2
else
  find $1 -type f -maxdepth 1
  exit 0
fi
