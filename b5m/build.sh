#!/usr/bin/env bash

#rebuild all including product-matching, scd-generation and send log server requests

home=`dirname $0`
if [ "$#" -gt "0" -a "$1" == "--reindex" ]
then
    echo 'reindex, remove work dir'
    rm -rf "$home/working"
fi
ruby "$home/b5m_matcher.rb"
