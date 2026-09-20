# Démarrage rapide — RecalBoxDMD RawEdition

[🇬🇧 English](QUICK-START.md) · 🇫🇷 **Français** · [🇪🇸 Español](QUICK-START.es.md)

> Cette page reprend seule la section « Démarrage rapide » du **[README.md](README.fr.md)** complet — pratique à partager ou imprimer. Pour les fonctionnalités, le matériel, la configuration et tout le reste, voir le README complet.

<p align="center"><b>🚀 De zéro à un marquee fonctionnel en 4 étapes 🚀</b></p>

<table align="center">
<tr>
<td align="center" width="70"><h2>1️⃣</h2></td>
<td>

**[Installez la boîte à outils PC](#installer-la-boîte-à-outils-pc) + premier lancement**
Scrapez vos jeux dans Recalbox, pointez l'outil vers votre dossier ROMs, cliquez sur **Démarrer**.

</td>
</tr>
<tr>
<td align="center"><h2>2️⃣</h2></td>
<td>

**[Assemblez le DMD](README.fr.md#matériel)**
Assemblez les deux panneaux, montez la carte DMDos, câblez — **~5 minutes, sans soudure**.

</td>
</tr>
<tr>
<td align="center"><h2>3️⃣</h2></td>
<td>

**[Flashez le firmware](README.fr.md#firmware--compiler-et-flasher)**
Installateur web en un clic — **pas besoin d'Arduino IDE**.

</td>
</tr>
<tr>
<td align="center"><h2>4️⃣</h2></td>
<td>

**Insérez la carte SD, allumez**
Le premier démarrage vous guide pour le Wi-Fi, puis la **[page de configuration web](README.fr.md#configuration-web--en-direct-dans-le-navigateur)** prend le relais pour tout le reste (luminosité, playlists, thèmes horloge...).

</td>
</tr>
</table>

## Installer la boîte à outils PC

Se télécharge sous 4 formes — prenez celle que vous préférez sur la **[page Releases](https://github.com/shan-aya/RecalBoxDMD/releases)** (les fichiers `.exe`/`.msi` compilés ne sont pas dans le dépôt lui-même, seulement publiés là-bas) :

**Option A — Installateur Windows (recommandé)**

```
1. Téléchargez RecalBoxDMD_Toolkit_Setup.exe depuis la page Releases
2. Lancez-le — raccourci menu Démarrer, icône bureau optionnelle, vrai désinstalleur
3. Lancez « RecalBoxDMD Toolkit » depuis le menu Démarrer
```

**Option B — Exécutable portable (sans installation)**

```
1. Téléchargez RecalBoxDMD_GUI.exe depuis la page Releases
2. Lancez-le directement — aucune installation, aucun Python requis, fichier unique
```

**Option C — .msi (pour un déploiement scripté/GPO)**

```
1. Téléchargez le .msi depuis la page Releases
2. msiexec /i "RecalBoxDMD Toolkit-1.0.0-win64.msi"   (ou double-clic)
```

**Option D — Depuis les sources Python**

```
1. Récupérez le dossier tools/
2. Double-cliquez sur install_and_run.bat — installe Python (via winget,
   si absent), Pillow et Markdown, puis lance la GUI
   (ou manuellement : pip install Pillow Markdown && python run_gui.py)
```

## Premier lancement

```
1. Scrapez vos jeux dans Recalbox (voir « Comment scraper ? » dans l'outil,
   selon votre version Recalbox — logo, marquee ou logo détouré)
2. Lancez la boîte à outils → onglet Main
3. Choisissez votre version Recalbox (10.x / 9.x / legacy)
4. Choisissez votre dossier ROMs (ex : D:\Recalbox\share\roms)
5. Cliquez Démarrer — le MODE 1 enchaîne tout le pipeline automatiquement
6. Insérez la carte SD → le bouton clignotant propose de la copier pour vous
```

Ensuite : [assemblez le matériel](README.fr.md#matériel) et [flashez le firmware](README.fr.md#firmware--compiler-et-flasher) — puis insérez cette carte SD et allumez.

---

📖 Documentation complète : **[README.md](README.fr.md)** · Mise à jour depuis une version antérieure : **[UPGRADING.md](UPGRADING.fr.md)** · Historique des versions : **[CHANGELOG.md](CHANGELOG.fr.md)**
