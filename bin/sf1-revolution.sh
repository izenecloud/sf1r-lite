#!/bin/bash
NAME=SF1r

CONSOLE_LOG_DIR="./consolelog"
mkdir -p $CONSOLE_LOG_DIR
TIMENOW=`date +%m-%d.%H%M`
LOG_FILE=$CONSOLE_LOG_DIR/log.$TIMENOW.log
 case "$1" in
  start)
	PROCESS_NUM=`ps -ef|grep "CobraProcess"|grep -v grep|wc -l`
	if [ $PROCESS_NUM -eq 1 ]
	  then
	    echo -e "$NAME has already started!\n"
	    exit 0
	  else
	    echo -e "Starting $NAME..."
	    ./CobraProcess -F config > $LOG_FILE 2>&1 &
	    sleep 4
	    PROCESS_NUM=`ps -ef|grep "CobraProcess"|grep -v grep|wc -l`
	    if [ $PROCESS_NUM -eq 1 ]
	      then
	        echo -e "Success. \nTo monitor the log file, type: 'tail -f $LOG_FILE'\n"
	      else
	        echo -e "Fail.\nCheck log $LOG_FILE for more detail.\n"
	    fi
	fi
	;;
  stop)
	echo -e "Stopping $NAME..."
	ps -ef|grep "CobraProcess"|grep -v grep|awk '{print $2}'|xargs kill -9 2>/dev/null
	sleep 1
	echo -e "Done.\n"
	;;
  restart)
	echo -e "Restarting $NAME..."
	ps -ef|grep "CobraProcess"|grep -v grep|awk '{print $2}'|xargs kill -9 2>/dev/null
	sleep 1
	./CobraProcess -F config > $LOG_FILE 2>&1 &
	sleep 4
	PROCESS_NUM=`ps -ef|grep "CobraProcess"|grep -v grep|wc -l`
	if [ $PROCESS_NUM -eq 1 ]
	  then
	    echo -e "Success. \nTo monitor the log file, type: 'tail -f $LOG_FILE'\n"
	  else
	    echo -e "Fail.\nCheck log $LOG_FILE for more detail.\n"
	fi
	;;
  *)
	echo -e "Usage: ./sf1-revolution.sh  { start | stop | restart }\n" >&2
	exit 1
	;;
 esac
exit 0
