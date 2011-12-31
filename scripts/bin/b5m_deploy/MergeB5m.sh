#!/bin/bash

home=`dirname $0`
scdsFinalDir=/opt/se/b5m/scds/4final/
scdsIndexDir=/opt/se/b5m/scds/6indexSCD/

#调用ruby脚本进行合并SCD
rm -f ${scdsIndexDir}*
exec "$home/b5m_scd_merger.sh" ${scdsFinalDir} ${scdsIndexDir}
