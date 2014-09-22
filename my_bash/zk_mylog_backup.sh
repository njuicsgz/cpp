#! /bin/bash
export PATH=/sbin:/usr/sbin:/usr/local/sbin:/usr/local/bin:/usr/bin:/bin

log_dir="/data/zookeeper/mylog"
backup_dir="/data/zookeeper/mylog_backup"
dir_name="zk_mylog_`date +'%Y-%m-%d'`"

backup_log()
{
	local dir="${backup_dir}/${dir_name}"
	if [[ ! -e "$backup_dir" ]];
	then		
		echo "$backup_dir is not exist, mkdir"
		mkdir "$backup_dir"		
	else		
		#if exist, delete it
		if [[ -e "$dir" ]];
		thens
			rm -r "$dir"
		fi
		
		cp -r $log_dir $dir
	fi
}

reduce_file()
{
	local file_name=$*
	if [[ ! -f $file_name ]];then
        echo "$file_name no found"
        return
	fi
	
	local del_content="set"
	sed -i -e "/${del_content}/d" "$file_name"   #删除只用这行就可以了
		
}

#if the dir does not exist, ignore it
ruduce_dir()
{	
	if [[ -z "${backup_dir}/${dir_name}" ]];
	then		
		echo "[WARN] ${backup_dir}/${dir_name} is not exist, ignored"
	else
		echo "[INFO] files will be reduce"
		cd "${backup_dir}/${dir_name}" || return
				
		find -type f -name 'log*' | while read var; do
			reduce_file "$var"
		done
		
		echo "[INFO] done"
	fi
}

backup_log
ruduce_dir
