#!/usr/bin/env bash

cpu_num=`grep -c "^processor" /proc/cpuinfo`

cd ..
CODEBASE_DIR="$(pwd)"

dependencie=(izenelib icma ijma ilplib imllib idmlib sf1r-engine)

element_count=${#dependencie[@]}
index=0
while [ "$index" -lt "$element_count" ]
do 
  echo "compile ${dependencie[$index]} ..."
  if [ -d  $CODEBASE_DIR/${dependencie[$index]}  ];then
     cd "$CODEBASE_DIR/${dependencie[$index]}/build"
     git pull
     make -j$cpu_num
     if [ ! $? -eq 0 ];then
       echo "error in compiling ${dependencie[$index]}!"
       exit 1
     fi
  else
    echo "ERROR:${dependencie[$index]} doesn't exists."
    exit 1
  fi 
  let "index = $index + 1"
done

