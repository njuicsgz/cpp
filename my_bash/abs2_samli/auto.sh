#! /bin/bash

## ----------------------------------------------------------

. "$( dirname $0 )/func-common.sh" || exit 1

cd "$WORKDIR" || exit 1

logd="$WORKDIR/log.d"
log="$logd/log.$( date '+%F_%H%M%S' )"
notok="$WORKDIR/notok"

[[ -d $logd ]] || mkdir -p $logd

bash ./abs.sh -t 60 -T 60 -L 999999 $@ 2>&1 | tee -a $log

dos2unix "$log" &> /dev/null

msg "----------------------------------------------------------------"
msg "LOGFILE: $log"
msg "NOTOK:   $notok"
msg "----------------------------------------------------------------"

