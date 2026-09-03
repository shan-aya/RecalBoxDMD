#!/bin/sh
mosquitto_pub -h 127.0.0.1 -p 1883 -t marquee/cmd/wifi_recovery -m ""
