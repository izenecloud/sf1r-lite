#!/usr/bin/env bash

if [ $# != 1 ];then
  echo "Usage: $0 collection"
  exit 1
fi

COLLECTION=$1

rm -r collection/$COLLECTION/collection-data/default-recommend-dir
mv collection/$COLLECTION/scd/recommend/user/backup/* collection/$COLLECTION/scd/recommend/user/
mv collection/$COLLECTION/scd/recommend/order/backup/* collection/$COLLECTION/scd/recommend/order/
