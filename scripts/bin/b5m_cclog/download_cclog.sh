#!/bin/sh

if [ $# -eq 0 ];then
  echo "usage: $0 [www|bak]"
  exit 1
fi

if [ $1 = "www" ];then
  IP=10.10.1.110
elif [ $1 = "bak" ];then
  IP=10.10.1.109
else
  echo "usage: $0 [www|bak]"
  exit 1
fi

echo "downloading cclog from $1 $IP"

mkdir -p cclog
scp lscm@$IP:/usr/local/nginx/logs/error.*.log ./cclog/
