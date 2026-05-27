# RecalBoxDMD
interface en MQTT entre recalbox et un DMD 128*32 esp32



RecalBoxLED firmware - résumé perf (hot-path rendu)
-----------------------------------------------------
Changements majeurs (focus: drawPng/drawRaw565 + SD probes):
1) drawRaw565() optimisé
   - Avant: lecture SD “ligne par ligne” (32 reads) -> jitter + latence.
   - Maintenant: lecture SD en 1 seul gros bloc (RAW565_W*RAW565_H*2 bytes) + blit ligne par ligne depuis RAM.
   - Mesures sur système lent:
     * raw565 présent (sans fallback): ~1.93–1.94 s
     * raw565 manquant (fallback raw default): ~326 ms
2) Fallback default.raw565 en cache RAM
   - Cache: /systems/_defaults/default.raw565 chargé une fois en RAM.
   - drawPng() sur systèmes lents (slowFlag 'L'): fallback direct depuis RAM (skip PNG decode).
3) drawPng(): décodage PNGle réactivé uniquement sur systèmes rapides
   - slowFlag 'L': pas de PNGle, fallback direct.
   - slowFlag 'N': tente SD.open PNG puis pngle; sinon fallback default.raw565.
4) openGif() optimisé (suppression pattern “probe SD.exists()+SD.open()”)
   - Pour les masks _defaults (.raw565pack + .meta): ouverture directe des fichiers.
5) Cache playlist hot-path optimisé
   - haveCache: suppression des SD.exists(playlistCachePath/playlistIdxPath)
     remplacé par SD.open(..., FILE_READ) pour déterminer existence.

Validation (via logs série):
- openGif(): le log “skip SD.exists” est observé sur le chemin mask.
- drawPng(): deltas cohérents avec les objectifs (latence fallback SD fortement réduite
