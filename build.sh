#!/bin/sh

export PROJBASE=$(cd `dirname "${BASH_SOURCE[0]}"` && pwd)
echo "Current directory: $PROJBASE"

COMMAND=$1

if [[ $COMMAND = "clean" ]]
then
    make -C cm clean
    make -C log clean
    make -C mq clean
    make -C ipc clean
    make -C net clean
    make -C cpp clean
    make -C dao clean
    make -C test/ipc clean
    make -C test/net clean
    make -C test/log clean
    make -C test/str clean
else
    make -C cm
    make -C log
    make -C mq
    make -C ipc
    make -C net
    make -C cpp
    make -C dao
    make -C test/ipc
    make -C test/net
    make -C test/log
    make -C test/str
fi
