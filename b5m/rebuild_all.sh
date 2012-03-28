#!/usr/bin/env bash

#rebuild all including product-matching, scd-generation and send log server requests

home=`dirname $0`
rm -rf "$home/working"
ruby "$home/b5m_matcher.rb"
