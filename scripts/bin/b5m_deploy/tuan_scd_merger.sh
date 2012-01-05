#!/bin/bash
home=`dirname $0`
exec "$home/../../ScdMerger" $1 DOCID,SourceUrl,City,Url,Title,Discount,MerchantAddr,MerchantArea,MerchantTel,Sales,TimeBegin,TimeEnd,PriceOrign,MerchantName,Price,Source,Category,SubCategory,Picture,uuid $2
