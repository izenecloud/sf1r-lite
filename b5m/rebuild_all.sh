#!/usr/bin/env bash

#rebuild all including product-matching, scd-generation and send log server requests

home=`dirname $0`
ruby "$home/b5m_matcher.rb" -F -FT -FA -NOGEN
ruby "$home/scd_gen_all.rb"
ruby "$home/log_server_sender.rb" -F
