#/usr/bin/bash 

doRsync()
{
	echo "start $1..."
	./ssh.exp  $1 root $2 36000 "rm -rf /data/zookeeper/testdata/*;"
}

doRsync 10.144.57.13 Z00k11per
doRsync 10.144.57.14 Z00k11per
doRsync 10.144.56.148 Z00k11per
doRsync 10.144.56.114 Z00k11per
doRsync 10.144.56.115 Z00k11per