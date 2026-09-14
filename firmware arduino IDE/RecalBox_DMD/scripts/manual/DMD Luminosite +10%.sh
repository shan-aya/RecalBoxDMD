#!/bin/sh
# 2026-09-14 -- bascule MQTT -> UDP (retour utilisateur : "les scripts
# utilisateur sur RB1 ne fonctionnent plus, ils etaient en mqtt"). Le
# firmware n'ecoute plus MQTT du tout depuis la bascule full UDP
# (mqttClient.connect() n'est plus jamais appele, voir DECISIONS.md/
# RecalBox_DMD.ino) -- mosquitto_pub publiait donc dans le vide. Meme
# mecanisme UDP que marquee.sh (send_udp()) : "CMD=..." envoye en UDP brut
# vers le port 5005 du DMD. IP en dur (meme convention que marquee.sh/
# dmd_score.sh/dmd_achievement.sh -- pas de decouverte automatique a ce
# jour, a adapter si l'IP du DMD change).
DMD_UDP_IP="192.168.0.51"
DMD_UDP_PORT=5005
send_udp() {
    python3 -c "import socket,sys; socket.socket(socket.AF_INET, socket.SOCK_DGRAM).sendto(sys.argv[1].encode('utf-8','replace'), (sys.argv[2], int(sys.argv[3])))" "$1" "$DMD_UDP_IP" "$DMD_UDP_PORT" 2>/dev/null
}
# 2026-09-02 -- topic marquee/cmd/brightness_up -> marquee/cmd unique
# (voir RecalBox_DMD.ino v148) : 12 topics fusionnes en 1 seul cote DMD --
# reste vrai cote commande UDP (meme cle "brightness_up").
send_udp "CMD=brightness_up"
echo "Commande +10% luminosite envoyee au DMD."
