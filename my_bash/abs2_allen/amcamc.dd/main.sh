#! /bin/bash

## ----------------------------------------------------------

export PATH=/sbin:/usr/sbin:/usr/local/sbin:/usr/local/bin:/usr/bin:/bin

export WORKDIR=$( cd ` dirname $0 ` && pwd )

if [[ ! -r "$WORKDIR/func-common.sh" ]]; then
    echo "[$WORKDIR/func-common.sh] NOT FOUND"
    exit 1
fi

. "$WORKDIR/func-common.sh" || exit 1

cd "$WORKDIR" || exit 1

## ----------------------------------------------------------

IP=$( get_localip )

RC=

## ----------------------------------------------------------

#allengao--begin

#stop zookeeper service
killall -9 java

#replace .class file
cp build.tar.gz /usr/local/zookeeper/
cd /usr/local/zookeeper/ || die "cd error"
rm -rf build
tar zxvf build.tar.gz

#start zookeeper service
cd /usr/local/zookeeper/bin && ./zkServer.sh start

#allengao--end

## ----------------------------------------------------------

RC=$?

if (( RC == 0 )); then
    logmsg "RS $IP OPOK OPOK" >&2
else
    rmsg "RS $IP NOTOPOK NOTOPOK" >&2
fi

