#! /bin/bash

#CCNL_BASE="/home/pi/ccn/build/bin"
CCNL_BASE=${CCNL_BASE:-"/home/pi/ccn/build/bin"}
MAC_NODE="d8:80:39:02:c0:f9"
MAC_WIRELESS=""
PREFIX="/p"
PREFIX_WIRELESS="/w"

if [ "$EUID" -ne 0 ]
    then echo "Please run as root, you noob"
    exit
fi

# Kill running relays
pkill ccn-lite-relay

$CCNL_BASE/ccn-lite-relay -e eth0 -w wpan0 > /dev/null 2>&1 &

sleep 2

FACEID=`$CCNL_BASE/ccn-lite-ctrl newETHface any $MAC_NODE 2049 | $CCNL_BASE/ccn-lite-ccnb2xml | grep FACEID | sed -e 's/^[^0-9]*\([0-9]\+\).*/\1/'`

$CCNL_BASE/ccn-lite-ctrl prefixreg $PREFIX $FACEID

FACEID_WIRELESS=`$CCNL_BASE/ccn-lite-ctrl newWPANface ff:ff:ff:ff:ff:ff:ff:ff 0x23 | $CCNL_BASE/ccn-lite-ccnb2xml | grep FACEID | sed -e 's/^[^0-9]*\([0-9]\+\).*/\1/'`

$CCNL_BASE/ccn-lite-ctrl prefixreg $PREFIX_WIRELESS $FACEID_WIRELESS