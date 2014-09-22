#! /bin/bash
export PATH=/sbin:/usr/sbin:/usr/local/sbin:/usr/local/bin:/usr/bin:/bin

crontab -l > /usr/local/crontab_bak/crontab_bak`date '+%Y%m%d'`.txt
