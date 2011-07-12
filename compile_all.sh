#!/usr/bin/env bash

cd ..
CODEBASE_DIR="$(pwd)"

dependencie=(izenelib icma ilplib imllib idmlib sf1r-engine)

element_count=${#dependencie[@]}
index=0
while [ "$index" -lt "$element_count" ]
do 
  echo "compile ${dependencie[$index]} ..."
  if [ -d  $CODEBASE_DIR/${dependencie[$index]}  ];then
     cd "$CODEBASE_DIR/${dependencie[$index]}/build"
     git pull
     make -j8
  else
    echo "ERROR:${dependencie[$index]} doesn't exists."
    exit 1
  fi 
  let "index = $index + 1"
done

