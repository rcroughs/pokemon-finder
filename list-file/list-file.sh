#!/bin/bash

if [[ -z $1 ]]
then
  # Dans le cas où il n'y a pas de paramètre
  echo "Missing directory name."
  exit 1
elif [[ ! -d $1 ]]
then
  # Dans le cas où le directory n'existe pas
  echo "The specified path is not a directory"
  exit 2
else
  # Execution classique
  find $1 -type f -maxdepth 1
  exit 0
fi
