#!/bin/bash
COUNT=10
if /usr/zhan1997/bin/egrep -q [0-9] testdoc.txt ; then
    let COUNT=COUNT+5
fi
echo $COUNT