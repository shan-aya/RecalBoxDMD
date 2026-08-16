# Changelog

Historial de **RecalBoxDMD — Raw565 Edition**, cubriendo tanto el **firmware ESP32** (incluida su página de configuración web) como la **caja de herramientas de PC**, desde el primer commit hasta hoy. Las entradas se agrupan por fecha; cada punto está etiquetado con la parte del proyecto a la que afecta.

[🇬🇧 English](CHANGELOG.md) · [🇫🇷 Français](CHANGELOG.fr.md) · 🇪🇸 **Español**

Este es un resumen seleccionado del historial interno de versiones del proyecto (76+ revisiones de firmware, 58+ de la config web, 34+ de la caja de herramientas, 48+ de la GUI) — agrupado por los hitos que realmente importan si usas el proyecto, no un volcado en bruto de cada micro-arreglo.

---

## 2026-08-16 — Imágenes de sistemas/géneros multilingües (FR/ES)

- **Caja de herramientas de PC**: el pack de respaldo `systems/_defaults` (insignias de género, pseudo-sistemas como Favoritos/Últimos Jugados/Portados/Todos los Juegos) ya está disponible en **francés y español**, 60/60 cada uno — el icono se conserva píxel por píxel (vectorizado, no solo ampliado), solo el texto se ha vuelto a renderizar y traducir. Los géneros aún no traducidos simplemente quedan en inglés, nunca falta ninguno.
- **Caja de herramientas de PC**: nuevo selector de **idioma de las imágenes de sistemas** (EN/FR/ES, con vista previa comparativa lado a lado) tanto en el Modo 1 (pipeline automático) como en el Modo 2 (pestaña Avanzado, descarga solo de `_defaults`) — `download_defaults()` siempre descarga primero el conjunto base en inglés (respaldo garantizado) y luego superpone los archivos traducidos del idioma elegido.
- **Caja de herramientas de PC**: el Modo 2 ahora siempre ofrece la galería de imagen de respaldo (cerrarla sin elegir vuelve al visual predeterminado del proyecto) en lugar de una pregunta sí/no condicionada a "aún no definido"; también se eliminaron los popups de confirmación tras elegir una (la elección ya es visible/se aplica de inmediato).
- **Caja de herramientas de PC**: corregido un problema real de lentitud en `_parallel_download_batch()` — `urlretrieve()` no tenía timeout, así que una sola conexión atascada dentro del pool de 16 hilos podía bloquear su hueco indefinidamente; ahora se establece un timeout de socket acotado durante el lote.
- **Recursos del firmware**: 15 logotipos de sistemas/géneros añadidos a `_defaults` — 10 que faltaban respecto al conjunto oficial de logos de Recalbox, más 5 incorporaciones muy recientes del canal alpha de Recalbox (Cassette Vision, EXL 100, ST-V, Vircon32, y el nuevo pseudo-sistema **Challenges**).

## 2026-08-13 — Preparación del lanzamiento público

