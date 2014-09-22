#! /bin/bash
export PATH=/sbin:/usr/sbin:/usr/local/sbin:/usr/local/bin:/usr/bin:/bin

log_file="/usr/local/zookeeper/log_clean_tools/zk_log_clean.log"
conf_file="/usr/local/zookeeper/conf/zoo.cfg"

log_prepare()
{
	touch "$log_file"
	local SIZE=`ls -s $log_file | awk '{print$1}'`
	if [ $SIZE -ge 10240 ];
	then
		echo "[`date '+%Y-%m-%d %H:%M:%S'`] begin" > $log_file
	fi
}

log()
{
	local msg=$*        
	echo "[`date '+%Y-%m-%d %H:%M:%S'`] "$msg"" >> $log_file
}

#if the dir does not exist, ignore it
clean_dir()
{
	local dir_name=$1
	local num=$2
	dir=$( grep -E "^[[:blank:]]*(${dir_name})=" ${conf_file} | awk -F "=" '{print $2}' )
	if [ -z "$dir" ];
	then		
		log "[WARN] $dir is not exist, ignored"
	else
		log "[INFO] $dir/version-2 will be clean"
		cd "$dir/version-2" || return
		
		ls -rt  | tail -n $num > /tmp/100
		ls                     > /tmp/all
		grep -wvf /tmp/100 /tmp/all  | xargs -r -n 100 rm
		log "[INFO] $dir/version-2 clean doen, remain $remain_num latest files"
	fi
}

remain_num=100
log_prepare
clean_dir "dataDir" $remain_num
clean_dir "dataLogDir" $remain_num

#import crontab, never done in this file, just for studying
#bak_file="/tmp/crontab.bak"
#crontab -l > $bak_file
#echo "#clean zookeeper server log/snapshot everyday. created by allengao@tencent.com" >> $bak_file
#echo "23 2 * * * /usr/local/zookeeper/log_clean_tools/zklog_clean.sh > /dev/null 2>&1" >> $bak_file
#crontab $bak_file
