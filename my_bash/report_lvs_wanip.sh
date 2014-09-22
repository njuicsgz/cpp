#! /bin/bash

## report $LANIP,$LANMAC,$WANIP,$WANMAC
## wylonhuang,20101021-10:22
## ----------------------------------------------------------

export PATH=/sbin:/usr/sbin:/usr/local/sbin:/usr/local/bin:/usr/bin:/bin
export WORKDIR=$( cd ` dirname $0 ` && pwd )

cd "$WORKDIR"                   || exit 1

. "$WORKDIR/func-common.sh"     || exit 1
. "$WORKDIR/lvs_fun.sh"         || exit 1
. "$WORKDIR/lvs_notify_fun.sh"  || exit 1
. "$WORKDIR/lvs_file_path.conf" || exit 1

MY_VERSION=1   ## need to be increased if report msg format changed
MY_RESULT=0    ## 0 -> SUCESS, 1 -> FAILURE
MY_PROTOCAL=9  ## 1: lvs config , 2: flux data, 3: connect data

TIMESTAMP=$( date +%Y%m%d%H%M%S )
LANIP=$( get_lvs_lanip )

## report 0 if we can not get config data
EMPTY_MSG="${LANIP:-0},0,0,0,0,0,0,0,0,0"

## keep local log, we may need this log someday to check history data, we write log in report()
LOG="$WORKDIR/log.d/log.$( basename $0 )"
TMPF="$WORKDIR/log.d/active.$( basename $0 )"

##---------------------  FUNCTION ------------------##
get_lvs_lanmac()
{
	local lan_mac
	lan_mac=$(/sbin/ifconfig|grep -B 1 "$LANIP "|awk '/HWaddr/{gsub(":",":",$5);print $5}')
	echo $lan_mac
}

get_lvs_wanip()
{
	local wanip
	NICNUM=$(cat /proc/net/dev |grep -E -i "eth[0-9]"|wc -l)
	if [[ $NICNUM == 2 ]];then
		WAN_IFACE="eth0"
	elif [[ $NICNUM == 4 ]];then
		WAN_IFACE="eth2"
	else
		WAN_IFACE="eth0"
	fi
	wanip=$(/sbin/ifconfig|grep -A 1 "$WAN_IFACE "|awk -F":" '/inet addr/{gsub(" Bcast","",$2);print $2}')
	[[ -n $wanip ]] || wanip="0.0.0.0"
	echo $wanip
}

get_lvs_wanmac()
{
	local wan_mac
	wan_mac=$(/sbin/ifconfig|grep -B 1 "$WANIP "|awk '/HWaddr/{gsub(":",":",$5);print $5}')
	echo $wan_mac
}

## --------------------  MAIN  -------------------- ##

install -d -m 750 $( dirname $LOG )

## make sure just one instance running
lock_file="/tmp/.$( basename $0 )"
if is_locked "$lock_file"; then
    echo "$TIMESTAMP already running, will exit ..." >> "$LOG"
    MY_RESULT=1
    report "lvs-$MY_VERSION,$MY_RESULT,$MY_PROTOCAL,$TIMESTAMP-${REPORT_MSG:-$EMPTY_MSG}-lvs"
    exit 1  
fi

lock_on "$lock_file"

LANIP=$( get_lvs_lanip )
LANMAC=$( get_lvs_lanmac )
WANIP=$( get_lvs_wanip )
WANMAC=$( get_lvs_wanmac )

## now collect lvs config info via lvs.conf
BODY_MSG=
EMPTY_MSG="$BODY_MSG|xxx,xxx,xxx,xxx"
total=0
BODY_MSG="$BODY_MSG|$LANIP,$LANMAC,$WANIP,$WANMAC"
#echo "BODY_MSG IS $BODY_MSG"
if ! [[ -z $BODY_MSG ]];then
   (( total++ ))
fi
if (( total == 0 )); then
   BODY_MSG="$EMPTY_MSG"
fi

BODY_MSG=$( echo "$BODY_MSG" | sed 's/^|//' ) ## trim leading "|"
#echo "$LANIP $LANMAC $WANIP $WANMAC"
report "lvs-$MY_VERSION,$MY_RESULT,$MY_PROTOCAL,$TIMESTAMP-${BODY_MSG:-$EMPTY_MSG}-lvs"
#echo "lvs-$MY_VERSION,$MY_RESULT,$TIMESTAMP-${BODY_MSG:-$EMPTY_MSG}-lvs"
