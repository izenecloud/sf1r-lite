#!/usr/bin/env bash
# Usage: ./compile_all [JOB_NUM]
# the default value of JOB_NUM is 2, it specifies the number of jobs to run simultaneously

JOB_NUM=2
[ $# -gt 0 ] && JOB_NUM=$1

cd ..
CODEBASE_DIR="$(pwd)"

export IZENELIB=$CODEBASE_DIR/izenelib
export ILPLIB=$CODEBASE_DIR/ilplib
export IMLLIB=$CODEBASE_DIR/imllib
export IDMLIB=$CODEBASE_DIR/idmlib
export WISEKMA=$CODEBASE_DIR/wisekma
export IZENECMA=$CODEBASE_DIR/icma
export IZENEJMA=$CODEBASE_DIR/ijma

if [ ! -d $WISEKMA ];then
    export WISEKMA=$CODEBASE_DIR/kma
fi

PROJECT_CMAKE_PATH=$CODEBASE_DIR/cmake
if [ -d $PROJECT_CMAKE_PATH ];then
  export EXTRA_CMAKE_MODULES_DIRS=$PROJECT_CMAKE_PATH

  cd $PROJECT_CMAKE_PATH
  git pull
else
  echo "ERROR: $PROJECT_CMAKE_PATH doesn't exist."
  exit 1
fi

dependencie=(izenelib icma ijma ilplib imllib idmlib sf1r-engine)

element_count=${#dependencie[@]}
index=0
while [ "$index" -lt "$element_count" ]
do
  echo "build ${dependencie[$index]} ..."

  if [ ! -d $CODEBASE_DIR/${dependencie[$index]} ];then
    echo "ERROR: ${dependencie[$index]} doesn't exists."
    exit 1
  fi

  BUILD_PATH=$CODEBASE_DIR/${dependencie[$index]}/build
  if [ ! -d $BUILD_PATH ];then
    mkdir $BUILD_PATH
  fi
  cd $BUILD_PATH

  git pull

  if [ $? -ne 0 ];then
      exit $?
  fi

  if [ -f CMakeCache.txt ];then
    rm CMakeCache.txt
  else
    if [ -f $CODEBASE_DIR/${dependencie[$index]}/CMakeLists.txt ];then
      cmake -DEXTRA_CMAKE_MODULES_DIRS=$PROJECT_CMAKE_PATH ..
    elif [ -f $CODEBASE_DIR/${dependencie[$index]}/source/CMakeLists.txt ];then
      cmake -DEXTRA_CMAKE_MODULES_DIRS=$PROJECT_CMAKE_PATH ../source
    else
      echo "ERROR: no CMakeLists.txt found in ${dependencie[$index]}!"
      exit 1
    fi

    if [ ! $? -eq 0 ];then
      echo "ERROR: cmake ${dependencie[$index]}!"
      exit 1
    fi
  fi

  make -j$JOB_NUM
  if [ ! $? -eq 0 ];then
    echo "ERROR: compiling ${dependencie[$index]}!"
    exit 1
  fi

  let "index = $index + 1"
done
