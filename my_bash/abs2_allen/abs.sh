#!/bin/sh
####################################
# 2006.06 by echoqin
# Copyright (C) 2006 by tencent
#
# 2006.06 by echoqin
# 	added: key parameter check
#	added: configuration check
#	updated: force parameter check
#
####################################

##########################################    var  defination    ##########################################

export PATH=/sbin:/usr/sbin:/usr/local/sbin:/usr/local/bin:/usr/bin:/bin

## we need the exit status of ssh.exp and scp.exp
EXIT_STATUS=1

##########################################    proc defination    ##########################################
# ���Թ����趨
ignore_init()
{
	arry_ignore_pwd_length=0
	if [ -f ./ignore_pwd ]; then
		while read IGNORE_PWD
		do
			arry_ignore_pwd[$arry_ignore_pwd_length]=$IGNORE_PWD	
			let arry_ignore_pwd_length=$arry_ignore_pwd_length+1
		done < ./ignore_pwd
	fi
	
	arry_ignore_ip_length=0
	if [ -f ./ignore_ip ]; then
		while read IGNORE_IP
		do
			arry_ignore_ip[$arry_ignore_ip_length]=$IGNORE_IP
			let arry_ignore_ip_length=$arry_ignore_ip_length+1
		done < ./ignore_ip
	fi
}

show_version()
{
	echo "abs version: 2.0"
	echo "published date: 2006.06.01"
	echo "author: echoqin"
}

show_usage()
{
	echo "USAGE:$0  [-h|--help] "
	echo "		[-u|--user] "
	echo "		[-v|-V|--version] "
	echo "		[-l|--iplist ... ] "
	echo "		[-c|--config ... ] "
	echo "		[-t|--sshtimeout ... ] "
	echo "		[-T|--fttimeout ... ] "
	echo "		[-L|--bwlimit ... ] "
	echo "		[-n|--ignore] " 
	#echo "ignr_flag: 'ignr'-some ip will be ignored; otherwise-all ip will be handled"
}


# ����Ĭ��ֵ
USER=root	# һ�������ʹ��root�û���½����˼��ڴ��趨���Ժ���Ҫ�ɽ��������ļ�����
myIFS=":::"	# �����ļ��еķָ���
TOOLDIR=`pwd`

IPLIST="abs-iplist"		# IP�б���ʽΪIP ���� �˿�
CONFIG_FILE="abs-config"	# �����б���ļ����������б��ؼ���Ϊcom:::��file:::
IGNRFLAG="noignr"		# �����Ϊignr����ű�����к����������ж�
SSHTIMEOUT=50			# Զ������ִ����ز����ĳ�ʱ�趨����λΪ��
SCPTIMEOUT=100			# �ļ�������ز����ĳ�ʱ�趨����λΪ��
BWLIMIT=0			# �ļ����͵Ĵ������٣���λΪkbit/s


# ��ڲ�������
TEMP=`getopt -o hvVl:c:t:T:L:u:n --long user:,help,version,iplist:,config:,sshtimeout:,fttimeout:,bwlimit:,ignore -- "$@" 2>/dev/null`

if [ $? != 0 ] ; then
	echo "ABS_ERROR: wrong option!"
	echo ""
	show_usage
	exit 1
fi

# �Ὣ����getopt��������Ĳ�������ǰ�棬�������ں��棬������������--
eval set -- "$TEMP"

while true ; do

	if [ -z "$1" ]; then
		break;
	fi

	case "$1" in
		-h|--help) show_usage; exit 1;;
		-u|--user) USER=$2; shift 2;;
		-v|-V|--version) show_version; exit 1;;
		-l|--iplist) IPLIST=$2; shift 2;;
		-c|--config) CONFIG_FILE=$2; shift 2;;
		-t|--sshtimeout) SSHTIMEOUT=$2; shift 2;;
		-T|--fttimeout) SCPTIMEOUT=$2; shift 2;;
		-L|--bwlimit) BWLIMIT=$2; shift 2;;
		-n|--ignore) IGNRFLAG="ignr"; shift;;
		--) shift;; 
		*) echo "ABS_ERROR: wrong option!"; echo ""; show_usage; exit 1 ;;
	esac
done



##########################################    main    ##########################################

