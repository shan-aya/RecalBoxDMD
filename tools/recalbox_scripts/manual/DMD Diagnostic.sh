#!/bin/sh
# 2026-10-31 -- lance le mode DIAGNOSTIC du DMD depuis la Recalbox (meme ecran que la page web /config/diag
# et que le bouton BOOT du DMD) : signal Wi-Fi, qualite, canal, liaison avec CETTE Recalbox (pertes, latence,
# gigue), reconnexions Wi-Fi. Meme mecanisme que les autres scripts manuels : "CMD=..." envoye en UDP brut
# vers le port 5005 du DMD (IP en dur, a adapter si l'IP du DMD change).
# Relancer ce script ARRETE le mode diagnostic (bascule) ; il s'arrete aussi tout seul au bout de 5 minutes.
# Les mesures de latence reposent sur la reponse "PONG" du pont marquee.sh : il doit tourner sur la Recalbox.
DMD_UDP_IP="192.168.0.51"
DMD_UDP_PORT=5005
send_udp() {
    python3 -c "import socket,sys; socket.socket(socket.AF_INET, socket.SOCK_DGRAM).sendto(sys.argv[1].encode('utf-8','replace'), (sys.argv[2], int(sys.argv[3])))" "$1" "$DMD_UDP_IP" "$DMD_UDP_PORT" 2>/dev/null
}
send_udp "CMD=diag"
echo "Mode diagnostic demande au DMD ($DMD_UDP_IP). Relancez ce script pour l'arreter (arret automatique apres 5 minutes)."
