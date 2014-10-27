#! /bin/bash
export PATH=/sbin:/usr/sbin:/usr/local/sbin:/usr/local/bin:/usr/bin:/bin
export WORKDIR=$( cd ` dirname $0 ` && pwd )

log_file="${WORKDIR}/prs-generate-availability-zone-topology.log"

TOPO_HEAD="<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<TopologyConfiguration xmlns=\"http://www.platform.com/ego/2005/05/schema\" \
xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" \
xsi:schemaLocation=\"http://www.platform.com/ego/2005/05/schema\">
<TopologyList>
    <Version>1.0</Version>
    <Topology>
        <Name>AvailabilityZone</Name>
        <LevelList>
            <Level Name=\"host\" Type=\"0\"/>
            <Level Name=\"zone\" Type=\"1\"/>
       </LevelList>"

TOPO_TAIL="    </Topology>
</TopologyList>
</TopologyConfiguration>"

EGO_TOPOLOGY_CONF_FILE="DynamicResourceTopology.xml"
EGO_TOPOLOGY_CONF_FILE_BAK="DynamicResourceTopology.xml.bak"

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

source_env()
{
        ego_top=$EGO_TOP
        if [[ ! -d "$ego_top" ]];then
                ego_top="/opt/ibm/ego"
        fi

        if [[ ! -f $ego_top/profile.platform ]];then
                log "file ($ego_top/profile.platform) does not exit, exit."
                log "please source ego profile before run this script."
                exit 1
        else
                log "use ego profile ($ego_top/profile.platform)"
        fi
        source $ego_top/profile.platform
}


is_vemkd_not_running()
{
    killall -0 "vemkd" &> /dev/null
}

check_vemkd_running()
{
        if ! is_vemkd_not_running; then
                log "vemkd is not running, exit."
                exit 1
        fi
}

get_zones()
{
        echo $( rpistat|grep availability_zone|awk -F '<' '{print $2}'|awk -F '>' '{print $1}' | uniq )
}

gen_xml()
{
        source_env

        if [ $EGO_CONFDIR == "" ];then
                log "EGO_CONFDIR is not in system environment, please source ego profile, exit now."
                exit 1
        else
                log "use ego configure dir ($EGO_CONFDIR)"
        fi

        cd $EGO_CONFDIR || exit 1
        if [ -f $EGO_TOPOLOGY_CONF_FILE ];then
                mv $EGO_TOPOLOGY_CONF_FILE $EGO_TOPOLOGY_CONF_FILE_BAK
        fi

        touch $EGO_TOPOLOGY_CONF_FILE
        echo "$TOPO_HEAD" > $EGO_TOPOLOGY_CONF_FILE

        local zone
        for zone in $( get_zones );do
                echo "       <Node Name=\"${zone}\">" >> $EGO_TOPOLOGY_CONF_FILE
                echo "           <ResourceRequirement>select(availability_zone='${zone}')</ResourceRequirement>" >> $EGO_TOPOLOGY_CONF_FILE
                echo "       </Node>" >> $EGO_TOPOLOGY_CONF_FILE
        done

        echo "$TOPO_TAIL" >> $EGO_TOPOLOGY_CONF_FILE

        cd $WORKDIR
}

log_prepare
check_vemkd_running
gen_xml
