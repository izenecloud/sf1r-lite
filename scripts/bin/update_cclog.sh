#!/bin/bash

# Log Server ip address is set at "../libdriver/config.yml(.default)"
# Assume Log Server is running on localhost

work_dir=`dirname $0`

Sf1rHost=172.16.0.162
Sf1rPort=18181

case "$1" in
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
	echo "#### update history cclogs ####"
	echo
	ruby $work_dir/log_server_json_sender.rb update_cclog_history cclog_history $Sf1rHost

	echo
	echo "#### update latest cclog ####"
	echo
	for file in cclog_latest/*
	do
	  if [ -e $file ]
	  then
	    ruby $work_dir/log_server_json_sender.rb update_cclog $file $Sf1rHost
	  fi
	done
        mkdir cclog_latest_backup
        mv cclog_latest/* cclog_latest_backup/
    ;;
  backup)
	# Backup is valid when log server is runing on localhost.
        # backup history cclogs
	rm -r cclog_history_bak
	mv cclog_history cclog_history_bak
	cp -r ../../bin/log_server_storage/cclog/history ./
	mv history cclog_history

        # backup history cclogs with raw docids
	rm cclog_history/*.errors
        rm ../../bin/log_server_storage/cclog/history_raw/*.errors
	for file in cclog_history/*
	do
	  ruby $work_dir/log_server_json_sender.rb backup_raw_cclog $file
	done        
    ;;
  recover)
	# recover cclog from backup with rawids
	rm ../../bin/log_server_storage/cclog/history_raw/*.errors
	for file in ../../bin/log_server_storage/cclog/history_raw/*
	do
	  ruby $work_dir/log_server_json_sender.rb convert_raw_cclog $file
	done
    ;;
  *)
	echo "Usage: $0 {update <Sf1rHost> | backup | recover}"
    ;;
esac


