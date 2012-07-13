#!/usr/bin/env bash

if [ $# != 1 ];then
  echo "Usage: $0 collection"
  exit 1
fi

COLLECTION=$1

rm -r collection/$COLLECTION/collection-data/default-collection-dir
mv collection/$COLLECTION/scd/index/backup/* collection/$COLLECTION/scd/index/
