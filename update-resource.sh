#!/usr/bin/env bash
username='dev'
password=''

usage() {
  echo "usage: update-resource.sh <branchname> [--all|--kpe|--nec|--speller-support|--ise|--sim]"
  echo "      <branchname> will include 'owl-1', 'owl-2', etc.."
  echo "      no module specific means all(--all)"
}

if [ $# -lt 1 ] || [ $# -gt 2 ]
then 
  usage
  exit 0
fi

if [ $# -eq 1 ] && [ "$1" = '--help' ]
then
  usage
  exit 0
fi


home=`dirname "$0"`
home=`cd "$home"; pwd`
if [ -d "$home/package/resource" ]
then
    a=1
else
    mkdir -p "$home/package/resource"
fi

if [ $# -eq 2 ]
then
  dir_name=''
  if [ "$2" = '--kpe' ]
  then
    dir_name='kpe'
  elif [ "$2" = '--nec' ]
  then
    dir_name='nec'
  elif [ "$2" = '--speller-support' ]
  then
    dir_name='speller-support'
  elif [ "$2" = '--ise' ]
  then
    dir_name='ise'
  elif [ "$2" = '--sim' ]
  then
    dir_name='sim'
  elif [ "$2" = '--all' ]
  then
    dir_name='all'
  fi
  
  if [ "$dir_name" != "" ] && [ "$dir_name" != "all" ]
  then
#    echo "rsyncing $dir_name only, type the password: $password"
    rsync -azvP --delete "$username@izenesoft.cn:/data/sf1r-resource/$1/resource/$dir_name/" "$home/package/resource/$dir_name/"
  elif [ "$dir_name" = "all" ]
  then
#    echo "rsyncing all, type the password: $password"
    rsync -azvP --delete "$username@izenesoft.cn:/data/sf1r-resource/$1/resource/" "$home/package/resource/"
  else
    echo "unknow module: $2"
    usage
  fi
else
#  echo "rsyncing all, type the password: $password"
  rsync -azvP --delete "$username@izenesoft.cn:/data/sf1r-resource/$1/resource/" "$home/package/resource/"
fi