- **Documentación**: reescritura completa del README en inglés/francés/español — capturas de pantalla, imágenes reales del dispositivo, referencia de modos, guía de hardware.
- **Firmware**: [instalador web](https://shan-aya.github.io/RecalBoxDMD/) — flashea el ESP32 directamente desde Chrome/Edge, sin Arduino IDE.
- **Caja de herramientas de PC**: instalador de Windows (`.exe` vía Inno Setup) y `.msi` (vía cx_Freeze), además de un `install_and_run.bat` de un clic para ejecutar desde el código fuente.

## 2026-08-11 — Vistas previas en vivo, pack de GIFs y acordeón de la pestaña Avanzado

- **Firmware / Config web**: elegir un tema de reloj o mover el control de brillo en la página de config web ahora **se muestra al instante en el panel físico**, antes incluso de guardar.
- **Firmware**: corrección del error «Reanudar DMD» ignorado mientras una vista previa de tema de reloj seguía activa; registro de diagnóstico del motivo del último reinicio al arrancar.
- **Caja de herramientas de PC**: los 8 radios planos de la pestaña Avanzado reorganizados en **5 categorías plegables** (descargas de GitHub / Gamelist / Imágenes / Cachés / Scripts); se añadieron el **Modo 10** (definir/generar la imagen de respaldo global) y el **Modo 11** (descarga en un clic del pack de ~600 GIFs); el umbral «L» de sistemas lentos pasó a ser un valor ajustable en la pestaña Configuración en vez de una constante fija.

## 2026-08-09 – 2026-08-10 — Ronda de estabilidad en hardware real

- **Firmware**: varias correcciones encontradas solo mediante pruebas directas en hardware, en torno a la máscara de sistemas lentos y la búsqueda rápida de juegos.
- **Caja de herramientas de PC**: el trabajo sobre el umbral del flag «L» comenzó aquí (ver arriba), motivado por diferencias reales de velocidad de tarjeta SD reportadas por usuarios.

## 2026-08-06 – 2026-08-07 — Estabilidad de memoria (heap)

- **Firmware**: dos correcciones independientes de fragmentación de memoria (un paso dedicado de generación de playlist, desactivación de la reconexión automática de WiFi) — sin incidentes después bajo pruebas reales intensivas, incluyendo un corte/reinicio del router en pleno uso.

## 2026-08-05 — Fusión de `dev/tous-txt-filter`

- **Caja de herramientas de PC**: herramientas de playlist y la base del banco de GIFs de GitHub fusionadas a la rama principal.

## 2026-08-03 — Revisión del flujo de primer arranque

- **Firmware / Config web**: la página de configuración de primer arranque / punto de acceso WiFi ampliamente reelaborada tras pruebas reales de primer uso.
- **Caja de herramientas de PC**: actualizaciones correspondientes en el selector de imagen de respaldo y en los popups relacionados con mensajes de primer uso/reinicio.

## 2026-08-01 – 2026-08-02 — La reescritura de `cache_master_gifs`

- **Firmware + Config web + Caja de herramientas de PC**: reescritura en tres partes del pipeline de playlists de GIFs en torno a `cache_master_gifs.dat`, un índice maestro de todos los GIFs ya presentes en la tarjeta SD — acelera la navegación de carpetas en la página web Medios y la construcción de playlists en la pestaña Playlist de la caja de herramientas, y hizo mucho más fiables las subidas masivas desde la página web (ajuste de tamaño de búfer, serialización de subidas para evitar `ERR_INVALID_CHUNKED_ENCODING`).

## 2026-07-26 – 2026-07-29 — Ronda de depuración en hardware real

- **Firmware**: investigaciones sobre el uso de memoria y la conexión MQTT en hardware real; varias regresiones encontradas y corregidas de este modo.
- **Caja de herramientas de PC**: el Modo 9 (instalar scripts de Recalbox) reforzado tras diagnosticar un caso real de fallo SMB/inicio de sesión de invitado en una Recalbox real.

## 2026-07-22 – 2026-07-23 — Pipeline del Modo 1 y detección de red

- **Caja de herramientas de PC**: `detect_recalbox_share()` (detección automática por NetBIOS de `\\RECALBOX\share`) y `resolve_recalbox_ip()`; el flujo de instalación de scripts de Recalbox reelaborado de principio a fin tras pruebas reales.

## 2026-07-20 – 2026-07-21 — Auditoría de traducción e instalador de scripts

- **Caja de herramientas de PC**: auditoría completa de traducción FR/EN/ES con paridad estricta de claves entre los tres idiomas; se lanzó el **Modo 9** — instala los scripts de usuario de Recalbox (puente marquee, recuperación WiFi, sincronización de config web) directamente por el recurso compartido de red de la Recalbox, sustituyendo un enfoque FTP anterior que la Recalbox de destino en realidad no soportaba.

## 2026-07-14 — 10º tema de reloj

- **Firmware**: «Level 1-1» — una recreación con scroll del primer nivel de Super Mario Bros — añadido como 10º tema de reloj.

## 2026-07-13 — Interfaz trilingüe

- **Firmware + Config web + Caja de herramientas de PC**: francés/inglés/español añadidos en todas partes — la página de config web del DMD y la caja de herramientas de Windows comparten el mismo idioma, enviado automáticamente al DMD al principio del Modo 1.

## 2026-07-11 — Imágenes de respaldo y detección de la versión de Recalbox

- **Caja de herramientas de PC**: selector de imagen de respaldo personalizada (elige qué se muestra cuando nada más coincide); se introduce el **selector «Versión de Recalbox»** (10.x / 9.x / legacy), para que la herramienta lea la etiqueta correcta de `gamelist.xml` (`<logo>`/`<thumbnail>`/`<image>`) y la carpeta de medios adecuada según tu configuración.

## 2026-07-10 — Llega la interfaz gráfica

- **Caja de herramientas de PC**: `RecalBoxDMD_GUI.py` v1 — una interfaz Tkinter que envuelve la herramienta de consola; copia a la SD reanudable tras una interrupción; refinamiento constante de diseño/UX en los días siguientes (pestaña Avanzado, panel de progreso, popup de exploración de la tarjeta SD).

## 2026-07-08 — Nace la caja de herramientas de PC

- **Caja de herramientas de PC**: versión base de `RecalBoxDMD_tool.py` (consola) — extracción de `gamelist.xml`, conversión PNG→raw565/GIF→raw565pack, construcción de la caché. El Modo 8 (verificación de imágenes faltantes) se lanzó desde el primer día.

## 2026-07-02 — Nace la página de configuración web

- **Firmware / Config web**: primera versión de la página de configuración en el navegador — FR/EN/ES con detección automática del idioma del navegador, tooltips en cada campo, subida/subida múltiple/eliminación de GIFs, regeneración automática de playlists, y el DMD pausándose con un mensaje de estado durante las operaciones de SD. Le siguió una serie densa de correcciones de fiabilidad el mismo día: evitar timeouts del watchdog en bucles largos de SD, workarounds de `mkdir`/`rmdir` para las peculiaridades de FAT32 de solo lectura, un mensaje de estado flotante persistente.

## 2026-07-01 — Llegan los temas de reloj

- **Firmware**: integración de `retro_clock` — 9 temas de reloj en pixel-art (Super Mario, Tetris, Pac-Man, Space Invaders, Pong, Neon, Matrix, Fire, Rainbow), sustituyendo el antiguo renderizador de dígitos simple.

## 2026-06-11 – 2026-06-29 — Primeros refuerzos

- **Firmware**: optimizaciones de renderizado raw565/raw565pack; subcarpetas alfabéticas `A..Z/#` añadidas específicamente para evitar las ralentizaciones de FAT32 a partir de ~800 archivos por carpeta; primer reloj multi-estilo con brillo configurable; corregido un error de bloqueo de playlist.

## 2026-06-10 — Nace el proyecto: el fork Raw565

- **Firmware**: fork de [RetroBoxLED de Jamyz](https://github.com/Jamyz/RetroBoxLED). El pipeline original de decodificación PNG/GIF se sustituye por un formato propio **raw565**/**raw565pack**, una **caché de juegos indexada por bigramas** (`games_cache.bin`), y la **máscara «L»** de sistemas lentos — la base que permite que un fullset MAME de 30 000 juegos se muestre en milisegundos, sin pantalla negra entre juegos.

---

*Las fechas provienen de las cabeceras de versión que se mantienen al principio de cada archivo fuente (`RecalBox_DMD.ino`, `web_config.h`, `RecalBoxDMD_tool.py`, `RecalBoxDMD_GUI.py`) — la convención interna de changelog del proyecto, condensada aquí para mayor legibilidad.*
