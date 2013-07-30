#!/usr/bin/env bash
filepath=$(cd "$(dirname "$0")"; pwd)
mkdir result
cd ../../testbin
./parseScd SPU.SCD $1
./xinhaoDectect $1
cp $1.train ~/classfication/sgd/data/conll2000/
if [ $#  -ne 1 ];then
   if [ $2 = "-test" ];then
   mv $3 $3.brand
   ./xinhaoDectect $3
   mv $3.train $1.test
   cp $1.test ~/classfication/sgd/data/conll2000/
   mv $3.brand $3
   fi
fi
cd ~/classfication/sgd/crf/
if [ ! -f $1.model.gz ];then
./run.sh $1
fi
if [ $#  -ne 1 ];then
    if [ $2 = "-test" ];then
      ./crfsgd -t $1.model.gz ../data/conll2000/$1.test
      cp ../data/conll2000/$1.rightType  $filepath/result/$1.rightType
      cp ../data/conll2000/$1.wrongType  $filepath/result/$1.wrongType
    fi
fi
