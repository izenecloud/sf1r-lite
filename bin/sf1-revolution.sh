#!/bin/bash
export LD_LIBRARY_PATH="/home/lscm/codebase/sf1r-engine/lib:/home/lscm/codebase/sf1r-engine/lib-thirdparty"
NAME=SF1r

CONSOLE_LOG_DIR="./consolelog"
mkdir -p $CONSOLE_LOG_DIR
TIMENOW=`date +%m-%d.%H%M`
 case "$1" in
  start)
	PROCESS_NUM=`ps -ef|grep "CobraProcess"|grep -v grep|wc -l`
	if [ $PROCESS_NUM -eq 1 ]
	  then
	    echo -e "$NAME has already started!\n"
	    exit 0
	  else
	    echo -e "Starting $NAME..."
	    ./CobraProcess -F config > $CONSOLE_LOG_DIR/log.$TIMENOW.log 2>&1 &
	    sleep 4
	    PROCESS_NUM=`ps -ef|grep "CobraProcess"|grep -v grep|wc -l`
	    if [ $PROCESS_NUM -eq 1 ]
	      then
	        echo -e "Success. \nTo monitor the log file, type: 'tail -f log'\n"
	      else
	        echo -e "Fail.\nCheck log for more detail.\n"
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
	./CobraProcess -F config > $CONSOLE_LOG_DIR/log.$TIMENOW.log 2>&1 &
	sleep 4
	PROCESS_NUM=`ps -ef|grep "CobraProcess"|grep -v grep|wc -l`
	if [ $PROCESS_NUM -eq 1 ]
	  then
	    echo -e "Success.\n"
	  else
	    echo -e "Fail.\nCheck log for more detail.\n"
	fi
	;;
  *)
	echo -e "Usage: ./sf1-revolution.sh  { start | stop | restart }\n" >&2
	exit 1
	;;
 esac
exit 0
