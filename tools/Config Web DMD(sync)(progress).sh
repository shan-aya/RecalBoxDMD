#!/bin/sh
mosquitto_pub -h 127.0.0.1 -p 1883 -t marquee/cmd/show_config -m ""
IP=$(mosquitto_sub -h 127.0.0.1 -p 1883 -t marquee/status/ip -C 1 -W 5)
if [ -z "$IP" ]; then
  echo "DMD injoignable via MQTT (timeout 5s) -- verifiez qu'il est allume et connecte au WiFi."
else
  echo "Ouvrez un navigateur sur http://$IP"
fi
