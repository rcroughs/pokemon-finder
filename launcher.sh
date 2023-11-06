#!/bin/bash


export PATH="$PATH:$PWD/img-dist/"
target_directory="${3:-./img/}" # valeur par défaut, mais ne marche pas


if [[ $1 = "-i" || $1 = "--interactive" ]]
then
  # Mode interactif (il faut encore rajouter le préfixe)
  ./img-search $2
elif [[ $1 = "-a" || $1 = "--automatic" ]]
then
  # Mode automatique
  ./list-file.sh $3 | ./img-search $2
fi