BEGINDATETIME=`date "+%D %T"`


if [ ! -f $IPLIST ]; then
	echo "ABS_ERROR: iplist \"$IPLIST\" not exists, please check!"
	echo ""
	exit 1
fi

if [ ! -f $CONFIG_FILE ]; then
	echo "ABS_ERROR: config \"$CONFIG_FILE\" not exists, please check!"
	echo ""
	exit 1
fi

# cat $IPLIST | grep -v '^$' > $IPLIST.1
# mv $IPLIST.1 $IPLIST

perl -lni -e ' print unless /^\s*$/ ' $IPLIST


# ǿ���ֹ�ȷ��
echo ""
echo "you are using:"
echo "	\"$CONFIG_FILE\"    ---- as your config"
echo "	\"$IPLIST\"    ---- as your iplist"
echo "	\"$SSHTIMEOUT\"    ---- as your ssh timeout"
echo "	\"$SCPTIMEOUT\"    ---- as your scp timeout"
echo "	\"$BWLIMIT\"    ---- as your bwlimit"
echo "	\"$IGNRFLAG\"    ---- as your ignore flag"
echo

while true
do
	echo -n "please make sure these all... [y/n]  "
	read PARM_CHECK
	if [ -z "$PARM_CHECK" ]; then
		continue
	fi
	if [ "$PARM_CHECK" != "y" ]; then
		exit 1
	else
		break;
	fi
done

echo ""
cat $CONFIG_FILE

echo ""
while true
do
	echo -n "the content of your \"$CONFIG_FILE\" as above, please make sure... [y/n]  "
	read CONFIG_CHECK
	if [ -z "$CONFIG_CHECK" ]; then
		continue
	fi
	if [ "$CONFIG_CHECK" != "y" ]; then
		exit 1
	else
		break;
	fi
done

echo ""

