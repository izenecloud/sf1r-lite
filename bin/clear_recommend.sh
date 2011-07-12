#!/usr/bin/env bash

COLLECTION=m18

rm -r collection/$COLLECTION/collection-data/default-recommend-dir
mv collection/$COLLECTION/scd/recommend/item/backup/* collection/$COLLECTION/scd/recommend/item/
mv collection/$COLLECTION/scd/recommend/user/backup/* collection/$COLLECTION/scd/recommend/user/
mv collection/$COLLECTION/scd/recommend/order/backup/* collection/$COLLECTION/scd/recommend/order/
