#! /bin/bash
export PATH=/sbin:/usr/sbin:/usr/local/sbin:/usr/local/bin:/usr/bin:/bin

export WORKDIR=$( cd ` dirname $0 ` && pwd )
cd "$WORKDIR" || exit 1

CONF_DIR_LOCAL=local
CONF_DIR_AWS=aws

# clean and prepare output dirs
rm -rf $CONF_DIR_LOCAL $CONF_DIR_AWS
mkdir -p $CONF_DIR_LOCAL $CONF_DIR_AWS
cp *.json *.yml $CONF_DIR_LOCAL
cp *.json *.yml $CONF_DIR_AWS

# read conf file
replace()
{
    local file=$1
    local dir=$2
    cat $file | while read line
    do
        key=${line%%=*}
        value=${line#*=}

        if [[ "$key"x != ""x && $key != \#* ]]; then
            value=$(echo $value | sed 's/\//\\\//g') 
            sed -i "s/$key/$value/g" $dir/* 
        fi 
    done
}

replace env/local.properties $CONF_DIR_LOCAL
replace env/aws_us_east.properties $CONF_DIR_AWS
