#!/usr/bin/env bash

#rebuild all including product-matching, scd-generation and send log server requests

home=`dirname $0`
ruby "$home/start_b5m_matcher.rb" $@
