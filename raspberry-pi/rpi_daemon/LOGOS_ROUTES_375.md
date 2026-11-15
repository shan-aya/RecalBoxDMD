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
## Ajout 3.81 (non publié) : `GET /games-snapshot`

Lecture seule. Renvoie les octets de `/custom/games.json` (JSON, ≤ 16 Mo), l'instantané du dernier scrape (Mode 1 du toolkit) : liste des jeux par système, avec le drapeau `"c":1` des jeux dont le visuel est une **carte texte** (un `.raw565` comme un autre, que rien d'autre ne distingue d'un logo). 404 `ERR: instantane absent` si le fichier n'existe pas, est vide ou dépasse 16 Mo.

Le toolkit (`pi_logos.fetch_snapshot`) le copie dans le miroir du Pi quand le Pi devient la source de l'onglet Personnalisation ; sur un démon plus ancien (404, ou page web en 200 sans JSON), il ne fait rien et garde l'instantané local. Le fichier arrive sur le Pi avec la copie du Mode 6 (`custom/games.json` de la partition médias).

**Non compilé** : aucun compilateur sur le PC de développement. À compiler et tester sur le Pi avant la release 3.81 (`PI_VERSION` n'a PAS été changé).

## Ajout 3.82 (non publié) : animations de navigation (`<jeu>.raw565pack` + `<jeu>.meta`)

Même route d'envoi et mêmes règles de nom que le logo fixe (`kind=logo`, `dir=<système>`, nom ASCII sans espace), avec deux extensions de plus :
- `<jeu>.raw565pack` : **2 à 600 images** de 8192 octets (128×32 RGB565) ; `<jeu>.meta` : **2 octets (uint16 LE) de délai en ms par image**, 1200 octets au plus. Chaque fichier passe par un `.tmp` puis un renommage (jamais de fichier tronqué). Le toolkit envoie le pack **puis** le meta.
- Emplacement : là où se trouve déjà un des fichiers du jeu (logo fixe, pack, meta), à plat ou dans le sous-dossier de sa lettre, sinon dans la lettre.
- `GET /logo-info?dir=&name=<jeu>.raw565pack` : comme pour le logo fixe, avec en plus `"frames"` (taille / 8192) ; FNV-1a 32 bits du pack entier. (`GET /logo` reste réservé au logo fixe.)
- `POST /logo-delete?dir=&name=<jeu>.raw565pack` : supprime le pack **et** son `.meta`.
- `POST /logos-refresh` : un jeu qui n'a qu'une animation compte comme ayant un visuel (il n'est plus « nologo »).
- **Le logo fixe est dessiné AVANT l'animation** (`CMD_GAME`, chemin rapide) : pour qu'une animation s'affiche, le toolkit supprime le `.raw565` du jeu après l'envoi (l'original reste dans son store, « Retirer la perso » le remet et supprime l'animation). Aucun changement du moteur de rendu : le chemin pack existait déjà (`pack.cpp`).
- Un démon < 3.82 refuse ces noms (400 / « nom de logo invalide ») : le toolkit teste `GET /load` (≥ 3.82) avant d'envoyer une animation.