#!/bin/sh
mosquitto_pub -h 127.0.0.1 -p 1883 -t marquee/cmd/brightness_down -m ""
echo "Commande -10% luminosite envoyee au DMD."
