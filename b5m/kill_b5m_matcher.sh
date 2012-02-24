#!/usr/bin/env bash
ps -ef|grep "b5m_matcher"|grep -v grep|awk '{print $2}'|xargs kill -9