IPSEQ=0
while read IP PASSWD PORT OTHERS
do
	let IPSEQ=$IPSEQ+1

	# ��������˺��ԣ�������������
	if [ ${IGNRFLAG} == "ignr" ]; then

		ignore_init;
		ignored_flag=0

		ii=0
		while [ $ii -lt $arry_ignore_pwd_length ]
		do
			if [ ${PASSWD}x == ${arry_ignore_pwd[$ii]}x ]; then
				ignored_flag=1
				break
			fi

			let ii=$ii+1
		done

		if [ $ignored_flag -eq 1 ]; then
			continue
		fi

		jj=0
		while [ $jj -lt $arry_ignore_ip_length ]
		do
			if [ ${IP}x == ${arry_ignore_ip[$jj]}x ]; then
				ignored_flag=1
				break
			fi

			let jj=$jj+1
		done

		if [ $ignored_flag -eq 1 ]; then
			continue
		fi

	fi

	# ���һ��$IP��ִ�������ļ��е�һ���ײ���
	while read eachline
	do
		# ���л�ȫ���ɿո��Ʊ������ɵ��б�����
		if [ -z "`echo $eachline | grep -v ^$`" ]; then
			continue
		fi

		# ���׵�һ���ǿ��ַ�Ϊ#ʱ�����б��϶�Ϊע�ͣ��������κδ���
		commentflag=${eachline:0:1}
		if [ "${commentflag}"x == "#"x ]; then
			continue
		fi

		myKEYWORD=`echo $eachline | awk -F"$myIFS" ' { print $1; } '`
		myCONFIGLINE=`echo $eachline | awk -F"$myIFS" ' { print $2; } '`

		# �������ļ��е�Ԥ����Ŀ���չ�����ַ���������չ
		# �ؼ���#IP#����$IP�����滻
		if [ ! -z "`echo "$myCONFIGLINE" | grep '#IP#'`" ]; then
			myCONFIGLINE_temp=`echo $myCONFIGLINE | sed "s/#IP#/$IP/g"`
			myCONFIGLINE=$myCONFIGLINE_temp
		fi

		# ʱ����عؼ��֣��õ�ǰʱ������滻
		if [ ! -z "`echo "$myCONFIGLINE" | grep '#YYYY#'`" ]; then
			myYEAR=`date +%Y`
			myCONFIGLINE_temp=`echo $myCONFIGLINE | sed "s/#YYYY#/$myYEAR/g"`
			myCONFIGLINE=$myCONFIGLINE_temp
		fi

		if [ ! -z "`echo "$myCONFIGLINE" | grep '#MM#'`" ]; then
			myMONTH=`date +%m`
			myCONFIGLINE_temp=`echo $myCONFIGLINE | sed "s/#MM#/$myMONTH/g"`
			myCONFIGLINE=$myCONFIGLINE_temp
		fi

		if [ ! -z "`echo "$myCONFIGLINE" | grep '#DD#'`" ]; then
			myDATE=`date +%d`
			myCONFIGLINE_temp=`echo $myCONFIGLINE | sed "s/#DD#/$myDATE/g"`
			myCONFIGLINE=$myCONFIGLINE_temp
		fi

		if [ ! -z "`echo "$myCONFIGLINE" | grep '#hh#'`" ]; then
			myHOUR=`date +%H`
			myCONFIGLINE_temp=`echo $myCONFIGLINE | sed "s/#hh#/$myHOUR/g"`
			myCONFIGLINE=$myCONFIGLINE_temp
		fi

		if [ ! -z "`echo "$myCONFIGLINE" | grep '#mm#'`" ]; then
			myMINUTE=`date +%M`
			myCONFIGLINE_temp=`echo $myCONFIGLINE | sed "s/#mm#/$myMINUTE/g"`
			myCONFIGLINE=$myCONFIGLINE_temp
		fi

		if [ ! -z "`echo "$myCONFIGLINE" | grep '#ss#'`" ]; then
			mySECOND=`date +%S`
			myCONFIGLINE_temp=`echo $myCONFIGLINE | sed "s/#ss#/$mySECOND/g"`
			myCONFIGLINE=$myCONFIGLINE_temp
		fi

		# IPSEQ�ؼ��֣��õ�ǰIP�����к��滻����1��ʼ
		if [ ! -z "`echo "$myCONFIGLINE" | grep '#IPSEQ#'`" ]; then
			myCONFIGLINE_temp=`echo $myCONFIGLINE | sed "s/#IPSEQ#/$IPSEQ/g"`
			myCONFIGLINE=$myCONFIGLINE_temp
		fi


		# �����ļ����йؼ���com:::���͵���ssh.exp����Զ������ִ��
		if [ "$myKEYWORD"x == "com"x ]; then

			$TOOLDIR/ssh.exp $IP $USER $PASSWD $PORT "${myCONFIGLINE}" $SSHTIMEOUT
            EXIT_STATUS=$?
			if [ $EXIT_STATUS -ne 0 ]; then
				break;
			fi

		# �����ļ����йؼ���file:::���͵���scp.exp�����ļ�����
		elif [ "$myKEYWORD"x == "file"x ]; then
			SOURCEFILE=`echo $myCONFIGLINE | awk ' { print $1; } ' `
			DESTDIR=`echo $myCONFIGLINE | awk ' { print $2; } ' `
			DIRECTION=`echo $myCONFIGLINE | awk ' { print $3; } ' `
			# SOURCEFILE=$( echo $myCONFIGLINE | perl -lane ' print "@F[1..-3]" ' )
			# DESTDIR=$( echo $myCONFIGLINE    | perl -lane ' print "$F[-2]' )
			# DIRECTION=$(  echo $myCONFIGLINE | perl -lane ' print "$F[-1]' )
		
			$TOOLDIR/scp.exp $IP $USER $PASSWD $PORT $SOURCEFILE $DESTDIR $DIRECTION $BWLIMIT $SCPTIMEOUT
            EXIT_STATUS=$?
			if [ $EXIT_STATUS -ne 0 ]; then
				break;
			fi

		else
			echo "ABS_ERROR: configuration wrong! [$eachline] "
			echo "           where KEYWORD should not be [$myKEYWORD], but 'com' or 'file'"
			echo "           if you dont want to run it, you can comment it with '#'"
			echo ""
			exit 1
		fi

	done < $CONFIG_FILE

done < ${IPLIST}

ENDDATETIME=`date "+%D %T"`

echo "$BEGINDATETIME -- $ENDDATETIME"
echo "$0 $* --excutes over!"

exit $EXIT_STATUS

