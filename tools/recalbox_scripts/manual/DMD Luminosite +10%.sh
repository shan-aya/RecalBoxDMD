#!/bin/sh
# 2026-09-02 -- topic marquee/cmd/brightness_up -> marquee/cmd unique
# (voir RecalBox_DMD.ino v148) : 12 topics fusionnes en 1 seul cote DMD.
mosquitto_pub -h 127.0.0.1 -p 1883 -t marquee/cmd -m "CMD=brightness_up"
echo "Commande +10% luminosite envoyee au DMD."
