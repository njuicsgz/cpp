#! /bin/bash

## ----------------------------------------------------------

. "$( dirname $0 )/func-common.sh" || exit 1

cd "$WORKDIR" || exit 1

## ----------------------------------------------------------

IP=$( get_localip )

RC=




## ----------------------------------------------------------











## ----------------------------------------------------------

RC=$?

if (( RC == 0 )); then
    echo "$IP OPOK OPOK"
else
    rmsg "$IP NOTOPOK NOTOPOK"
fi

