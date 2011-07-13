#!/usr/bin/env bash

COLLECTION=intel_demo

rm -r collection/$COLLECTION/collection-data/default-collection-dir
mv collection/$COLLECTION/scd/index/backup/* collection/$COLLECTION/scd/index/
