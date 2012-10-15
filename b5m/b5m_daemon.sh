#!/usr/bin/env bash
NAME='b5m_matcher'
home=`dirname $0`
LOG_FILE=$home/daemon.log
 case "$1" in
  start)
    PROCESS_NUM=`ps -ef|awk '{print $8}'|grep $NAME|wc -l`
    if [ $PROCESS_NUM -eq 1 ]
      then
        echo -e "$NAME has already started!\n"
        exit 0
      else
        echo -e "Starting $NAME..."
        $home/b5m_matcher >> $LOG_FILE 2>&1 &
        sleep 2
        PROCESS_NUM=`ps -ef| awk '{print $8}'|grep "b5m_matcher"|wc -l`
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
    ps -ef|awk '{print $2,$8}'|grep $NAME|awk '{print $1}'|xargs kill -9 2>/dev/null
    sleep 1
    echo -e "Done.\n"
    ;;
  restart)
    echo -e "Restarting $NAME..."
    ps -ef|awk '{print $2,$8}'|grep $NAME|awk '{print $1}'|xargs kill -9 2>/dev/null
    sleep 1
    $home/b5m_matcher >> $LOG_FILE 2>&1 &
    sleep 4
    PROCESS_NUM=`ps -ef|awk '{print $8}'|grep $NAME|wc -l`
    if [ $PROCESS_NUM -eq 1 ]
      then
        echo -e "Success. \nTo monitor the log file, type: 'tail -f $LOG_FILE'\n"
      else
        echo -e "Fail.\nCheck log $LOG_FILE for more detail.\n"
    fi
    ;;
  *)
    echo -e "Usage: b5m_daemon.sh  { start | stop | restart }\n" >&2
    exit 1
    ;;
 esac
exit 0
