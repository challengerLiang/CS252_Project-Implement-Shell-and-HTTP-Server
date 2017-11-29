#!/bin/bash

#DO NOT REMOVE THE FOLLOWING TWO LINES
git add $0 >> .local.git.out
git commit -a -m "Lab 2 commit" >> .local.git.out

#Your code here
#echo indssseqw1>$1
read PASSWORD<$1
#echo $PASSWORD
NUM=$(wc -m < $1)
let NUM=NUM-1
#echo $NUM

#+1 point for each character in the string
POINTS=$NUM
#echo POINTS = $POINTS
if [ $NUM -lt 6 ] ||  [ $NUM -gt 32 ]  ; then
	  echo "Error: Password length invalid."
else
    #check the length of the password
    if [ $(grep [#$\+%@] $1) ] ;  then
       let POINTS=POINTS+5
	   #echo "POINTS is $POINTS      sepcial cha"
    fi      

    #check whether the password contains numbers
    if [ $(grep [0-9] $1) ] ; then
        let POINTS=POINTS+5
	    #echo "POINTS is $POINTS      num"
    fi

    #check whether the password contains letters
    if [ $(grep [A-Z] $1) ] || [ $(grep [a-z] $1) ];  then
        let POINTS=POINTS+5
	    #echo "POINTS is $POINTS      letter"
    fi

    #minus points item
    #check whether the password contains the repetition of the same alphanumeric character 
    if egrep -q '([A-Z])\1+' $1 || egrep -q '([a-z])\1+' $1 || egrep -q '([0-9])\1+' $1 ;  then
        let POINTS=POINTS-10
	    #echo "POINTS is $POINTS      repeated letter"
    fi
    
    #check whether the password contains 3 consecutive lowercase characters
    if [ $(grep [a-z][a-z][a-z] $1) ];   then
        let POINTS=POINTS-3
	    #echo "POINTS is $POINTS      consecutive lower_case letter"
    fi

    #check whether the password contains 3 consecutive uppercase characters
    if [ $(grep [A-Z][A-Z][A-Z] $1) ];   then
       let POINTS=POINTS-3
	   #echo "POINTS is $POINTS      consecutive upper_case letter"
    fi

    #check whether the password contains 3 consecutive numbers
    if [ $(grep [0-9][0-9][0-9] $1) ];   then
        let POINTS=POINTS-3
	    #echo "POINTS is $POINTS      consecutive number letter"
    fi

    echo "Password Score: $POINTS"
fi