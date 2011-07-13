#!/usr/bin/env bash

COLLECTION=chinese-wiki

rm -r collection/$COLLECTION/collection-data/default-collection-dir
mv collection/$COLLECTION/scd/index/backup/* collection/$COLLECTION/scd/index/
