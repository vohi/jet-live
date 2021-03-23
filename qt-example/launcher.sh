#!/bin/bash

$1 &> debug.log &
pid=$!

trap ctrl_c INT

function ctrl_c() {
    kill -SIGINT $pid
    sleep 1
    [ $(ps $pid) ] && kill $pid || return
    sleep 1
    [ $(ps $pid) ] && kill -9 $pid || return
    echo "Hard kill"
}

while ps $pid &> /dev/null
do
    printf "\r(r)eload, (R)estart, or (q)uit: "
    unset cmd
    read -rsn1 -t 1 cmd
    case $cmd in
    q)
        printf "Quitting\n"
        kill -SIGINT $pid && sleep 1
        exit 0
        ;;
    r)
        printf "Reloading\n"
        kill -SIGUSR1 $pid
        ;;
    R)
        printf "Restarting\n"
        kill -SIGUSR2 $pid
        ;;
    esac
done
