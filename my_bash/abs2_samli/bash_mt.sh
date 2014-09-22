## bash multi-process lib
## Author: samli@tencent.com, 2010-12-25
## Version: 0.1

## make a fifo pipe, init the max process count
mt_init()
{
    local mt_cnt=$1
    local freefd=10   ## the fd of fifo

    export FIFO_PIPE="${WORKDIR:-/tmp}/.fifo.$$"
    export FIFO_FD


    ## find a free fd as the fifo fd
    while (( freefd <= 60000 )); do
        if [[ ! -L /dev/fd/$freefd ]]; then
            FIFO_FD=$freefd
            break
        else
            (( freefd++ ))
        fi
    done

    [[ -n $FIFO_FD   ]] || exit 1
    mkfifo "$FIFO_PIPE" || exit 1
    eval "exec $FIFO_FD<> \"$FIFO_PIPE\""

    /bin/rm "$FIFO_PIPE"

    mt_up $mt_cnt
}

mt_up()
{
    local cnt=$1

    [[ -n $cnt ]] || cnt=1

    while (( cnt > 0 )); do
        eval echo >&"$FIFO_FD"
        (( cnt-- ))
    done 
}

mt_down()
{
    local cnt=$1

    [[ -n $cnt ]] || cnt=1

    while (( cnt > 0 )); do
        read -u "$FIFO_FD"
        (( cnt-- ))
    done
}

## run the job in backgroup, exec mt_up after job done
mt_run()
{
    { mt_down; eval "$@"; mt_up; } &
}

