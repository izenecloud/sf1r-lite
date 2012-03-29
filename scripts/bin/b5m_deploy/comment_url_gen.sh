#!/bin/bash

home=`dirname $0`
ruby "$home/comment_url_gen.rb" /opt/se/b5m/scdsComm/sourceSCD3C/singleSCD/B*.SCD
#mv newegg.SCD /opt/se/b5m/scdsComm/sourceSCD/newegg.com.cn
#mv 360buy.SCD /opt/se/b5m/scdsComm/sourceSCD/360buy.com

