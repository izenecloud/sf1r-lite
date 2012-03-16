#!/bin/bash

# Log Server ip address is set at "../libdriver/config.yml(.default)"
# Assume Log Server is running on localhost

log_server_dir=$HOME/codebase/sf1r-engine/bin/log_server_storage
work_dir=`dirname $0`

Sf1rHost=localhost # required as commandline parameter
Sf1rPort=18181


case $1 in
  update)
	if [ -n "$2" ]
	then
	  Sf1rHost=$2
    else
	  echo "Please specify Sf1rHost to update."
      echo "Usage: $0 {update <Sf1rHost> | backup}"
      exit
	fi

	echo
	echo "#### updating latest cclog ####"
	echo

    rm cclog/*.errors
    mkdir cclog/archive
	for file in cclog/*
	do
	  if [ -f $file ]
	  then
        echo "$file"
        ruby $work_dir/log_server_client.rb update_cclog $file $Sf1rHost        
        mv $file cclog/archive/
	  fi
    done

    echo
	echo "#### updating history cclogs ####"
	echo
	rm cclog/history/*.errors
    mkdir cclog/history
    ruby $work_dir/log_server_client.rb update_cclog_history cclog/history $Sf1rHost

    # udpate history
    cp -r $log_server_dir/cclog/history cclog/
    ;;

  backup)
    echo     
    echo "### backup cclogs "
    rm cclog/history/*.errors
    for file in cclog/history/*
    do
      ruby $work_dir/log_server_client.rb backup_raw_cclog $file
    done
    rm cclog/history/*.errors    
    cp -r $log_server_dir/cclog/history_raw cclog/
    ;;
  recover)
	# recover cclog from backup with rawids
	rm $log_server_dir/cclog/history_raw/*.errors
	for file in $log_server_dir/cclog/history_raw/*
	do
	  ruby $work_dir/log_server_client.rb convert_raw_cclog $file
	done
    ;;
  *)
	echo "Usage: $0 {update <Sf1rHost> | backup | recover }"
    ;;
esac


