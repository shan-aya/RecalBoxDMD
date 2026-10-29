# Routes « logos de jeux » du démon Pi (3.75, non publiée)

Pour l'onglet Personnalisation du toolkit. Toutes sur le port 80 du Pi. Cible : `/systems/<sys>/<Lettre|#>/<jeu>.raw565` (128x32 RGB565 petit-boutiste, 8192 octets).

Règles de nom (communes) : `dir` = système `[A-Za-z0-9_-]`, 48 max, sans `_`/`.` en tête, dossier déjà présent dans `/systems`. `name` = `<jeu>.raw565`, ASCII imprimable sans espace ni `\ / : * ? " < > |`, sans `.`/`_` en tête (même assainissement que le toolkit : espaces supprimés, interdits → `_`). Violation → 400 (ou `ok:false` pour l'envoi).

| Route | Rôle | Réponse |
|---|---|---|
| `POST /upload?kind=logo&dir=<sys>` (multipart, 1 fichier `<jeu>.raw565`) | Écrit le logo là où celui du jeu existe déjà (sous-dossier de lettre, sinon à plat), sinon dans `<Lettre\|#>/`. `.tmp` puis renommage. | `{"ok":N,"files":[{"name","ok":true}\|{"name","ok":false,"err"}]}` (HTTP 200 même en échec : lire `files[].ok`) |
| `GET /logo-info?dir=<sys>&name=<jeu>.raw565` | Existence et empreinte | `{"exists":false}` ou `{"exists":true,"where":"letter"\|"flat","size":8192,"fnv":"xxxxxxxx"}` (FNV-1a 32 bits du fichier) |
| `GET /logo?dir=<sys>&name=<jeu>.raw565` | Les 8192 octets (garder l'original avant remplacement) | octets, ou 404 |
| `POST /logo-delete?dir=<sys>&name=<jeu>.raw565` | Supprime ce fichier (jamais un dossier) | `{"deleted":true,"where":...}` ou 404 |
| `POST /logos-refresh` (formulaire `games=<sys>/<jeu>\|<sys>/<jeu>…`, `dry=1` facultatif) | À appeler UNE fois en fin de lot : complète `games_cache.bin` avec les jeux qui ont un logo mais pas d'entrée (sinon le Pi affiche l'image par défaut), redessine l'écran si le jeu affiché en fait partie | `{"added","known","nologo","invalid","cache":"ok"\|"unavailable","identical":bool}` |

Notes :
- `cache:"unavailable"` (fichier absent, illisible ou non reproductible à l'identique) : rien n'est écrit ; demander à l'utilisateur de lancer le Mode 6, ou prévenir que le logo ne s'affichera qu'après.
- `dry=1` : compte sans écrire (utile pour prévenir avant).
- Rétablir l'original = `GET /logo` avant le premier remplacement (à conserver côté PC), puis `kind=logo` pour le remettre ; `logo-delete` laisse le jeu sans logo (image par défaut).
- Un jeu SANS logo d'origine : `logo-delete` suffit à revenir à l'état initial (l'entrée du cache reste, sans effet : fichier absent → repli).
- Détection du Pi : `find_pi_dmd()` du toolkit (`GET /load` → `is_pi`). Démon ≥ 3.75 requis (un démon plus ancien répond 200 mais range le fichier comme un GIF : tester la version avant tout envoi, comme `REELS_MIN_PI`).
- Le serveur du Pi est multi-fils mais l'ESP32 NON : ces routes n'existent pas sur l'ESP32 (carte SD retirée, comme aujourd'hui).
