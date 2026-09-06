# Actualizar desde una versión anterior (v12 → v13)

[🇬🇧 English](UPGRADING.md) · [🇫🇷 Français](UPGRADING.fr.md) · 🇪🇸 **Español**

¿Ya usas RecalBoxDMD (firmware v12 o anterior, caja de herramientas de PC más antigua que v6243)? Esto es lo que realmente cambia y lo que hay que hacer — la mayor parte es opcional.

## 1. Actualiza la caja de herramientas de PC

Descarga la última release desde la [página de Releases](https://github.com/shan-aya/RecalBoxDMD/releases) e instálala sobre la anterior (o sustituye el `.exe` portable). Tus ajustes (tema, idioma, IP de la Recalbox, imagen de respaldo guardada...) se conservan automáticamente — viven en un archivo separado `RecalBoxDMD_prefs.json`, que la actualización no toca.

## 2. Flashea el nuevo firmware

Como siempre — el [instalador web en un clic](https://shan-aya.github.io/RecalBoxDMD/) (Chrome/Edge) flashea la v13 por USB en aproximadamente un minuto. No hace falta marcar «Borrar dispositivo» esta vez (eso es solo para una primera instalación o al venir de otro firmware) — un reflasheo normal conserva tu `config.ini` y todo lo que ya está en la tarjeta SD.

## 3. Regenera la caché — opcional, pero recomendado

**No se rompe nada si te saltas este paso.** El firmware v13 lee perfectamente un `systems_cache.dat` con el formato antiguo — cae automáticamente en el antiguo indicador «lento» por sistema entero cuando los nuevos datos por bucket no están presentes.

Pero la v13 introduce un cálculo más preciso de los sistemas «lentos» para el sistema de máscara: en lugar de un único indicador **«L»** para todo un sistema (MAME, FBNeo...), ahora se calcula **por subcarpeta alfabética**. Un sistema con una subcarpeta grande y varias pequeñas ya no penaliza innecesariamente a las pequeñas — la pantalla de espera se activará *con menos frecuencia* en las colecciones donde eso ocurría.

Para aprovechar esta mejora, regenera los dos archivos de caché con la caja de herramientas de PC actualizada:

- **Lo más rápido** — pestaña Avanzado → **Modo 6** (caché de juegos) y luego **Modo 7** (caché de sistemas), apuntando a tu tarjeta SD existente. Un par de minutos incluso en una colección grande, no toca para nada tus marquees/imágenes.
- **Lo más sencillo** — simplemente vuelve a ejecutar el **Modo 1** como una actualización normal; de todos modos reconstruye ambas cachés como parte del proceso completo.

Nada que configurar — el nuevo cálculo por bucket es automático. El umbral de «sistemas lentos» (pestaña Ajustes) también volvió a su valor por defecto original, **800** (archivos convertidos), para coincidir — solo se había subido a 5000 para compensar el antiguo cálculo por sistema entero; si habías personalizado este valor siguiendo el espíritu de los valores antiguos, revísalo.

## 4. Disfruta de las nuevas superposiciones en juego — opcional

Hi-Score, Info del juego, RetroAchievements y el Challenge RB mensual (ver el [README](README.es.md#superposiciones-en-juego--hi-score-info-del-juego-logros-y-challenge-rb)) necesitan sus scripts instalados en el lado de Recalbox. Ejecuta **Modo 9** una vez (o un nuevo **Modo 1**) — los instala junto con todo lo demás, y limpia automáticamente los nombres de scripts antiguos. Nada que configurar en el lado del DMD; empieza a funcionar en cuanto lances un juego que tenga datos disponibles.

## Lo que *no* necesitas hacer

- Volver a hacer scrape de tus juegos, reconstruir tu tarjeta SD desde cero, o volver a descargar el pack de 600 GIFs — nada de eso cambió.
- Recrear tus playlists — están intactas.
- Borrar manualmente los scripts antiguos de Recalbox antes de instalar — Modo 9/Modo 1 limpian los nombres antiguos por sí solos.
