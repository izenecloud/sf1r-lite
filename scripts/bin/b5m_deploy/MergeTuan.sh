#!/bin/bash

home=`dirname $0`

scdsFinalDir=/opt/se/b5m/scdsTuan/3final/
scdsMergeDir=/opt/se/b5m/scdsTuan/4mergeSCD/
scdsIndexDir=/opt/se/b5m/scdsTuan/6indexSCD/
scdsSF1Dir=/opt/sf1/apps/sf1r/bin/collection/tuanm/scd/index/

#调用ruby脚本进行合并SCD
rm -f ${scdsMergeDir}*
rm -f ${scdsIndexDir}*
bash "$home/tuan_scd_merger.sh" ${scdsFinalDir} ${scdsMergeDir}
#调用ruby脚本处理分类
ruby "$home/tuan_processor.rb" ${scdsMergeDir}B*.SCD ${scdsIndexDir}
