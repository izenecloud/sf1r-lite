#!/bin/bash
home=`dirname $0`
exec "$home/../../ScdMerger" $1 DOCID,Url,Title,Picture,Price,Source,Category,Attribute,Content,uuid $2
