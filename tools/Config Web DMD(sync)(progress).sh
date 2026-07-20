#!/bin/sh
mosquitto_pub -h 127.0.0.1 -p 1883 -t marquee/cmd/show_config -m ""
IP=$(mosquitto_sub -h 127.0.0.1 -p 1883 -t marquee/status/ip -C 1)
echo "Ouvrez un navigateur sur http://$IP"
