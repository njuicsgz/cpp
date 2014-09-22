#!/bin/sh

export PATH=$PATH:/sbin:/usr/sbin:/usr/local/sbin:/usr/local/bin:/usr/bin:/bin

process="zkmonitor_bin"

cd ` dirname $0 `

count=`ps -ef |grep "${process}" |grep -v grep|wc -l`

if [ $count -lt 1 ]; then

	echo "[`date +'%Y-%m-%d %T'`] process number:$count, restart it!"
	killall -9 ${process};
	## cd /usr/local/zkmonitor;
	./${process}

else

	echo "[`date +'%Y-%m-%d %T'`] process number is normal:$count"

fi
