#!/bin/sh

GW=./gw

# Edit this script to set up any gateways you like.

while test \! -e /tmp/chaos_stream; do
    sleep 1
done

"$GW" 95 SUPDUP 3150
