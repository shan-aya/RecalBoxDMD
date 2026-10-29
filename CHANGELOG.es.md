# Changelog

Historial de **RecalBoxDMD — RawEdition v3.x**, cubriendo tanto el **firmware ESP32** (incluida su página de configuración web) como la **caja de herramientas de PC**, desde el primer commit hasta hoy. Las entradas se agrupan por fecha; cada punto está etiquetado con la parte del proyecto a la que afecta.

[🇬🇧 English](CHANGELOG.md) · [🇫🇷 Français](CHANGELOG.fr.md) · 🇪🇸 **Español**

Este es un resumen seleccionado del historial interno de versiones del proyecto (185+ revisiones de firmware, 64+ de la config web, 43+ de la caja de herramientas, 62+ de la GUI) — agrupado por los hitos que realmente importan si usas el proyecto, no un volcado en bruto de cada micro-arreglo.

---

## 2026-10-29 — Caja de herramientas 14286: el Modo 3 ya no crea tarjetas de texto

**Caja de herramientas del PC build 14286** (firmware 3.98 y demonio Pi 3.74 sin cambios)
- **Modo 3 sin tarjetas de texto.** El Modo 3 extrae las imágenes en bruto del `gamelist.xml` para DMD GIF Creator; las tarjetas de texto generadas para los juegos sin imagen se convertían como imágenes reales, con un resultado catastrófico. Los juegos sin imagen vuelven a figurar en `images_manquantes.txt`. Los demás modos conservan las tarjetas de texto (el Modo 18 las regenera por separado). Una carpeta ya extraída conserva sus tarjetas: bórrela o ejecute el Modo 3 en una carpeta vacía.
- **Logos reales**: se retira el logo `dice` del conjunto (mostraba ATARI CLASSICS; DICE es un emulador, no una consola). 136 logos base.

## 2026-10-28 (2) — Demonio Raspberry Pi v3.74 y caja de herramientas 14184: los logos reales se envían a la Pi por Wi-Fi

**Demonio Raspberry Pi v3.74 · Caja de herramientas del PC build 14184**
- **Modo 20: envío de los logos reales a la Raspberry Pi por Wi-Fi.** Tras la descarga, si una Pi (demonio 3.74 o superior) responde en la red, la caja de herramientas le envía directamente la carpeta `logos-reels` (167 archivos, unos 4 s): ya no hace falta sacar su tarjeta SD. El demonio recarga los índices y redibuja el logo en pantalla al instante. Si no se encuentra ninguna Pi: un mensaje, y la copia a la tarjeta SD como antes (el ESP32 sigue con la copia a tarjeta SD).
- **Demonio Pi**: nuevo tipo de envío `POST /upload?kind=reels&dir=base|r_us|r_eu|r_jp` (nombres, carpetas y tamaños comprobados; archivo temporal y luego renombrado, nunca un logo truncado) y `POST /reels-refresh`. La caja de herramientas rechaza un demonio más antiguo (guardaría los archivos como GIF).
- Probado en una Raspberry Pi 3B: 167 archivos enviados, SHA-256 idénticos al original.

## 2026-10-28 — Firmware v3.98, demonio Raspberry Pi v3.73 y caja de herramientas 14083: logos reales de consolas

**Firmware v3.98 · Demonio Raspberry Pi v3.73 · Caja de herramientas del PC build 14083 (Modo 20)**
- **Logos reales de consolas**: nueva opción *Logos reales de consolas* en la página Pantalla (*Desactivado / Todos los temas / Solo los temas marcados*). El DMD muestra entonces el logo real de cada consola (137 sistemas, variantes EE. UU. / Europa / Japón elegidas por la región anunciada por Recalbox, base = logo «mundo») **antes** de los logos de región y base del tema; la variante de idioma del tema conserva la prioridad y los logos ausentes del set conservan el logo del tema. Funciona aunque la carpeta del tema no esté en la SD.
- **La lista para marcar se construye sola**: temas instalados en la SD (`GET /reels_installed`) + temas ya anunciados por Recalbox (`reels_seen` en `config.ini`, 24 como máximo) + opción para marcar automáticamente los temas nuevos — nada que mantener cuando salgan temas nuevos.
- **Mismo comportamiento en la Raspberry Pi** (demonio): mismas claves de `config.ini`, mismo orden, misma página, misma ruta. Verificado en una Raspberry Pi 3B real (demonio compilado en la Pi, registro del demonio): elige el archivo correcto (base y luego `r_jp/` cuando la Recalbox anuncia la región Japón), redibuja el logo en pantalla al instante cuando cambia la región y marca automáticamente el tema anunciado.
- **Verificado en el DMD (ESP32) contra una Recalbox**: cambiar la región del tema en Recalbox cambia el logo real mostrado en el DMD.
- **Pantalla de configuración (ESP32) corregida**: el mensaje de fondo (la dirección de la página) nunca se establecía, así que «Guardando…» dejaba la línea 2 vacía al expirar; la dirección vuelve ahora a los 5 s.
- **Modo 20 del toolkit — Logos reales** (*Download from GitHub*): descarga solo la carpeta `logos-reels`, para actualizarla por separado; los Modos 1 y 13 la listan premarcada mientras falte o esté obsoleta en la SD. Columna izquierda de la pestaña Avanzado reducida (516 → 491 px) para que quepa un quinto modo.
- **Paquete**: `logos-reels` (137 logos base + `r_us` / `r_eu` / `r_jp`) es un tema *extra* del paquete (`EXTRA_THEMES` en `build_theme_package.py` v10): el flujo de los lunes nunca lo borra. Logos de ScreenScraper (CC BY-NC-SA 4.0), ver el `LISEZMOI.txt` de la carpeta.
- **Plaquette n.º 12** (FR/EN/ES).
- **Agradecimientos**: **Mighty Jim** por su apoyo («¡un café es un café!») y **Beavisx** por la idea de los logos realistas, y gracias por el café sabor arce 🇨🇦.

## 2026-10-27 (2) — Firmware v3.97, demonio Raspberry Pi v3.72 y caja de herramientas 13982: cada DMD anuncia su hardware, avisos de actualización adaptados a la Pi

**Firmware v3.97 · Demonio Raspberry Pi v3.72 · Caja de herramientas del PC build 13982**
- **Cada DMD anuncia su hardware** (campo `platform` de `/load`: `esp32` o `rpi`). Las páginas web y la caja de herramientas lo usan para comparar una Raspberry Pi con la versión publicada **del demonio Pi** (clave `pi` del manifiesto) y un ESP32 con la del **firmware**: ya no se invita a una Pi a instalar el firmware ESP32.
- **Caja de herramientas: aviso de la Pi** — a un DMD Raspberry Pi desactualizado se le ofrece la actualización del demonio (página Ajustes del DMD o Modo 17), no el Web Installer ni el Modo 14.
- **Caja de herramientas: el aviso de los scripts de Recalbox vuelve en cada inicio** mientras el lote anunciado por el DMD sea más antiguo que el del repositorio, nombrando el DMD afectado (antes solo se mostraba una vez por versión publicada).

## 2026-10-27 — Scripts Recalbox n.º 7 (logos daphne/Singe), demonio Raspberry Pi 3.71

**Scripts Recalbox** (número de conjunto 6 → 7, instalación con el Modo 9)
- **Corrección: los logos de juego del sistema `daphne` (Singe incluido) no aparecían al navegar**, solo una vez lanzado el juego. En este sistema cada juego es una carpeta `<juego>.daphne`, que el script `marquee` (v44) tomaba por una subcarpeta de navegación y respondía con el logo del sistema.

**Demonio Raspberry Pi 3.71** (solo páginas web, mismo binario que la 3.70)
- **Corrección: la página Ajustes ofrecía a una Pi el firmware ESP32 (3.96)** — ahora compara la Pi con la versión `pi` del manifiesto, como la insignia de la página de inicio.

## 2026-10-26 (4) — Documentación: sección Diagnóstico del README, tutorial del pack ULTIMATE, plaquette n.º 11, agradecimientos

**Documentación** (sin cambios de firmware, demonio ni caja de herramientas)
- **README (ES/EN/FR): nueva sección «Diagnóstico Wi-Fi / Recalbox»** — cómo iniciarlo (BOOT, acción 12, página web), qué mide, cómo leer las 4 páginas del panel y la página web, hallazgos típicos, Raspberry Pi. La tabla de páginas de la configuración web incorpora la página **Diagnóstico**, la de modos de la caja de herramientas el **Modo 19** y el envío de la imagen de reserva por Wi-Fi (**Modo 10**); la sección Playlist describe el envío de carpetas de GIF a la Pi por Wi-Fi, la ventana de fin de eliminación y el registro.
- **Nueva sección «Tutoriales paso a paso»**: marquees animados e **instalación del pack ULTIMATE GIFS DLC** (~11 000 GIF de eLLuiGi / RpiTeaM), en ES/EN/FR con capturas reales: [docs/tutorials/ultimate-gif-pack.es.md](docs/tutorials/ultimate-gif-pack.es.md).
- **Plaquette n.º 11** (ES/EN/FR): las novedades de las últimas versiones (`medias/marketing/plaquette_11_nouveautes_*.png`).
- **Agradecimientos**: **Mighty Jim** por su ayuda para depurar la herramienta de diagnóstico; **Dodoche3826** por su apoyo («gracias por el bol de café ^^»).

## 2026-10-26 (3) — Firmware v3.96, demonio Raspberry Pi v3.70 y caja de herramientas del PC build 13881: imagen de respaldo por Wi-Fi, preferencias guardadas en la versión instalada, detección del DMD reforzada

**Firmware v3.96 · Demonio Raspberry Pi v3.70 · Caja de herramientas del PC build 13881**
- **Imagen de respaldo por Wi-Fi** (ESP32 y Raspberry Pi): en la galería « Imagen de respaldo » de la caja de herramientas, el nuevo botón **« Enviar la imagen elegida al DMD (Wi-Fi) »** sustituye `default.raw565` directamente en el DMD, sin sacar la tarjeta SD. El DMD escribe un archivo temporal, comprueba que mide exactamente 8192 bytes y sustituye la imagen anterior; un envío interrumpido o de tamaño incorrecto deja intacta la imagen anterior. Un firmware / demonio demasiado antiguo se detecta y no se envía nada.
- **Versión instalada (Setup)**: el Setup instala en « Program Files », donde un usuario normal no puede escribir; el archivo de preferencias nunca se creaba allí, así que el idioma, el tema, la dirección de la Recalbox, la del DMD y la imagen de respaldo no se conservaban entre arranques. Las preferencias se escriben ahora en `%APPDATA%\RecalBoxDMD` cuando la carpeta del exe no es modificable; la versión portable conserva su archivo junto al exe. La imagen de respaldo elegida en la galería también se recuerda al reiniciar el exe portable (las imágenes importadas se guardan en `%APPDATA%\RecalBoxDMD\default_images`).
- **« DMD: no encontrado » al iniciar**: la búsqueda prueba también el nombre `dmd-marquee.local` y recorre **todas** las tarjetas de red activas (VPN, WSL, Hyper-V o varias tarjetas ya no ocultan la red del DMD), con tiempos más largos y una segunda pasada; el registro explica lo que se probó. Nuevo campo **« Dirección del DMD (IP o nombre) »** en la pestaña Ajustes, usado en primer lugar.

## 2026-10-26 (2) — Firmware v3.96, demonio Raspberry Pi v3.70: modo diagnóstico Wi-Fi / Recalbox

**Firmware v3.96 · Demonio Raspberry Pi v3.70**
- **Nuevo modo diagnóstico** para comprobar la señal Wi-Fi y el enlace con la Recalbox, que se lanza de tres maneras: la nueva **acción 12 de la página Botón** («Diagnóstico Wi-Fi / Recalbox»), una **pulsación corta del botón BOOT del ESP32** (incluso sin botón físico), o la nueva **página Diagnóstico** de la configuración web (también en el menú de inicio). Funciona en bucle hasta una nueva pulsación.
- **Qué mide**, una vez por segundo en una ventana deslizante de las últimas 20 pruebas: intensidad de la señal (dBm) y calidad, canal, punto de acceso (BSSID), y luego un PING a la Recalbox con **pérdida, latencia mín / media / máx y jitter**, la antigüedad del último paquete, los cortes UDP y, en el ESP32, las **reconexiones Wi-Fi con el motivo de la última desconexión** y el historial de las últimas 8 conexiones (a qué punto de acceso se unió el DMD, y cuándo). El panel muestra 4 páginas de 4 s; la página web muestra los valores, dos curvas y una **guía «Cómo leer estos valores»** (umbrales, causas, qué hacer), y puede generar un **informe de texto** para copiar o descargar en un mensaje de asistencia.
- **Salir del modo**: nueva pulsación, salir de la página, cambiar de pestaña o cerrar el navegador (y, por seguridad, 8 s sin señales de vida de la página).
- **Corregido (ESP32)**: el DMD se unía al **primer** punto de acceso que respondía en lugar del **más fuerte**. Con una red en malla o un repetidor podía engancharse a un punto de acceso lejano (−75 dBm en lugar de −52 dBm) y dejar de recibir nada mientras mostraba «conectado». Ahora escanea todos los canales y elige la mejor señal (el escaneo añade un breve retraso en cada conexión).
- **Corregido (ESP32)**: **«Reanudar DMD»** tras un arranque en modo configuración (primer arranque, o `first_boot=1`) dejaba una **pantalla negra**: el índice de la lista nunca se cargaba. Ahora se carga al reanudar.
- **Raspberry Pi**: mismo modo diagnóstico (acción 12 de la página Botón y la misma página web). No hay botón BOOT en el Pi; el historial de reconexiones Wi-Fi solo existe **mientras se ejecuta un diagnóstico**; con Ethernet, el panel muestra «ETHERNET / SIN WIFI».

## 2026-10-26 — Caja de herramientas del PC build 13579: imagen de respaldo recordada, comprobación de la eliminación en la pestaña Playlist

**Caja de herramientas del PC build 13579**
- **Imagen de respaldo**: la imagen elegida en la galería (por ejemplo `default_rgbdual2`) no se recordaba en el siguiente arranque de la versión portable, y volvía el antiguo logo de Recalbox con fantasmas. La elección se recupera ahora en cada arranque, las imágenes importadas se guardan en `%APPDATA%\RecalBoxDMD\default_images` y se listan en la galería, y si la imagen guardada ha desaparecido el registro lo indica y se aplica la imagen predeterminada.
- **Pestaña Playlist**: tras « Eliminar », una ventana « Eliminación terminada » muestra el resultado comprobado en la tarjeta; cada acción de la pestaña se escribe en el registro; la eliminación también quita las líneas correspondientes de `cache_master_gifs.dat`.

## 2026-10-25 (3) — Firmware v3.89, demonio Raspberry Pi v3.67: etiquetas de la sección « Reposo de Recalbox » aclaradas; tutorial de marquesinas animadas

**Firmware v3.89 · Demonio Raspberry Pi v3.67**
- **Sección « Reposo de Recalbox » (página Pantalla)**: las etiquetas indican ahora que, **desmarcada, el DMD muestra la playlist con los widgets y el reloj según tus ajustes** (en vez de « playlist simple »), tanto para la demo de juegos como para los clips de vídeo; marcada (por defecto): la marquesina del juego en curso. Sin cambios de comportamiento. ESP32 y Raspberry Pi.
- **Nuevo tutorial** (FR/EN/ES, con capturas): crear marquesinas animadas con DMD GIF Creator (modo AUTO) y la caja de herramientas, hasta una tarjeta SD con archivos `.raw565pack`: [docs/tutorials/animated-marquees.es.md](docs/tutorials/animated-marquees.es.md).

## 2026-10-25 (2) — Firmware v3.88, demonio Raspberry Pi v3.66 y caja de herramientas 13377: el DMD se reanuda solo al cerrar la página de configuración, envío de carpetas de GIF a la Pi

**Firmware v3.88 · Demonio Raspberry Pi v3.66 · Caja de herramientas del PC build 13377**
- **El DMD ya no se queda en modo configuración al cerrar la página** (cruz, cierre del navegador, fallo). La página avisa al DMD de que se va y este se reanuda 10 s después; actualizar (F5) o pasar a otra página de configuración cancela la reanudación. Si el navegador falla o se corta el Wi-Fi, el DMD se reanuda tras 3 min sin señales de vida. Sin reanudación automática durante el primer arranque, en la pantalla del punto de acceso Wi-Fi ni durante la generación de una playlist; los mensajes de la caja de herramientas y los envíos de archivos cuentan como actividad. Mismo comportamiento en ESP32 y en Raspberry Pi.
- **Caja de herramientas, pestaña Playlist: botón « Enviar a la Pi (Wi-Fi)… »** (solo Raspberry Pi). Envía carpetas de GIF del ordenador a `gifs/` de la Pi por Wi-Fi, sin sacar su tarjeta: misma selección de carpetas que « Añadir una carpeta del PC… », confirmación, progreso y resumen. Un DMD ESP32 se rechaza (copia en su tarjeta SD). La tarjeta de la Pi solo acepta ASCII: los acentos de los nombres de carpetas y archivos se convierten (é pasa a e) y el resumen lo indica.

## 2026-10-25 — Firmware v3.87, demonio Raspberry Pi v3.65 y caja de herramientas 13277: enlace de descarga en los avisos de actualización

**Firmware v3.87 · Demonio Raspberry Pi v3.65 · Caja de herramientas del PC build 13277**
- **Página web del DMD (ESP32 y Pi)**: los avisos « scripts de Recalbox » y « caja de herramientas del PC » tienen ahora un enlace **« Descargar el toolkit »** junto a « Notas de versión » (página de la última versión), traducido como el resto del aviso.
- **Caja de herramientas**: el aviso de nueva versión muestra también **« Descargar la última versión »**, junto a « Notas de versión » y « GitHub ».

## 2026-10-24 (3) — Firmware v3.86, demonio Raspberry Pi v3.64 y caja de herramientas 13177: modo Recalbox nativo, Visual Pinball en la Pi

**Firmware v3.86 · Demonio Raspberry Pi v3.64 · Caja de herramientas del PC build 13177**
- **Modo Recalbox nativo: vuelta a la lista de reproducción cuando la Recalbox se apaga o deja de responder.** Antes la pantalla se quedaba en la última imagen de EmulationStation. El DMD (ESP32 y Pi) devuelve ahora la pantalla a la lista cuando la Recalbox se apaga (unos segundos) o no responde a dos PING seguidos (unos 30 s), y recupera el flujo de EmulationStation en cuanto vuelve.
- **Modo nativo (firmware): por fin se reproduce la animación de arranque festiva.** La fecha nunca se guardaba en este modo, así que no se detectaba ninguna fiesta; ahora sí (sincronización de la hora silenciosa). La animación festiva suena a partir del arranque siguiente al que guardó la fecha.
- **Visual Pinball en la Pi (modo normal y modo nativo): las mesas se muestran.** Dos causas del lado de Recalbox, corregidas por el script `dmd_vpx_config` v4 (instalado por el Modo 9): la dirección del DMD en la configuración de VPX podía estar caducada, y Recalbox borraba la opción del plugin DMD en cada lanzamiento de mesa. El script toma la dirección de la opción ZeDMD de Recalbox (o del DMD que responde) y vuelve a poner la opción « DMD externo » en ZeDMD antes de cada lanzamiento; no toca nada si elegiste otra pantalla. El script `marquee` v55 avisa al DMD al apagar.
- **Caja de herramientas: enlace « GitHub »** junto a « Notas de versión » en el aviso de actualización.
- **Conviene saber — actualizar en modo Recalbox nativo.** Con un firmware 3.84 o anterior, la actualización desde la página web falla en modo nativo (`echec:heure`: la hora no se sincroniza). Desmarca *modo Recalbox nativo*, **Guardar y reiniciar**, lanza la actualización y vuelve a marcarlo. Desde la 3.85 (por tanto la 3.86), la actualización funciona directamente.

## 2026-10-24 (2) — Caja de herramientas del PC build 13077: textos traducidos

**Caja de herramientas del PC build 13077**
- **Tres textos que seguían en francés se traducen ahora** (francés, inglés, español): el diálogo de reintento tras un fallo de copia en la tarjeta SD, el mensaje de error al abrir la ayuda y el título de error de la ventana de temas. Ningún otro cambio respecto al build 12977.

## 2026-10-24 — Firmware v3.83 y demonio Raspberry Pi v3.63: zona horaria en « Ajustes del reloj », aviso de actualización traducido, animaciones protegidas

**Firmware v3.83 · Demonio Raspberry Pi v3.63**
- **Zona horaria en los ajustes del reloj.** La página Reloj (abierta por el botón **« Ajustes del reloj »** de la página Widgets) ofrece ahora la zona horaria en su primera línea; es el mismo ajuste que el de la página Lugar.
- **Aviso de actualización traducido.** El aviso que anuncia una nueva versión (firmware, scripts, caja de herramientas) se quedaba en el idioma de la carga de la página, a menudo el francés: ahora sigue el selector de idioma.
- **Animaciones de arranque protegidas.** La carpeta `gifs/_boot` queda oculta de la lista de carpetas, la página web ya no puede borrarla, y solo se pueden enviar a ella los seis archivos de animación conocidos (Modo 19 de la caja de herramientas). Las próximas versiones descargan sus animaciones de la carpeta `boot_anim/` del repositorio (`binaries/` sigue reservada al firmware).

## 2026-10-23 (2) — Caja de herramientas del PC build 12977: Modo 19, animaciones de arranque por Wi-Fi

**Caja de herramientas del PC build 12977**
- **Nuevo Modo 19 — Animaciones de arranque (Wi-Fi)** (categoría « FIRMWARE DEL DMD »): envía por Wi-Fi al DMD, **ESP32 o Raspberry Pi**, la animación por defecto Shan_ayA y las cinco de fiesta, descargadas de GitHub. Cada archivo se comprueba (tamaño y huella) tras el envío; los que el DMD ya tiene no se vuelven a enviar; un botón reinicia el DMD para ver la animación. Las animaciones de fiesta requieren un firmware ESP32 3.67 o posterior (Raspberry Pi 3.62 o posterior).
- Las animaciones tienen ahora su propia carpeta `boot_anim/` en GitHub (`binaries/` sigue reservada al firmware; la copia antigua se conserva para los firmwares 3.63 a 3.82). Ayuda actualizada. Ningún otro cambio respecto al build 12876.

## 2026-10-23 — Firmware v3.82 y demonio Raspberry Pi v3.62: líquido en una cubeta, Arena 2, lámpara de lava, arranque y alertas en la Pi

**Firmware v3.82** (de la v3.68 a la v3.82)
- **Líquido en una cubeta.** El líquido está ahora en una cubeta (suelo y dos paredes, del ancho del panel) sobre una barra triangular en el centro inferior: la cubeta **bascula** hasta el ángulo máximo del líquido, con una duración de fase elegida al arrancar (5 a 10 s) y una **velocidad elegida en cada balanceo** (ángulo máximo alcanzado y pausa, o balanceo interrumpido antes). Las olas golpean las paredes y lanzan **salpicaduras** (hasta 48 gotas, más numerosas y vivas cuanto más rápido es el frente) en los líquidos fluidos; los líquidos espesos (jarabe, cera, miel) se **adhieren** a las paredes (menisco, película que se escurre despacio).
- **Arena 2 rehecha** (como un cuadro de arena de verdad). Reserva fuera de pantalla, capas finas de cuatro tonos, un color pesado que cae en grumos y uno ligero que deriva como una nube (peso propio de cada color y de cada abertura), corriente lateral propia de cada abertura, aberturas con vida aleatoria (hasta 6 a la vez) y capas que se deslizan cuando una capa pesada se posa sobre una ligera; el panel se llena hasta cerca del 80 %.
- **Lámpara de lava con punto caliente.** El calor parte de un punto caliente en el centro del fondo y se difunde hacia los bordes: primero se anima el centro y luego la actividad llega a los lados, con un halo de calor en el aceite.

**Demonio Raspberry Pi v3.62**
- **Las mismas ambientaciones que el DMD ESP32**: Arena 2, líquido en una cubeta, lámpara de lava con punto caliente (código idéntico, tomado automáticamente del firmware).
- **Animación de arranque y temporadas.** La Pi reproduce la misma animación Shan_ayA que el DMD, y su versión festiva (Halloween, Navidad, Año Nuevo, San Valentín, Pascua) según la fecha. Se entrega con la actualización del demonio: nada que descargar ni configurar. Rutas `GET /boot-anim` y `GET /boot-season`, y el Modo 19 también puede enviarla.
- **Las mismas alertas de estado que el DMD**: « ni wifi / ni recalbox » (sin red) y « RecalBox sin conexión » (dos PING seguidos sin respuesta), parpadeantes 5 s y luego vuelta a la lista; « RecalBox conectada » sin el emoticono, centrada sobre la tinta de las letras.

## 2026-10-22 (4) — Caja de herramientas del PC build 12876: animaciones de arranque en el Modo 1

**Caja de herramientas del PC build 12876**
- **El Modo 1 copia las animaciones de arranque** en la tarjeta SD (`gifs/_boot/`): la de por defecto Shan_ayA y las cinco de fiesta (Halloween, Navidad, Año Nuevo, San Valentín, Pascua), descargadas de GitHub y comprobadas (tamaño y huella) tras la copia. En la medida de lo posible: si GitHub no está accesible, el proceso continúa y el DMD las descarga él mismo (firmware v3.67). No para una tarjeta de Raspberry Pi. Ningún otro cambio respecto al build 12775.

## 2026-10-22 (3) — Firmware v3.67: animaciones de arranque de temporada

**Firmware v3.67**
- **Animaciones de arranque de temporada.** Alrededor de cada fiesta, el DMD reproduce una versión temática de la animación de arranque Shan_ayA (nuevos colores, decorados y una pupila en forma de corazón para San Valentín) y luego vuelve al logotipo por defecto. **Halloween** (15 oct. – 2 nov.: naranja, pupila rasgada, murciélagos), **Navidad** (10 – 26 dic.: rojo, nieve), **Año Nuevo** (27 dic. – 3 ene.: oro, confeti y estrellas), **San Valentín** (7 – 14 feb.: rosa, corazones) y **Pascua** (desde 7 días antes hasta el día siguiente al Domingo de Pascua, calculado cada año: huevos). Las ventanas rodean la fecha, no es solo el propio día.
- **Nada que configurar.** El DMD memoriza la fecha en cada sincronización de la hora (no tiene reloj al encenderse) y descarga él mismo los archivos de temporada, de una vez, desde 21 días antes de una fiesta (2 intentos al día, silencioso si GitHub no está accesible). Si falta un archivo se reproduce la animación por defecto. La animación solo se reproduce al encender: un DMD encendido de forma continua cambia en su siguiente reinicio. El Modo 1 de la caja de herramientas PC también copia los archivos de temporada en una tarjeta SD nueva.
- **Página web.** Nuevas rutas de diagnóstico: `GET /boot-season?d=AAAAMMDD` (qué animación se reproduciría en esa fecha) y `GET /boot-anim?s=<fiesta>`.
## 2026-10-22 (2) — Firmware v3.66: un reinicio ya no cancela la actualización

**Firmware v3.66**
- **Corregido — una actualización OTA cancelada por un reinicio.** Tras una actualización, la nueva versión quedaba « por verificar » durante 60 segundos, y cualquier reinicio en ese plazo (el botón « Guardar y reiniciar » de la página web, una pulsación del botón, un comando de la Recalbox) devolvía la versión anterior. Un reinicio **voluntario** ahora valida la nueva versión en cuanto el DMD lleva al menos 20 segundos funcionando. Los fallos reales al arrancar siguen protegidos: se mantiene la vuelta automática a la versión anterior.
- **Resultado de la actualización honesto.** Si funciona una versión más antigua mientras la última actualización anunciaba una más reciente, la página muestra ahora un fallo explícito (« versión más antigua en servicio ») en lugar de un falso « correcto ». Este segundo punto solo actúa a partir de un DMD que ya esté en 3.66.

## 2026-10-22 — Firmware v3.65: animación de arranque y mensajes de estado

**Firmware v3.65**
- **Animación de arranque.** Al encender, el panel reproduce una breve animación del **logotipo Shan_ayA** (el nombre gira alrededor del ojo, la pupila lo sigue); el título aparece encima durante 2,5 s y luego la animación continúa 2 s más antes del arranque normal. No hay nada que instalar a mano: en el primer arranque de este firmware, si hay una WiFi configurada, el DMD **descarga él mismo la animación** desde GitHub (unos segundos, con el panel apagado; comprobación SHA-256 y tamaño exacto; 2 intentos como máximo). El Modo 1 de la caja de herramientas PC también la copia en una tarjeta SD nueva (`gifs/_boot/`). Sin el archivo, se muestra el título clásico exactamente como antes.
- **Mensajes de estado.** Desaparecen los emoticonos de « Recalbox conectada » y « offline »; la alerta francesa « Pas de wifi / ni Recalbox » pasa a **« ni wifi / ni recalbox »**; las líneas se centran sobre las letras mismas (desaparece un desfase de 2 píxeles en « ni wifi »).
- **Fiabilidad.** Un envío por la página web ahora indica un error cuando la tarjeta SD corta una escritura, en lugar de responder « OK » con un archivo truncado; nueva ruta `GET /boot-anim` que devuelve el tamaño y la huella de la animación.

## 2026-10-21 (3) — Caja de herramientas del PC build 12775: logo Shan_ayA y enlace de apoyo

**Caja de herramientas del PC build 12775**
- El texto « GUI - Shan_ayA 2026 » al pie de la ventana se sustituye por el enlace **☕ Invítame a un café** (globo al pasar el ratón, abre la página de Buy Me a Coffee; FR/EN/ES) y el **logo Shan_ayA** se muestra a la derecha, sobre el fondo del tema. Ningún otro cambio respecto al build 12675.

## 2026-10-21 (3) — Firmware v3.60 y demonio Raspberry Pi v3.61: páginas web

**Firmware v3.60 · Demonio Raspberry Pi v3.61**
- **Páginas web.** El aviso (toolkit, firmware, scripts) empieza ahora debajo del selector de idioma y del enlace Ayuda, que ocultaban su final en pantallas estrechas; el subrayado del enlace « Invítame a un café » es más espaciado. Sin cambios de lógica (el binario del demonio es idéntico al de la 3.60).

## 2026-10-21 (2) — Demonio Raspberry Pi v3.60: botón ausente, pie de página

**Raspberry Pi**
- **Botón ausente o falsa pulsación.** Misma corrección que el firmware 3.59: la pulsación muy larga se activa solo al soltar, y las acciones « reiniciar » y « rescate Wi-Fi » provenientes del botón se ignoran durante los 5 primeros minutos tras el arranque del demonio.
- **Páginas web.** Nueva imagen de inicio y la mención « © Shan_ayA 2026 » con un enlace « Invítame a un café » al pie de cada página.
- Las fuentes de la carpeta `raspberry-pi/` corresponden exactamente a esta versión (incluyen también la actualización desde la página web añadida en la 3.59).

## 2026-10-21 — Firmware v3.59: un botón ausente ya no puede reiniciar el DMD

**Firmware v3.59**
- **Corregido — reinicios en bucle en un DMD sin botón físico.** Una tarjeta SD copiada de un DMD *con* botón (opción activada, pulsación muy larga = reiniciar) en un DMD *sin* botón entraba en bucle de reinicios: el pin del botón, flotante, pasa a nivel bajo durante la pantalla « RecalBox conectada ». La pulsación muy larga ahora se activa al **soltar**, el estado de pulsación se borra cuando el pin está bloqueado, y un **cortacircuitos** ignora el botón durante todo el arranque que sigue a un reinicio provocado por el botón menos de 5 minutos después del arranque. Una actualización OTA ya no se cancela por esta falsa pulsación.
- **Páginas web.** Nueva imagen de inicio y la mención « © Shan_ayA 2026 » con un enlace « Invítame a un café » al pie de cada página (bajo la imagen de introducción en el inicio).
## 2026-10-20 (2) — Demonio Raspberry Pi v3.59: actualización desde la página web

**Raspberry Pi**
- **Actualización del demonio desde la página web** (página *Ajustes*): aviso cuando se publica una versión más reciente (release `pi-v…`), botón **Actualizar ahora**; descarga desde GitHub, huella SHA-256 verificada, instalación, reinicio del servicio y **vuelta automática a la versión anterior** si no arranca. `config.ini` y el contenido de la tarjeta nunca se tocan. Una Pi en 3.58 pasa una primera vez a 3.59 con el Modo 17.
## 2026-10-20 — Versión Raspberry Pi (demonio pi-v3.58)

**Raspberry Pi**
- **El DMD en Raspberry Pi 3B** (validada) con dos paneles P5: mismo protocolo, mismos scripts de Recalbox, misma tarjeta SD, mismas páginas web, widgets, temas de logos, reloj, puntuaciones, modo Pinball y modo nativo de Recalbox que el ESP32. Demonio C++ bajo **GPL-2.0+** (enlazado con rpi-rgb-led-matrix), fuentes en `raspberry-pi/`, binario y carga útil en la release `pi-v3.58`. Pi 4 / 400 / CM4 / Pi 2: `gpio_slowdown` se ajusta automáticamente según el modelo, **sin probar**. Ver [RASPBERRY-PI.es.md](RASPBERRY-PI.es.md).

**Caja de herramientas del PC build 12675**
- **Destino Raspberry Pi**: **Modo 1** todo en uno desde una tarjeta virgen, **Modo 16** (preparar la tarjeta, sustituye a Raspberry Pi Imager), **Modo 17** (instalar / actualizar). El software de la Pi se descarga de GitHub (SHA-256 verificado).
- **Modo 18 — Solo tarjetas de texto**: regenera solo las tarjetas de texto de los juegos sin imagen. Los juegos sin logo reciben una tarjeta de texto (estilo sobrio sobre fondo oscuro); los no-juegos se excluyen.
- **Conversión fiel al original**: pixel-perfect, con retorno a la escala exacta cuando el divisor entero reduce el logo más de un 3 %. Los scrapes ilegibles se reparan o se señalan.
- Perfil de Recalbox **« 11.x-10.x »** (antes « 10.x »). Aviso antes de la conversión: la descripción mostrada en el DMD está en el idioma del scrape de Recalbox. Botones **Actualizar** para la detección de tarjetas SD, panel de copia SD que ya no se recorta, botón Stop de la copia y ventana de error grave.

## 2026-10-20 — Firmware v3.58: mensajes superpuestos más grandes y del mismo tamaño, « RecalBox conectada » parpadea

**Firmware v3.58**
- **Mensajes superpuestos.** Todos los mensajes dibujados sobre la imagen (RecalBox conectada, sin conexión, sin Wi-Fi, alertas de RetroAchievements…) tienen ahora el mismo tamaño en francés, inglés y español, tan grandes como permite el panel (se reducen solo si un idioma se desbordaría).
- **« RecalBox conectada ».** El mensaje permanece 5 segundos sobre la imagen por defecto **parpadeando**, y después el logo queda solo.
- **Ajustes por defecto.** El **modo Pinball (VPX)** y las **alertas de RetroAchievements** vienen marcados por defecto, como en la versión Raspberry Pi (un `config.ini` sin la clave = activo; `feat_vpinball_dmd=0` / `feat_ra_window=0` los desactiva).
## 2026-10-18 (2) — Firmware v3.56: insignia « Actualización » en el botón Ajustes

**Firmware v3.56**
- **Insignia « ⬆ Actualización ».** Cuando se publica un firmware o un lote de scripts de Recalbox más reciente, aparece ahora una insignia naranja en el botón **Ajustes** de la página de inicio y en el enlace Ajustes del menú de cada página (en francés, inglés o español), además de la barra de avisos. La barra conserva su comprobación cada 5 minutos de la v3.55.
## 2026-10-18 — Firmware v3.55: los avisos de actualización aparecen enseguida en todas las páginas

**Firmware v3.55**
- **Corregido — sin aviso de actualización en la página de inicio.** La barra de avisos mostrada en todas las páginas (firmware, scripts de Recalbox, toolkit del PC más reciente) solo releía la versión publicada cada 6 horas: un nuevo firmware se anunciaba en la página de *Ajustes* (que comprueba cada vez) pero no en las demás. La barra vuelve a comprobar ahora cada 5 minutos.
## 2026-10-17 (2) — Firmware v3.54 y caja de herramientas del PC build 12572: los avisos de actualización ya no piden un Modo 9

**Firmware v3.54**
- El aviso de actualización del firmware (página *Ajustes*) ya no pide ejecutar un **Modo 9** tras cada actualización — los scripts de Recalbox no cambian con cada firmware. La indicación del Modo 9 solo aparece en el aviso de **scripts de Recalbox**, mostrado cuando se publica un nuevo lote de scripts. Ese aviso es ahora independiente del aviso de firmware (antes se ocultaba mientras se anunciaba un firmware más reciente).

**Caja de herramientas del PC build 12572**
- El mismo cambio en la ventana de avisos del toolkit: la frase « haz también un Modo 9 » ya no se añade a cada aviso de firmware; solo la lleva el aviso de scripts.
## 2026-10-17 — Firmware v3.53 y caja de herramientas del PC build 12472: 10 eventos de cuenta atrás, avisos de actualización con notas de versión

**Firmware v3.53**
- **Cuenta atrás: 10 eventos** (en lugar de 5), cada uno con una casilla **Activo** para ponerlo en pausa sin borrarlo, y nombres de hasta 40 caracteres: un nombre de más de 14 caracteres se desplaza ahora en el widget. Una fila incompleta (nombre sin fecha, fecha sin nombre, año fuera de 2000–2100) muestra una alerta en lugar de ignorarse en silencio.
- **Corregido — el fondo no seguía a la playlist.** Elegir una playlist podía dejar el fondo en *Solo widgets* si había otra página *Widgets* abierta en otra pestaña (borraba la elección). Una página solo escribe ahora lo que ha cambiado ella misma.
- **Corregido — la misma temperatura para todos los lugares en la vista previa del tiempo.** Los valores guardados no estaban ligados a su lugar, y la vista previa usaba el lugar guardado en vez del escrito en la página *Lugar*. Ambos corregidos.
- **Inicio y Ajustes.** Una página de inicio abierta escribiendo la dirección pone ahora el DMD en modo configuración (la petición especulativa del navegador se ignoraba y la llamada posterior usaba el método equivocado). El inicio recibe *Guardar y reiniciar* y *Reanudar DMD*; los Ajustes reciben *Reanudar DMD*.
- **Avisos de actualización.** Los avisos de firmware y scripts tienen un enlace **Notas de versión**; al pasar el ratón por encima, una burbuja muestra las últimas entradas del changelog. Las páginas web muestran también un aviso (cerrable) cuando se publica un toolkit del PC más reciente.

**Caja de herramientas del PC build 12472**
- Todos los avisos de actualización (toolkit, firmware, scripts de Recalbox, temas de citas) usan ahora una sola ventana con botones con nombre, un enlace **Notas de versión** y las últimas entradas del changelog en una burbuja al pasar el ratón. Ayuda integrada actualizada.
## 2026-10-15 — Firmware v3.52 y caja de herramientas PC build 12372: instalación nueva, widgets por defecto, lista ⇄ fondo

**Firmware v3.52**
- **Instalación nueva corregida.** Un `config.ini` ausente ahora se crea (la página AP decía « OK » y daba vueltas sin escribir nada); se muestra un error claro si la tarjeta SD falta o está protegida; la lista de redes Wi-Fi ya no sale vacía a la primera.
- **Primer arranque.** La configuración web siempre es obligatoria. Ajustes propuestos si no hay lista ni widget: **solo widgets + reloj aleatorio** (duraciones: reloj 60 s, fecha 10, efemérides 15, cuenta atrás 10, tiempo 15).
- **Regla de arranque.** Tras el primer arranque, la configuración web solo se impone si no hay nada que mostrar: *solo widgets* sin widget, o *lista de GIFs* sin lista. La IP de Recalbox ya no es obligatoria.
- **Lista ⇄ fondo.** Elegir una lista pone el fondo en *Lista de GIFs*; elegir *Solo widgets* deselecciona la lista (se recuerda); volver a lista la reactiva (el DMD se reinicia para cargarla). *Solo widgets* se rechaza si no hay ningún widget seleccionado.
- **Los submenús conservan sus ajustes.** Lugar, eventos, colores del reloj y widgets guardan los cambios sin guardar; **un solo Guardar, desde cualquier página, guarda todo**.
- La vista previa del tiempo / efemérides sin lugar propone elegir uno; pulsar un color desactiva *Auto*; la vista previa del brillo ya no cambia el ajuste guardado; la página Wi-Fi indica *MQTT/UDP*; la ventana de ayuda de la primera conexión se ha reescrito.

**Caja de herramientas PC build 12372**
- **Vistas previas al pasar el ratón** en el menú *Download from GitHub* (Modos 2, 11, 13, 15); el **Modo 15 informa** de lo copiado; el botón **Salir** ya no sale de la zona visible en la pestaña Avanzado; ayuda integrada al día.

## 2026-10-15 — Caja de herramientas PC, build 12072: temas de citas (Modo 15), varias Recalbox, nuevos nombres de archivo

- **Modo 15 — temas de citas listos para usar** (francés, inglés, español): 15 temas de 50 entradas publicados en GitHub, listados con su estado en la tarjeta SD (por instalar / al día / actualización disponible / modificado por usted), instalados a petición con verificación de la suma de control; un archivo modificado se guarda antes de ser reemplazado y no se borra nada. El **Modo 1** lo ofrece con una vista previa de cada tema; la caja de herramientas avisa al abrirse de las nuevas revisiones del paquete.
- Los Modos 1 y 9 aceptan **varias Recalbox** (hasta 4) para el cambio automático del firmware 3.x.
- Ayuda integrada actualizada (Modos 1 y 15, FR/EN/ES); las ventanas del Modo 14 y de elección de actualización eran ilegibles en tema oscuro (corregido).
- Archivos de release renombrados `RecalBoxDMD_Toolkit_<build>_Setup.exe` / `_Portable.exe` / `_Source.zip`; los binarios del firmware se publican sin versión en el nombre (`binaries/RecalBoxDMD_app.bin`, `RecalBoxDMD_merged.bin`).

## 2026-10-15 — Firmware v3.50: widgets, ambientes, nuevos relojes, varias Recalbox, página Ajustes, botón físico

**Añadido**
- **Widgets** (página *Widgets*): fecha, efemérides, cuenta atrás, tiempo, ambiente (8 estilos), citas y quiz, y tres widgets Recalbox (desafío, recompensas, salud), mezclados con la lista de GIF — *desactivado / en lugar del reloj / además*, disparador cada N GIF o N minutos, o **solo widgets** (ningún GIF). Un botón **Vista previa** reproduce cualquier widget al instante.
- **Cielo** que sigue el amanecer/atardecer real de tu lugar y el tiempo en directo tras los widgets fecha, tiempo, efemérides y cuenta atrás.
- **5 nuevos relojes animados**: Plasma, Life, Stars, Rain, Lava.
- **Citas y quiz**: 4 formatos de línea incluido un quiz de 3 opciones (cuenta atrás, la respuesta correcta se queda en pantalla), crear / editar / importar / borrar temas desde la web, fondos ilustrados (integrados en el firmware, escritos en la SD si faltan).
- **Widgets Recalbox** alimentados por `dmd_rbwidgets` (script de Recalbox, UDP `CMD=rbw` cada 2 min), cada uno con un fondo ilustrado; recompensas 8 s por página, duraciones por defecto 15 / **30** / 15 s.
- **Varias Recalbox, un solo DMD**: hasta 4 IP (`recalbox_ips`, `recalbox_target`), cambio automático a la primera que responde, paquetes de la otra Recalbox de la lista ignorados; los Modos 1 y 9 de la caja de herramientas aceptan varias IP.
- **Botón físico** (GPIO 21): pulsación corta / doble / larga / muy larga, cada una ligada a una acción desde la página *Botón*.
- **Página ⚙ Ajustes** (justo después de *Inicio* en el menú): brillo, arranque silencioso, modo Pinball, alertas RetroAchievements, modo Recalbox nativo y versión del firmware con su botón *Actualizar* (OTA), sacados de la página de inicio — que pasa a ser un **menú sin más**.
- **Banner de actualización en la parte superior de cada página**: un firmware más reciente (enlace a *Ajustes*) y / o un **lote de scripts de Recalbox** más reciente (enlace a la caja de herramientas de PC, que los instala: Modo 9) se anuncian en cualquier página que abras. Lo comprueba tu navegador — el DMD no necesita acceso a Internet — como máximo cada 6 horas.
- **Quiz**: una respuesta larga pasa ahora a **2 líneas** en lugar de cortarse.
- **Temas de citas listos para usar** (**Modo 15** de la caja de herramientas): 15 temas de 50 entradas — citas célebres, galleta de la fortuna, chistes, adivinanzas, quiz de retrogaming — en **francés, inglés y español**, publicados en GitHub (`widgets/quotes/`). El Modo 15 los lista con su estado en la tarjeta SD (por instalar / al día / actualización disponible / modificado por usted), instala los que marque (suma de control verificada, un archivo que usted haya editado se guarda antes de ser reemplazado, nunca se borra nada), y la caja de herramientas avisa al abrirse cuando se publica una nueva revisión. El **Modo 1** también lo ofrece, con una vista previa de cada tema.

**Cambiado**
- Las páginas web se envían con un retraso sin progreso acotado (2 s) en lugar de congelar el bucle principal hasta 19 s con un cliente lento.
- Se eliminó el widget *Estadísticas*.
- Las páginas web comparten su estilo, la ayuda, las traducciones y una cola de peticiones mediante dos archivos en caché (`/cfg.css`, `/cfg.js`): páginas entre un 20 y un 60 % más ligeras y el navegador pide al DMD **una petición cada vez**.
- Una página web pedida mientras corre un widget o el reloj **los detiene primero y libera su memoria, y luego se sirve** — el modo web config descarga lo que puede antes de abrir una página.
- Las listas de carpetas y de playlists cargan **unas 16 veces más rápido** (nombres leídos directamente del directorio de la SD).
- Se eliminó el atajo *Continuar* de la página de inicio.

**Corregido**
- **Fiabilidad de `config.ini`**: escrituras atómicas con reparación al arrancar; guardados verificados antes de responder *OK*; campos Wi-Fi / lista vacíos rechazados; ajustes Wi-Fi aplicados solo por una página *Wi-Fi* cargada (borradores caducados del navegador podían apagar el Wi-Fi y la IP fija); **el modo solo widgets, el disparador, el orden y el número de extras nunca se escribían** en `config.ini` y volvían a los valores por defecto en cada reinicio.
- Widgets con cielo omitidos («cielo imposible (heap)») tras un guardado: la asignación reintenta con bandas más finas.
- **«No se pudo cargar la configuración» y guardados que no terminaban**: la memoria estática había superado un umbral (≈ 90 KB) con los ambientes líquido / arena, tras lo cual el servidor web ya no podía enviar. Los búferes de los widgets solo se reservan mientras corre un widget (memoria estática 98 → 88 KB) y se liberan al abrir la config web.
- **Congelación del Wi-Fi ante una ráfaga de cargas de página** (se servía una página desde dentro de un widget en curso con muy poca memoria libre).
- La pila de la tarea principal pasa a 12 KB (8 KB se desbordaban durante los widgets), y las conexiones inactivas del navegador se descartan a los 0,6 s en lugar de 5 s.

## 2026-10-12 — Firmware v2.64: elección de los temas de reloj del sorteo aleatorio

- **Reloj: temas admitidos en el sorteo.** Cuando el tema es *Aleatorio*, la página Reloj muestra 10 casillas (una por tema): el reloj solo sortea entre los temas marcados (al menos uno). Un tema fijo no cambia; por defecto se admiten todos.
- **Nunca el mismo tema dos veces seguidas.** El sorteo recuerda ahora el último tema de una aparición del reloj a la siguiente (la regla no se aplicaba entre dos apariciones sucesivas).

## 2026-10-11 (2) — Firmware v2.61: modo Recalbox nativo, Visual Pinball por ZeDMD WiFi (lote de scripts 5)

- **Modo Recalbox nativo** (nueva casilla en la página web del panel, sección propia bajo *Pantalla*). Recalbox 11 sabe manejar un DMD por sí mismo: marca la casilla y el panel se comporta como un **ZeDMD WiFi** — Recalbox decide todo lo que se muestra (marquee del sistema y del juego, mesas de Visual Pinball y, en la alfa Patreon de Recalbox 11, hi-scores y RetroAchievements). Desmárcala: todas las funciones del firmware vuelven exactamente como las habías configurado (playlist de espera, reloj, fichas de información, RB Challenge, comandos de scripts). Indica la dirección del DMD en los ajustes DMD de Recalbox (*ZeDMD WiFi*).
- **Visual Pinball en el DMD en modo nativo.** La configuración de Visual Pinball de Recalbox no pasa la dirección del ZeDMD WiFi al emulador; el script `dmd_vpx_config` (v3, **lote de scripts 5 — ejecuta una vez el Modo 9 de la caja de herramientas**) copia ahora el ajuste `dmd.zedmd.wifi` de Recalbox cuando se elige *external DMD: zedmd* en las opciones de Visual Pinball de Recalbox. Medido: unas 100 imágenes por segundo desde una mesa.
- **Última imagen conservada.** En modo nativo el panel vuelve a mostrar al reiniciar la última imagen vista durante la navegación (no se guarda durante una partida).
- **Correcciones encontradas probando en hardware:** la recepción del flujo ZeDMD podía congelarse (sustituida por un socket normal: el paso marquee ↔ mesa es inmediato); el panel ya no entra en modo configuración cuando el navegador solo *precarga* la dirección que escribes; la página Wi-Fi rellena la IP fija, la puerta de enlace, la máscara y los DNS desde la conexión actual y rechaza una IP fija incompleta (provocaba *IP CFG ERR* al reiniciar); la página web vuelve a mostrar su mensaje en el panel y restaura la imagen después.
- También en esta versión: la opción experimental *Alertas RetroAchievements de ES* (modo normal, desactivada por defecto).

## 2026-10-11 — Caja de herramientas PC, build 11469: compatibilidad con Recalbox 11, logos más nítidos y fieles

- **Logos reducidos como lo hace Recalbox 11 en su propio DMD (render 21).** Recalbox 11 puede manejar un DMD de forma nativa: EmulationStation rasteriza cada logo directamente al tamaño de la pantalla, con un anti-aliasing exacto. La caja de herramientas reduce ahora los logos igual (área exacta) en lugar de forzar cada LED a todo o nada: las letras ya no se deforman, los textos pequeños y los detalles conservan sus niveles de gris. Se reconstruyeron todo el paquete de logos (11 temas) y las imágenes por defecto.
- **Contornos oscuros que siguen oscuros.** En *Multi-game compilation*, *Board game*, *Rhythm & music*, *Platformer* y *Platform shooter* el contorno es azul oscuro, violeta o marrón, no negro; se aclaraba hasta parecerse a las letras. Ahora queda casi negro y las letras destacan.
- **Tres logos que faltaban.** *recalboxtv*, *online* (Recalbox Online) y *wheel* (volante), presentes en la Recalbox 11.0-patron-1.1 pero no en el archivo de la 11.0-patron-1, forman ahora parte de la caja de herramientas y del paquete (también como imágenes por defecto).

## 2026-10-11 — Caja de herramientas PC, build 11369: texto fino legible, logos de Recalbox 11, alternativa para GitLab

- **Los trazos finos ya no se cortan en rayitas (render 19).** Los trazos más finos que media LED (texto fino como *TECHNOLOGIES*, *MULTIPLE ARCADE MACHINE EMULATOR*, *PlayStation Portable*, contornos como la elipse de PC Engine) se engrosan un poco antes de la reducción. Medido en 311 logos: **88 LED aisladas encendidas de más antes, 16 ahora**. Los trazos gruesos y las superficies no se tocan.
- **Logos de Recalbox 11.0-patron-1.** Los logos integrados en Recalbox vienen ahora de la 11.0-patron-1: **32 sistemas nuevos** (Sega Model 1 / 2, Namco System 22, Hyper Neo Geo 64, Jaguar CD, N-Gage, Game.com, Archimedes, FM Towns…), logos existentes sin cambios. Las imágenes por defecto los incluyen.
- **Alternativa para GitLab.** Desde el 10/10 GitLab responde a la petición de versión de los temas propios de Recalbox con una página anti-robot (HTTP 403); la caja de herramientas lee ahora la versión de la rama principal, así que *recalbox-next* y *recalbox-240p* siguen apareciendo en el Modo 12 incluso sin caché local.

## 2026-10-09 — Caja de herramientas PC, build 11269: copia a la tarjeta SD más segura, columna *Idiomas | zonas* más clara

- **Copia a la tarjeta SD con reanudación.** En el Modo 12, cada tema se **prepara primero en la carpeta de trabajo de la caja de herramientas y luego se copia a la tarjeta SD**. Si la copia se interrumpe (tarjeta retirada, error de escritura, fallo), no se pierde nada: el botón **Copiar a la SD (n)** — desactivado cuando no hay nada pendiente — continúa donde se detuvo, y la tarjeta nunca muestra un tema a medio escribir como actualizado (el marcador de versión se escribe el último y el anterior se retira antes). Los temas preparados en una sesión anterior se listan al abrir la ventana.
- **Columna Idiomas | zonas** (antes *Variantes*): lista **todos** los idiomas y zonas que ofrece un tema, base inglés / US incluida (por ejemplo *en, fr | us, eu, jp*).
- Docs: la página de inicio y el README presentan ahora las **actualizaciones del firmware por Wi-Fi** (sin cable USB desde el firmware 2.44: página web de configuración, teléfono incluido, y Modo 14 de la caja de herramientas).
- **Correcciones de logos (render 16)**: **Data East Classics** faltaba en las imágenes por defecto (existe con un nombre con espacio duro, el que envía Recalbox) — añadido; el contorno oscuro de los géneros cargados (*Platformer*, *Platform Shooter*, *Board Game*, *Multi-game Compilation*) es ahora casi negro para que las letras destaquen; *SNK Classics* ya no tiene un punto blanco en la A; *Atari ST* pierde la marca ® que aparecía como un cuadrado luminoso; **Philips Videopac G7000** — la variante europea de la Odyssey 2 se encuentra ahora (el tema la escribía con otro nombre que el que envía Recalbox), y los demás logos de región integrados en Recalbox (por ejemplo *Genesis* para EE. UU.) también se instalan.
- **Logos más nítidos (render 18)**: la reducción a 128×32 se rehízo. Medido en 74 logos, **el 34 % de los LED de borde estaban medio encendidos** (trazo borroso) con el método anterior; ahora son el **10 %**, sin cambiar la cantidad de materia encendida (1,02 × el promedio simple) y **conservando los trazos muy finos** en lugar de perderlos. Cada LED toma el color de la materia encendida de su bloque, y solo los LED de borde de una forma sólida se cortan (encendido o apagado). Además, convertir un tema es unas **10 veces más rápido** (un tema entero en segundos) y el símbolo de Atari conserva sus brazos finos. Las imágenes por defecto y el paquete de 11 temas se regeneraron.

## 2026-10-08 — Caja de herramientas PC, builds 10768 → 11169: logos del DMD más cercanos a lo que muestra Recalbox, Modo 12 más rápido

- **Temas propios de Recalbox** (*recalbox-next*, el tema por defecto, y *recalbox-240p*): la caja de herramientas también convierte los logos **integrados en Recalbox** (géneros, Favoritos, Último jugado, Ports…) con sus variantes en francés. Los logos de género siguen el idioma de tu Recalbox (*Multijoueur* en lugar de *Multiplayer*) y los logos propios de una región (Super Nintendo EU / JP / US) siguen tu región. Ambos temas están en el paquete de GitHub, que ahora contiene **11 temas**.
- **Logos más nítidos y legibles**: reducción «pixel art» (Kopf et al. 2013) en lugar de un simple promedio; «240p TEST SUITE» legible; variantes del Theme Studio (`<path.fr>`, `<path.EU>`) detectadas. Una **placa** oscura detrás de texto blanco sigue negra (Dragon 32, Lynx, *CLASSICS* de Midway / Sammy, VIC-20, Textual Adventure, el búho de BBC Micro); un **contorno** oscuro sigue siendo un gris oscuro visible en lugar de un halo blanco; vuelve el **degradado de Dashboard-X**; los azules puros siguen azules (Vectrex, lutro, SAM Coupé); retoques para EXL100, Mitchell y ScummVM.
- **Como el renderizado cambió (render 15), los temas ya convertidos aparecen *por actualizar* en el Modo 12** — basta un *Actualizar los obsoletos*.
- **Modo 12 (build 11169)**: un tema cuya versión es la del paquete de GitHub se actualiza **descargando los logos ya preparados** (segundos) en lugar de convertirlo (minutos); solo convierte desde tu Recalbox si la versión difiere, si el tema no está en el paquete o si sus logos se modificaron. Una **barra de progreso** muestra el avance, cada tema pasa a *AL DÍA* en cuanto termina, y el mensaje de verificación de la tarjeta SD indica que los temas instalados en la Recalbox se comparan con la Recalbox (la referencia).
- **Modos 12 / 13**: el tema activo de tu Recalbox se ofrece primero; una sola pregunta a la vez; verificación más rápida. El **Modo 9** propone reiniciar la Recalbox tras instalar los scripts; se corrigió la falsa alerta *scripts por actualizar* (`dmd_udp_resync` v13 — reinstala los scripts con el Modo 9).
- Detalle del comportamiento del Modo 12: pestaña Ayuda (*Temas de Recalbox*).

## 2026-10-05 (2) — Firmware v2.47: nueva opción «Idioma prioritario» para el seguimiento del tema

- **Novedad**: página de configuración web, sección «Tema de Recalbox»: una nueva casilla **Idioma prioritario (tema sin variante)**, **desmarcada por defecto** (no cambia nada). Marcada, si el tema no tiene variante para el idioma de Recalbox (por ejemplo Midnight en español), el DMD prefiere la imagen por defecto traducida (francés o español, instalada por el Modo 2 de la caja de herramientas) al logo base del tema, a menudo en inglés. El DMD puede entonces diferir de la pantalla de Recalbox. Una variante de idioma del propio tema siempre tiene prioridad. Se aplica en el próximo cambio de sistema.
- Actualización desde la página de configuración web (botón **Actualizar ahora (Wi-Fi)**) o la caja de herramientas PC (**Modo 14**). No hay que reinstalar nada en la tarjeta SD.

## 2026-10-05 (1) — Firmware v2.46: el DMD muestra lo que muestra la Recalbox

- **Cambio**: el orden de elección de los logos vuelve a ser el de la pantalla de la Recalbox. Para un sistema, el DMD usa, en este orden: el logo del tema para su **idioma**, luego para su **región**, luego el **logo base** del tema. La imagen por defecto traducida (francés, español) solo se usa si el tema **no tiene ningún logo** para ese sistema. Esto revierte la v2.45, que ponía la imagen por defecto traducida antes de los logos base del tema y podía hacer que el DMD fuera distinto de la pantalla de la Recalbox.
- Consecuencia: con un tema sin variante para su idioma, el DMD muestra el mismo logo (a menudo en inglés) que la pantalla de la Recalbox. Los autores de temas pueden añadir variantes de idioma (`path.fr`, `path.es`, `path.en`) y ambas pantallas las seguirán.
- Actualización desde la página de configuración web (botón **Actualizar ahora (Wi-Fi)**) o la caja de herramientas PC (**Modo 14**). No hay que reinstalar nada en la tarjeta SD. Las imágenes por defecto de `systems/_defaults/es` y `fr` también se han rehecho (logos nuevos, sin errores de texto): ejecute el **Modo 2** para actualizarlas.

## 2026-10-04 (9) — Firmware v2.45: las imágenes traducidas de Favoritos / Último jugado / géneros ganan a las del tema, en inglés

- **Corrección**: con un idioma de Recalbox que tiene imágenes por defecto traducidas (francés, español) y un tema que aporta sus propios logos de *Favoritos*, *Último jugado* o de géneros **sin variante para ese idioma** (por ejemplo Midnight en español: solo existe una variante francesa), el DMD mostraba el logo en inglés del tema aunque la imagen traducida estuviera en la tarjeta SD. Ahora se usa la imagen traducida; las consolas conservan el logo del tema, y una variante de idioma del propio tema sigue teniendo prioridad (Midnight en francés no cambia para Favoritos y Último jugado).
- Actualización desde la página de configuración web (botón **Actualizar ahora (Wi-Fi)**) o la caja de herramientas PC (**Modo 14**). No hay que reinstalar nada en la tarjeta SD.

## 2026-10-04 (8) — Firmware v2.44 y caja de herramientas PC build 10567: actualización del DMD por Wi-Fi

- **Nuevo**: el DMD se actualiza ahora solo desde GitHub. El aviso « nueva versión disponible » de su página web incluye un botón **Actualizar ahora (Wi-Fi)**, y la caja de herramientas PC un nuevo **Modo 14 — Firmware del DMD (Wi-Fi)**, que encuentra el DMD, compara versiones e inicia la actualización. El DMD descarga él mismo el firmware (unos 2 minutos, pantalla apagada mientras tanto, varios reinicios), comprueba su tamaño y su SHA-256 antes de instalarlo y vuelve solo al firmware anterior si la descarga falla o si el nuevo no arranca.
- **Hace falta una reinstalación por USB para pasar a la 2.44** (Web Installer): ha cambiado el reparto de la memoria flash (dos ranuras de firmware en lugar de una). Su `config.ini` y la tarjeta SD no se tocan; después, sin cable. Véase [Actualización](UPGRADING.es.md). Compilación propia: esquema de particiones **Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)**.
- **Eliminado**: el puerto serie Bluetooth clásico (desactivado por defecto, ~660 KB de flash y ~40 KB de RAM). El DMD dispone ahora de unos 58–72 KB de memoria libre en uso normal en lugar de 12–24 KB.

## 2026-10-04 (7) — Firmware v2.43: los logos de tema de Recalbox también se aplican a los sistemas de género

- **Corrección**: en Recalbox 10.1, los sistemas de género (Deportes, Estrategia, Plataformas de acción…) y sistemas virtuales como *Últimos jugados* conservaban las imágenes por defecto del DMD aunque hubiera un tema seleccionado. EmulationStation los anuncia como `genre-<nombre>` pero busca su logo de tema como `auto-<nombre>`. Ahora el DMD prueba `auto-<nombre>` cuando el tema no tiene logo con el nombre anunciado (incluidas las variantes de idioma y región). No hay que reinstalar nada: los paquetes de temas ya contienen estos logos (Midnight incluye, por ejemplo, `auto-sports`, `auto-strategy`…).
- Actualice desde la página de configuración web (aviso «nueva versión disponible») o el instalador web.

## 2026-10-04 (6) — Firmware v2.42, script v11: la comprobación de actualizaciones cubre también los scripts de Recalbox

- **Novedad**: la página web del DMD y la caja de herramientas del PC (build `10466`, al arrancar) avisan también cuando los **scripts de Recalbox** no están al día, e indican qué hacer: **ejecute la caja de herramientas del PC y haga un Modo 9, luego reinicie la Recalbox**. El aviso de nuevo firmware también lo recuerda.
- Cómo: los scripts (`dmd_udp_resync` v11, reinstálelos con el **Modo 9**) anuncian su número al DMD, que lo muestra en su página web y se lo da a la caja de herramientas por Wi-Fi. No se lee nada en la propia Recalbox. Los scripts anteriores a la v11 no anuncian nada: el DMD los reconoce cuando la Recalbox lleva encendida un minuto y medio.
- Actualice el firmware desde la página de configuración web o el instalador web y haga un **Modo 9** una vez.

## 2026-10-04 (5) — Firmware v2.41 y script v10: «Reanudar DMD» muestra el estado actual de la Recalbox

- **Corrección** (script `dmd_udp_resync` v10 — reinstálelo con el **Modo 9** y reinicie la Recalbox): tras **Reanudar DMD**, el DMD podía quedarse con la pantalla vacía hasta que se moviera por la Recalbox. Cuando la Recalbox mostraba un **sistema** (no un juego), respondía «playlist». Ahora envía el sistema actual y su logo aparece enseguida.
- **Firmware** (v2.41): cerrar o salir de la configuración web ya no fuerza la pantalla de configuración una vez reanudado el DMD; una vista previa del reloj lanzada tras reanudar ya no se queda congelada en pantalla.
- Actualice desde la página de configuración web (aviso «nueva versión disponible») o el instalador web.

## 2026-10-04 (4) — Firmware v2.40: la configuración web conserva sus cambios entre páginas

- **Corrección**: en la configuración web del DMD, un ajuste cambiado en una página (por ejemplo el **modo Pinball** en la página de inicio) se perdía si pulsaba Guardar en otra página. Ahora los cambios hechos en cualquier página se conservan mientras navega, y **un solo Guardar / Guardar y reiniciar (en cualquier página) los guarda todos**.
- Al salir con **Reanudar DMD** o **Reiniciar** con cambios sin guardar, un cuadro de diálogo se lo indica y ofrece **Guardar** / **Salir de todos modos** (o Cancelar). Nada más le interrumpe: moverse entre páginas sigue siendo libre.
- Actualice desde la página de configuración web (aviso «nueva versión disponible») o el instalador web.

## 2026-10-04 (3) — Firmware v2.39 y script v9: el cambio de tema en Recalbox llega al DMD

- **Corrección** (script `dmd_udp_resync` v9 — reinstálelo con el **Modo 9** y reinicie la Recalbox): al cambiar de tema en Recalbox, el DMD podía quedarse con los logos del tema anterior. Los dos mensajes (idioma/región y luego tema) se enviaban en el mismo milisegundo y el DMD solo conservaba uno.
- **Firmware** (v2.39): los mensajes de tema e idioma/región se aplican en cuanto llegan (ya no se puede perder ninguno); se **memorizan el último tema, idioma y región**, así que tras un arranque en frío el DMD muestra enseguida los logos correctos en lugar de los predeterminados; el logo en pantalla se **redibuja de inmediato** al cambiar de tema, idioma o región.
- Actualice el firmware del DMD desde la página de configuración web (aviso «nueva versión disponible») o el instalador web.

## 2026-10-04 (2) — Caja de herramientas PC build 10365: logos de consolas por defecto (US) corregidos

- **Corrección**: con las variantes de idioma / región, los **logos de base** de un tema se tomaban de la ruta *neutra* del tema en lugar de su variante **US**. En los temas donde ambas difieren (por ejemplo **Midnight**: el logo neutro de la Super Nintendo es el *Super Famicom* japonés), el DMD mostraba el logo japonés para la región US por defecto. Corregido en el paquete (**10 temas reconstruidos**, cambian unos pocos logos de consolas por tema) y en las conversiones propias de la caja de herramientas (Modo 12).
- **Para tenerlo**: reinstala / actualiza tus temas una vez más (el Modo 12 los marca como desactualizados; o Modo 1 / 13).
- **Recordatorio**: tras el **Modo 9** (scripts de Recalbox), **reinicia la Recalbox** — los scripts antiguos siguen ejecutándose en memoria hasta entonces, así que el DMD no seguiría el idioma y la región de la Recalbox.

## 2026-10-04 — Firmware v2.38 y caja de herramientas PC build 10364: el idioma y la región siguen a tu Recalbox, en directo

- **Firmware** (v2.38) + **script de Recalbox `dmd_udp_resync` v8** (reinstálalo con el **Modo 9**): el DMD usa ahora el **idioma** y la **región del tema** de tu Recalbox. La **región** elige los logos de las consolas (*Super Famicom* / *Super Nintendo*, *Mega Drive* / *Genesis*…), el **idioma** elige los textos traducidos (*Favoritos* / *Favorites*…). Cambia uno u otro en Recalbox y el DMD lo sigue en pocos segundos — sin reinstalar, sin reiniciar. También funciona para las imágenes por defecto (inglés, francés, español) cuando no hay tema activo.
- **Caja de herramientas PC** (build `10364`): **nada que elegir** — la pregunta de idioma de los Modos 1, 2 y 13 y las listas de idioma / región del Modo 12 desaparecen. La caja de herramientas instala la base inglés / US más **todas las variantes** (temas: subcarpetas `l_<idioma>/` y `r_<región>/`; imágenes por defecto: `_defaults/fr/` y `_defaults/es/`), unos cientos de KB más por tema.
- **Paquete de temas** reconstruido en el nuevo formato (10 temas). **Hace falta reinstalar una vez** para obtener las variantes: Modo 1, 12 o 13 para los temas, Modo 1 o 2 para las imágenes por defecto. Hasta entonces todo sigue funcionando con los logos en inglés / US. Las cajas de herramientas antiguas solo instalan los logos US de base.
- Motivo: la región del tema es un ajuste propio de Recalbox (US por defecto, independiente del idioma), mientras que el paquete antiguo ataba el idioma a la región — nueve logos de consolas de `recalbox-next` diferían de lo que mostraba Recalbox.

## 2026-10-03 (7) — Caja de herramientas PC build 10263: panel de copia a la SD corregido tras el Modo 13, columna SD en el Modo 12

- **Corrección**: tras el **Modo 13** (y los Modos 2 y 11) lanzado desde la pestaña Avanzado, el **panel «copiar a la tarjeta SD»** se ocultaba justo al terminar la descarga. Ahora permanece visible, para copiar los archivos a la tarjeta SD enseguida.
- **Modo 12** (Gestión de temas de Recalbox): una nueva columna **SD**, junto a *Recalbox*, indica si cada tema ya está en la tarjeta SD del DMD (✓ presente, — ausente, ? si no se detecta la tarjeta).

## 2026-10-03 (6) — El tema por defecto de Recalbox (recalbox-next) ya funciona: caja de herramientas PC build 10163 + script de Recalbox v7

- **Script de Recalbox `dmd_udp_resync.py` v7** (reinstálalo con el **Modo 9**): si nunca cambiaste de tema en Recalbox, el script anuncia ahora el tema por defecto de Recalbox, **`recalbox-next`**. Antes anunciaba «ningún tema» en ese caso, así que la mayoría de los usuarios nunca veía los logos de tema.
- **Paquete de logos**: **`recalbox-next`** (tema por defecto) y **`recalbox-240p`** se unen al paquete, que cuenta ahora **10 temas**. Se redistribuyen con el permiso de Recalbox (véase `NOTICE.txt` en el paquete). Instálalos con los Modos 1, 12 o 13 (aparecen en la lista; el tema por defecto aparece marcado de antemano cuando es el de tu Recalbox).
- **Caja de herramientas PC** (build `10163`): también puede descargar y convertir directamente de GitLab los temas que vienen con Recalbox cuando no están en el paquete (solución de reserva para futuros temas); la pestaña Ayuda lo explica.
- Nada que flashear: el firmware no cambia (v2.37).

## 2026-10-03 (5) — Caja de herramientas PC build 10062: el aviso de actualización compara con el firmware flasheado en tu DMD

- **Caja de herramientas PC** (build `10062`): unos segundos después de abrirse, lee la **versión del firmware de tu DMD por Wi-Fi** (encuentra el DMD en tu red sola — sin USB) y te avisa si hay un firmware más reciente publicado, una vez por versión publicada. El DMD debe tener el firmware **2.37 o posterior** para indicar su versión; uno más antiguo se reconoce como «anterior a la v2.37». DMD apagado o en otra red: el aviso sigue siendo informativo, como antes. Es cuidadosa con el DMD (una sola petición al arrancar, se recuerda su dirección). Para desactivar las comprobaciones: `"update_check": false` en `RecalBoxDMD_prefs.json`.
- **Sitio web**: una etiqueta verde **«Novedad»** marca ahora la última funcionalidad en la página de inicio (temas de Recalbox).

## 2026-10-03 (4) — Firmware v2.37 y caja de herramientas PC build 9961: te avisan cuando sale una nueva versión

- **Firmware** (v2.37): la página de inicio de la configuración web muestra ahora la **versión del firmware** y, cuando se publica una más reciente, un aviso **«Nueva versión disponible»** con un enlace al Web Installer. La comprobación la hace **tu navegador** (el DMD no necesita acceso a Internet ni usa memoria adicional); sin conexión, no se muestra nada.
- **Caja de herramientas PC** (build `9961`): unos segundos después de abrirse, consulta la última caja de herramientas y el último firmware publicados. Si existe una caja de herramientas más reciente, una ventana ofrece abrir la página de descarga (una sola vez por versión). El aviso del firmware es solo informativo: la caja de herramientas no puede saber qué versión está flasheada en tu DMD, mira la versión en la página web del DMD. Para desactivar la comprobación: `"update_check": false` en `RecalBoxDMD_prefs.json`.
- **Web Installer**: la página muestra ahora la versión que va a instalar, y ese número se actualiza automáticamente a partir del propio archivo del firmware (el firmware lleva su propio número de versión).
- **Mantente informado**: en GitHub, *Watch → Custom → Releases* (o el feed de releases).

## 2026-10-03 (3) — Caja de herramientas PC build 9860: elegir los temas de Recalbox que instalar

- **Caja de herramientas PC** (build `9860`): tú decides qué temas de Recalbox van al DMD.
  - El **Modo 1** ahora pregunta si se activa el seguimiento de los temas de Recalbox (la nueva opción del firmware 2.35), y luego lista los temas instalados en tu Recalbox junto a los disponibles en GitHub, con casillas para marcar: **solo se copian los temas marcados**. Un tema ya **actualizado en la tarjeta SD** no se descarga ni se reescribe. ¿Recalbox inaccesible? Obtienes la lista completa de temas disponibles.
  - **Nuevo Modo 13 — Temas Recalbox** (categoría *DOWNLOAD FROM GITHUB*): descarga en la carpeta de trabajo los temas que elijas, con las mismas preguntas que el Modo 1.
  - **Nuevo Modo 12 — Gestión de temas** (categoría propia, se lanza con INICIAR): compara GitHub, los temas instalados en tu Recalbox y la **tarjeta SD del DMD**, señala los temas obsoletos o nunca convertidos, los actualiza y convierte los que GitHub no ofrece. La lista se rellena sola al abrir, con un botón *Actualizar*.
  - El panel de copia a la tarjeta SD ahora preselecciona la unidad llamada **RECALBOXDMD**; su botón *Iniciar la copia* sigue visible tras el Modo 13; el menú Avanzado es más compacto (el botón Salir quedaba cortado). La pestaña Ayuda documenta los nuevos modos.
- **Para tener toda la función**: firmware **2.35** (casilla para activar o no los logos de temas) + esta caja de herramientas. El paquete de 8 temas se mantiene al día desde el hub de temas de Recalbox mediante una comprobación semanal automática en GitHub.

## 2026-10-03 (2) — Firmware v2.35: una casilla para los logos de los temas de Recalbox

- **Firmware** (v2.34 y v2.35): nueva sección **«Tema de Recalbox»** en la página web (*Pantalla y listas*) con una casilla **«Seguir el tema de Recalbox»**, marcada por defecto (mismo comportamiento que en la v2.33). Desmarcada, el DMD muestra siempre los logos por defecto. El tema anunciado por Recalbox se recuerda: volver a marcar la casilla lo aplica al instante. El ajuste se guarda en `config.ini` (`feat_theme_follow=`).
- **Corrección**: cuando el tema cambia **mientras se reproduce una animación**, ahora se carga el índice de logos del tema (`_index.bin`) (antes se omitía por falta de un bloque de memoria libre suficientemente grande, y un logo ausente del tema tardaba cerca de 1 segundo en volver al logo por defecto).
- **Nada que cambiar en la Recalbox**: el `dmd_udp_resync.py` v6 de la v2.33 sigue siendo el correcto. La instalación de los paquetes de temas desde la caja de herramientas de PC llegará con su próxima versión.

## 2026-10-03 — Firmware v2.33: logos de los temas de Recalbox en el DMD

- **Firmware** (v2.33): el DMD ahora puede mostrar los **logos de sistema del tema seleccionado en Recalbox** (Midnight, Recalbox Next, Dashboard-X...) en lugar de los logos por defecto. Los scripts de Recalbox envían el nombre del tema; el firmware busca `/systems/_defaults/_themes/<tema>/<sistema>.raw565` en la tarjeta SD y, si el logo no está, vuelve al logo por defecto. Un pequeño archivo `_index.bin` por tema evita búsquedas lentas de logos ausentes. Sin carpeta de tema en la SD = no cambia nada.
- **Scripts de Recalbox**: `dmd_udp_resync.py` v6 (en `tools/recalbox_scripts/dmd_helpers/`) envía el tema al DMD al arrancar, al cambiar de tema y tras cada reinicio del DMD. **Actualice este script en su Recalbox** para que la función funcione.
- **Tarjeta SD**: los logos por defecto de `carte SD/systems/_defaults/` se regeneraron a partir de los logos vectoriales (SVG) de Recalbox, y hay un **paquete de logos de temas** (8 temas, variantes inglés/francés/español) en `carte SD/systems/_defaults/_themes/`: copie los temas que use en el mismo lugar de su tarjeta SD. La instalación de este paquete por el toolkit (Modo 1) y la ventana de actualización llegarán con la próxima versión del PC Toolkit.

## 2026-09-30 — Herramienta PC: tarjeta SD reconocida por Windows pero ausente de la herramienta

- **Herramienta PC** (build `8056`): corregida la detección de la tarjeta SD (Modos 1, 6 y 8). Algunas tarjetas que Windows ve como una unidad extraíble FAT32 normal **faltaban en la lista sin ningún mensaje**: la herramienta pedía las unidades a PowerShell, y cualquier fallo de lectura (carácter con acento en el nombre del volumen, arranque lento de PowerShell, error de WMI) daba una lista vacía en silencio. Ahora la salida se lee en UTF-8, la espera es mayor y, si PowerShell sigue fallando, la herramienta **consulta directamente a Windows** (llamada Win32 nativa) en lugar de rendirse.
- **Si tu tarjeta sigue sin aparecer**: pulsa *Actualizar* en la ventana de unidades e indica la letra, el sistema de archivos y el tamaño que muestra Windows.

## 2026-09-27 — Firmware v2.32: se acabaron los reinicios en bucle al arrancar con una playlist muy larga

- **Firmware** (v2.32): corregido un **reinicio en bucle nada más arrancar** cuando la playlist elegida es muy larga (varias decenas de miles de líneas). La reconstrucción de la caché de la playlist ocupaba el procesador el tiempo suficiente para disparar el watchdog del ESP32, que reiniciaba el DMD antes de guardar la caché — y cada reinicio volvía a empezar. Ahora la reconstrucción deja respirar al sistema con regularidad: probado con una playlist de 23 842 líneas, la caché se construye una vez (unos cincuenta segundos, con el reloj de arena en pantalla) y se reutiliza al instante en los arranques siguientes.
- **Si su DMD está atascado en este bucle**: flashee la v2.32 con el Web Installer o, mientras tanto, ponga la tarjeta SD en un PC y haga que `playlist=` en `config.ini` apunte a una playlist más corta.

## 2026-09-26 (2) — Scripts de Recalbox: dos copias del mismo script podían ejecutarse a la vez

- **Scripts de Recalbox** (`dmd_helpers/singleton_lock.sh`, compartido por `marquee`, `dmd_score` y `dmd_achievement`): corregida una carrera poco frecuente en el bloqueo «una sola copia a la vez». Durante una ráfaga de eventos de EmulationStation (los scripts se relanzan en cada evento), una copia nueva podía tomar el bloqueo de una copia que acababa de arrancar, y **ambas seguían ejecutándose** — mensajes duplicados o desordenados en el DMD. Medido en una Recalbox Raspberry Pi muy cargada: 1 ráfaga de 40 antes, 0 después. La comprobación también es más ligera (ya no se lanza ningún proceso en cada relanzamiento de EmulationStation).
- **Para actualizar**: ejecute el Modo 9 del Toolkit PC (instala los scripts desde GitHub) y reinicie la Recalbox.

## 2026-09-26 — Toolkit PC: imágenes animadas de juegos de vuelta en la navegación rápida, Modo 4 en una carpeta plana

- **Toolkit PC** (build `8055`): **corregida una regresión del build 8053** — la caché de sistemas marcaba todos los sistemas como «imagen fija», así que el DMD ya no probaba nunca la imagen animada (`.raw565pack`) de un juego en la navegación rápida. El tipo de cada sistema (fijo, animado o ambos) vuelve a deducirse de sus **imágenes de juegos**, subcarpetas incluidas; las imágenes aún no convertidas cuentan como lo que serán (`.png` = fija, `.gif` = animada), así que el Modo 7 sigue funcionando justo después de un Modo 3. La entrada del 2026-09-24 más abajo era errónea en este punto: el tipo no viene de `systems/_defaults/`.
- **Toolkit PC**: el **Modo 4** convierte ahora las imágenes colocadas directamente en la carpeta elegida (sin subcarpeta de sistema) — antes indicaba «0 PNG, 0 GIF» sin ningún error. Se escriben en `systems/<nombre de la carpeta>/`.

## 2026-09-24 — Toolkit PC: caché de sistemas corregida, carpetas por defecto de los Modos 2/3/6/7/11

- **Toolkit PC** (build `8053`): corregido el **Modo 7**, que indicaba «0 sistemas encontrados» en una carpeta cuyas imágenes de juegos aún no están convertidas (por ejemplo justo después de un Modo 3). El tipo de cada sistema (imagen fija, animación o ambas) se lee ahora de su imagen por defecto en `systems/_defaults/`, exactamente como hace el firmware del DMD — antes se deducía por error de las imágenes de juegos de la carpeta del sistema. También se aplica al Modo 1. La caché de sistemas solo lista las carpetas de sistemas realmente presentes (más `default`), nunca más de lo que el DMD puede guardar.
- **Toolkit PC**: el indicador «lento» de la caché de sistemas cuenta también las imágenes aún sin convertir (un `.png` cuenta como su futuro `.raw565`, un `.gif` como su futuro `.raw565pack` + `.meta`): una carpeta grande como `mame/S` queda marcada como lenta incluso antes de la conversión.
- **Toolkit PC**: los Modos 2, 3 y 11 abren la carpeta de trabajo al terminar; los Modos 6 y 7 apuntan por defecto a la carpeta `systems` de la carpeta de trabajo si existe (una carpeta elegida a mano se conserva).

## 2026-09-23 (2) — Toolkit PC: diseño del Modo 9 corregido, limpieza de scripts

- **Toolkit PC** (build `7951`): los botones Pausar / Reanudar / Saltar / Parar del marco Progreso ya no desaparecen por debajo de la ventana durante el Modo 9 (se desplazaban al aparecer el texto de resultado). El marco Progreso conserva siempre su sitio, el resultado del Modo 9 tiene una altura fija y la descripción del Modo 9 es más corta (ahora menciona la copia de los `.hi`).
- **Toolkit PC**: tras instalar los scripts, el mensaje pide **reiniciar la Recalbox** — reiniciar solo EmulationStation deja funcionando los scripts antiguos.
- **Scripts de Recalbox** (`dmd_score` v54, `dmd_achievement` v8): eliminados restos de código MQTT (el DMD ya no usa MQTT); el registro del puente de RetroAchievements es ahora `dmd_achievement.log`. Sin cambios de comportamiento. Reinstala con el Modo 9 y reinicia la Recalbox.

## 2026-09-23 — Récords independientes de la versión de MAME, tablas verificadas de MAME corregidas

- **Scripts de Recalbox** (`dmd_hiscore_verified.py` v2): corregidas las tablas de récords verificadas a mano **que nunca se mostraban en los juegos MAME** — la búsqueda usaba el nombre del sistema (`mame`) mientras que las claves llevan la versión de MAME (`mame0278_…`). Unos 2270 juegos MAME muestran ahora su tabla cuando no existe un `.hi` real. Reinstala los scripts con el Modo 9.
- **Scripts de Recalbox** (`dmd_hiscore_generic.py` v6): la carpeta de récords de MAME ya no está fija en el código — el script usa el núcleo MAME que Recalbox ejecuta realmente (`mame.core` en `recalbox.conf`, si no la carpeta donde MAME escribió por última vez). Un futuro núcleo MAME no requiere actualizar el script.
- **Toolkit PC** (build `7651`): los `.hi` de MAME se copian en la carpeta del núcleo MAME en uso en la Recalbox (misma regla), en lugar de una carpeta `mame0278` fija. Un `.hi` existente sigue sin sobrescribirse nunca.
- **Datos**: en [`tools/hiscore_recalbox/`](tools/hiscore_recalbox/), los archivos MAME pasan a `hi/mame/hiscore/` (sin versión) y el Toolkit usa un nuevo `hiscore_hi_pack_v2.zip`; el zip antiguo se mantiene para el build 7550.

## 2026-09-22 — La caja de herramientas copia los archivos de récords, nueva carpeta de datos

- **Toolkit PC** (build `7550`): **el Modo 1 y el Modo 9 ahora también copian los archivos de récords (`.hi`)** — unos 3000 archivos de FBNeo y MAME 0.278 — en `/recalbox/share/saves` de la Recalbox, para que el DMD tenga puntuaciones que mostrar de juegos nunca jugados. **Un `.hi` ya presente en la Recalbox nunca se sobrescribe**; solo se añaden los que faltan. Por el recurso compartido de red, con SSH como alternativa. Las tablas de récords (JSON) ya se instalaban con los scripts.
- **Toolkit PC**: la pestaña Ayuda menciona ahora la copia de los `.hi` (3 idiomas).
- **Docs / datos**: nueva carpeta [`tools/hiscore_recalbox/`](tools/hiscore_recalbox/) — 3161 archivos `.hi` (FBNeo 2491, MAME 0.278 670), los dos archivos JSON de récords, un manifiesto de sumas de comprobación y un README, listos para entregar al equipo de Recalbox. La tabla `verified_default_scores.json` se actualiza (3941 entradas: 2277 mame0278 + 1664 fbneo); reinstala los scripts con el Modo 9 para obtenerla.

## 2026-09-20 (2) — Modo Visual Pinball (VPX) sin reinicio, nueva página de inicio de la configuración web, opciones de reposo de Recalbox

- **Firmware**: nuevo **Modo Pinball (VPX)** — una mesa de Visual Pinball lanzada desde Recalbox se muestra en directo en el DMD (protocolo ZeDMD-WiFi, plugin DMDUtil en el lado de Recalbox). **Sin reinicio del DMD**: cambia sobre la marcha al lanzar la mesa y vuelve a la normalidad al terminar la partida (o unos 5 s después del último fotograma). Mientras corre la mesa, la playlist y las pantallas de Recalbox («RecalBox conectada»…) quedan suspendidas para no dibujar nada encima. Desactivado por defecto; el brillo se restaura al salir. Por ahora solo probado con mesas 128×32.
- **Firmware**: para hacerle sitio se retocó el presupuesto de memoria — un descompresor zlib mínimo integrado (solo en pila) sustituye al anterior, más pesado, los búferes solo se reservan si el modo está activado, y el índice de sistemas/juegos en memoria se redimensionó (300 → 160 entradas; una línea de log lo indica si una colección enorme alcanza el límite). La memoria libre gana unos 12 KB.
- **Configuración web**: el menú principal (página de inicio) se abre ahora con el marco **Pantalla** — brillo, arranque silencioso e interruptor **Modo Pinball (VPX)**. Su botón Guardar muestra una confirmación en la línea 2 del DMD. La página Pantalla conserva las opciones de las superposiciones en juego.
- **Firmware**: el título de arranque del DMD muestra ahora la **versión real del firmware** (p. ej. `RawEdition v2.31`) en lugar de un `v2.0` fijo; la insignia del README y el Web Installer muestran el mismo número.
- **Firmware + script de Recalbox**: la opción Modo Pinball **autoriza** ahora un nuevo script de Recalbox, `dmd_vpx_config`, a comprobar y escribir los ajustes de Visual Pinball que necesita el DMD (DMDUtil / ZeDMD WiFi, AlphaDMD) en `VPinballX-configgen.ini` en cada arranque de la Recalbox y después de cada partida — nunca mientras corre una mesa, y sin sobrescribir una IP que hayas escrito. Sin marcar, el script no hace nada. Un aviso en la burbuja de ayuda de la opción (config web) lo indica. Requiere reinstalar los scripts con el Modo 9.
- **Firmware**: corregido el modo Pinball en mesas coloreadas (Serum) — las grandes actualizaciones de color se perdían en parte, con fondos dibujados unos sobre otros y textos ausentes. El emisor parte un mensaje en dos paquetes UDP cuando supera 1400 bytes; el DMD ahora reensambla esos mensajes en vez de descartarlos. Las mesas coloreadas (Diner…) se muestran correctamente.
- **Firmware**: corregida la ausencia de las superposiciones hi-score / info del juego / descripción cuando el DMD se enciende *antes* que la Recalbox (solo reiniciar el DMD lo arreglaba) — el DMD reenvía ahora sus ajustes a la Recalbox en la primera respuesta tras el arranque o tras un corte.
- **Docs**: las mesas antiguas con display de segmentos (alfanumérico) requieren activar el plugin VPX **AlphaDMD** en el lado de Recalbox — ver el README (sección Pinball). Probado con 8 mesas: 6 funcionan; dos (Black Hole, Farfalla) tienen una disposición de display que ese plugin no soporta.
- **Firmware**: corregida la pantalla *RecalBox conectada* que aparecía con la Recalbox apagada (y luego se sustituía por *RecalBox sin conexión*) tras «Reanudar DMD», el botón Guardar de la página de inicio o al salir del modo Pinball — ahora solo aparece cuando la Recalbox ha respondido de verdad.
- **Configuración web**: un botón **Inicio** figura ahora en primer lugar en la barra de menú de todas las páginas de configuración (Pantalla, Playlist, Wi-Fi y BT, Reloj, Medios).
- **Configuración web**: nueva sección **Reposo de Recalbox** — durante los salvapantallas «demos de juegos» / «clips de vídeo de juegos», elige entre seguir el logo del juego (como antes, por defecto) o la playlist simple, como los demás reposos.
- **Scripts de Recalbox** (`marquee` v54, `dmd_score` v53): las opciones de reposo anteriores y una salida inmediata del Modo Pinball al terminar una partida. Reinstala los scripts con el Modo 9 para tenerlos.
- **Docs**: el README (3 idiomas) documenta el Modo Pinball, la página de inicio y las nuevas claves de `config.ini` (`feat_vpinball_dmd`, `feat_demo_follow`, `feat_clip_follow`).
- **Toolkit PC** (build `7449`): la pestana Ayuda es ahora un manual de uso real (primer arranque, Modo 1 paso a paso, cada modo de la pestana Avanzado, pestana Playlist, scripts de Recalbox incluido `dmd_vpx_config`, resolucion de problemas) en lugar de una copia del README; sin menciones a MQTT.

## 2026-09-20 — IP del DMD descubierta automáticamente, Toolkit PC adaptado al DPI de Windows, imagen shuffle instalada, renombrado de etiqueta SD restablecido

- **Scripts de Recalbox**: corregido el DMD que se quedaba atascado en su playlist de reposo durante una partida — los scripts tenían la dirección IP del DMD escrita a fuego (`192.168.0.51`) y hablaban al vacío en cualquier red donde el DMD tuviera otra dirección. Ahora usan la dirección desde la que el DMD se anuncia realmente (recurren al valor antiguo mientras no se haya visto ninguna). Nace de un informe real de un probador.
- **Firmware**: el renombrado de la etiqueta de la tarjeta SD vuelve a estar activo — se había quedado desactivado tras una sesión de diagnóstico.
- **Toolkit PC**: toda la interfaz sigue ahora la escala de pantalla de Windows (125 %, 150 %...) y no solo las fuentes — al 125 % en una pantalla 4K la parte inferior de la ventana (la sección Progreso) quedaba cortada. Se aplica al volver a iniciar sesión si la escala se acaba de cambiar.
- **Toolkit PC**: el número de build se muestra ahora en la barra de título de la ventana (actualmente `7349`), para saber fácilmente qué versión usa alguien.
- **Toolkit PC**: el Modo 9 recuerda ahora reiniciar EmulationStation tras instalar los scripts de usuario — el menú *Scripts de usuario* sigue en gris hasta que EmulationStation vuelve a leer sus scripts al arrancar.
- **Toolkit PC**: la creación de la tarjeta SD instala también la imagen de interferencia CRT del modo shuffle (`_shuffle.raw565pack` + `_shuffle.meta`); antes estos dos archivos se omitían y el DMD mostraba una pantalla vacía en modo shuffle. Si tu SD se creó con un build anterior, copia esos dos archivos desde `carte SD/systems/_defaults/` a `systems/_defaults/` en la tarjeta.
## 2026-09-14 — MQTT eliminado por completo, crash del watchdog corregido, IP fija opcional, página de FAQ

- **Firmware**: el subsistema MQTT (código de conexión/tarea, ~800 líneas) ahora se ha eliminado por completo del código fuente — no solo desactivado por defecto como el día anterior. Si tenías algo conectado a los antiguos topics MQTT del DMD, consulta [UPGRADING.md](UPGRADING.es.md) para saber qué implica.
- **Firmware**: corregida una pantalla de "RecalBox conectada" que podía aparecer incluso con la Recalbox apagada — antes se activaba con el simple ENVÍO de un mensaje UDP, sin confirmación de que realmente se hubiera recibido; ahora espera una respuesta real antes de mostrarse.
- **Firmware**: corregido un crash real — una simple visita normal a la página de configuración web podía, en ocasiones, activar el watchdog por hardware del ESP32 y forzar un reinicio (`WebServer::_parseRequest()` podía quedarse esperando activamente hasta 5 segundos sin ceder el control, justo al límite de la ventana de 5 segundos del propio watchdog de la plataforma). Corregido y confirmado en hardware real: 300 solicitudes rápidas reproduciendo exactamente el patrón que antes lo hacía fallar, cero fallos.
- **PC Toolkit**: el paso de configuración WiFi del Modo 1 ahora también puede establecer una IP fija en el DMD (opcional, desactivado por defecto) — incluye una advertencia explícita contra configurar a la vez una reserva DHCP en el router Y una IP estática en el DMD (puede causar problemas de conexión reales), y rellena previamente los campos de red a partir de la configuración de este PC para reducir el riesgo de errores de escritura.
- **PC Toolkit**: corregido un falso error de "no se puede conectar con la Recalbox" en el Modo 1 incluso con la IP correcta — la comprobación de accesibilidad solo intentaba una conexión TCP directa al puerto SMB, que algunas reglas de firewall/antivirus bloquean para un proceso no reconocido, aunque el propio cliente SMB del Explorador de Windows accede sin problema al mismo recurso compartido; ahora recurre a un intento real de acceso al recurso compartido (lo mismo que hace el Explorador) antes de rendirse.
- **PC Toolkit**: intento de corrección para la desaparición de la parte inferior de la interfaz (la sección de Progreso) en pantallas de alta resolución (reportado por un usuario en una pantalla 4K al 125% de escala) — informes posteriores mostraron que no cubría todos los casos; hay una corrección adicional en curso.
- **Docs**: nueva página dedicada de [FAQ y solución de problemas](FAQ.es.md) — sensibilidad a la alimentación USB (brillo frente a consumo de corriente), notas sobre la revisión del chip ESP32, calidad de la tarjeta microSD, bucles de configuración WiFi, orientación sobre la IP fija, y más — enlazada desde la sección de solución de problemas del README.

## 2026-09-13 — MQTT → UDP: fusión de `dev/dmd-udp-transport` en master

- **Firmware**: el enlace en tiempo real entre Recalbox y el DMD pasa de **MQTT a UDP** — el transporte anterior chocaba con un muro a nivel de plataforma, dentro de la pila TCP/IP del ESP32 (un `TCP_SND_BUF` fijo de unos 5,7 KB, grabado en el core de Arduino precompilado, sin ninguna palanca posible a nivel de aplicación) que podía bloquear un `SUBSCRIBE` MQTT varios segundos tras una reconexión; UDP no tiene ese apretón de manos que bloquear. El soporte MQTT todavía estaba presente en el firmware en ese momento, desactivado por defecto, como vía de reversión — eliminado por completo al día siguiente, ver arriba.
- **Firmware**: caza de varios meses a un congelamiento de recepción UDP severo reportado históricamente (12 a 90s+) — nunca reproducido tras una campaña de instrumentación intensiva (sonda activa de congelamiento, medición del peor caso por llamada, tráfico real sostenido), aunque en el camino aparecieron y se corrigieron cerca de diez errores reales sin relación (ver abajo). Conclusión actual: los reportes originales eran muy probablemente un efecto colateral benigno de un umbral de detección demasiado agresivo frente a la pérdida normal de paquetes UDP, no un congelamiento real de recepción — detallado en `DECISIONS.md`.
- **Firmware**: corrige una falsa alerta de "Recalbox fuera de línea" disparada por la pérdida de un solo paquete UDP (normal y esperado en UDP) — ahora exige dos pérdidas consecutivas antes de avisar.
- **Firmware**: corrige la resincronización tras reconexión, que podía quedarse atascada en el modo de repliegue en caché (`MODE_PNG`) en lugar de ponerse al día con el estado real de Recalbox.
- **Firmware**: corrige "Reanudar DMD" (página de configuración web), que forzaba siempre la playlist de espera sin importar lo que Recalbox estuviera haciendo realmente (en juego, demo, gameclip...) — una comprobación obsoleta de la era MQTT dejaba la rama correcta inalcanzable desde el cambio a UDP; ahora resincroniza el estado real, igual que lo hace una reconexión.
- **Firmware**: reduce el número de aperturas SD por cambio de juego (hasta 6 antes, tan solo 1 en el mejor de los casos ahora) al recordar cuál de las dos convenciones de nombres de carpeta usa la tarjeta SD en lugar de probar ambas cada vez — también reduce la exposición a un raro fallo de asignación de memoria dentro del controlador del sistema de archivos SD; se añadió un reintento automático para la variante recuperable de ese mismo fallo.
- **Scripts de Recalbox**: corrige un proceso zombi del round-robin de hi-score que podía sobrevivir a un despertar del salvapantallas y seguir dibujando una puntuación antigua sobre el marquee indefinidamente.
- **Scripts de Recalbox**: corrige el modo `gameclip`/demo, que mostraba la playlist genérica en lugar del marquee del propio juego del clip (resto de una solución alternativa de la era MQTT que lo había desactivado en exceso).
- **Documentación**: el master anterior queda archivado como `archive/mqtt-pre-udp-transport-final` — último estado del proyecto antes de este cambio de transporte, conservado para poder volver atrás.
- **Marca**: la edición de firmware del proyecto se renombra **Raw565 Edition → RawEdition v2.0** (el formato de píxeles `raw565` en sí, y todo lo que se basa en él, no cambia — solo cambian el nombre de producto y la insignia de versión).

## 2026-09-06 — `dev/core-reassignment` fusionada a master: superposiciones en juego, Challenge RB, pestaña Playlist

La fusión más grande en la historia del proyecto — meses de trabajo en una rama separada, reconciliados con todo lo publicado en master mientras tanto, y luego probados punto por punto (cada conflicto resuelto y vuelto a probar individualmente) antes de llegar aquí. ¿Ya usas una versión anterior? Consulta **[UPGRADING.es.md](UPGRADING.es.md)**.

- **Firmware**: nuevo **sistema de superposiciones en juego** — mientras un juego está realmente en marcha, el panel puede alternar automáticamente el marquee con el verdadero **Hi-Score de MAME/FBNeo** (manifiesto comunitario, ~2758 juegos, decodificado desde el propio archivo de puntuación guardado por el emulador, sin lectura de RAM en vivo), la **Info del juego** (descripción/género/desarrollador/año desde `gamelist.xml`), los **RetroAchievements** desbloqueados, y la clasificación mensual del **Challenge** oficial de Recalbox. Totalmente pasivo en el lado del DMD — toda la lógica de tiempos vive en los scripts del lado de Recalbox, el firmware simplemente muestra lo que se le envía y vuelve solo al marquee con su propio temporizador local, para que un script lento o fallido nunca pueda dejar el panel bloqueado. Ver la [sección dedicada del README](README.es.md#superposiciones-en-juego--hi-score-info-del-juego-logros-y-challenge-rb).
- **Caja de herramientas de PC**: el **Modo 9** (y la instalación automática integrada en el Modo 1) ahora también instala los scripts de Hi-Score/Info del juego/Logros/Challenge (`dmd_helpers/`) y limpia los nombres de scripts antiguos de una instalación anterior — antes solo el paso de preparación separado del Modo 1 se encargaba de esto, no el propio Modo 9.
- **Caja de herramientas de PC**: el indicador «lento» del sistema de máscara (**«L»**, activa la pantalla de espera en colecciones enormes) ahora se calcula **por subcarpeta alfabética («bucket»)** en lugar de por sistema entero — un sistema con una subcarpeta grande y varias pequeñas ya no penaliza innecesariamente a las pequeñas. El umbral por defecto de la pestaña Ajustes cambia en consecuencia (5000 → 800, vuelve a tener sentido ahora que se aplica por bucket).
- **Caja de herramientas de PC**: **pestaña Playlist** — crea tus propias rotaciones en modo atracción combinando el pack de 600 GIFs y tus propios GIFs (arrastra una carpeta del PC); ahora también posible **en pleno Modo 1**, antes incluso de copiar la carpeta de trabajo a la tarjeta SD, y no solo después desde una tarjeta ya insertada.
- **Caja de herramientas de PC**: el idioma de la interfaz por defecto ahora es el del **sistema Windows** en el primer inicio (en lugar de siempre inglés) — una elección explícita en la pestaña Ajustes sigue teniendo siempre prioridad después.
- **Caja de herramientas de PC**: varios errores encontrados probando la fusión en vivo — un panel de «copia a SD» que quedaba visible podía empujar la barra de Progreso fuera de la ventana fija en algunos modos de la pestaña Avanzado, el sorteo aleatorio de tema podía caer en el tema «default» sin más, y cerrar la app en pleno proceso de añadir GIFs propios (paso de playlist del Modo 1) ahora retoma el proceso en lugar de cerrar la aplicación.
- **Firmware / MQTT**: los 12 topics separados `marquee/cmd/*` se consolidaron en un único topic `marquee/cmd` (payload compacto `CMD=/ARG=`) — reduce el número de suscripciones MQTT por (re)conexión de 12 a 2, limitando la exposición a una condición rara de la pila WiFi del ESP32 en la que una suscripción podía silenciosamente no salir nunca del dispositivo.

## 2026-08-19 — Detección de unidades extraíbles y posicionamiento de ventanas emergentes

- **Caja de herramientas de PC**: corregida la detección de tarjeta SD que fallaba silenciosamente en las builds recientes de Windows 11 — el listado de unidades dependía por completo de `wmic.exe`, retirado por defecto por Microsoft en las versiones recientes de Windows 11; un usuario veía su tarjeta SD en el Explorador de Windows pero nunca aparecía en la herramienta (Modo 1/6/8), sin ningún mensaje de error. La detección ahora pasa por `Get-CimInstance` (PowerShell), conservando la antigua llamada a `wmic` solo como último recurso para entornos poco habituales.
- **Caja de herramientas de PC**: corregidas varias ventanas emergentes (fin de copia, selección de unidad de tarjeta SD, confirmación de cierre) que aparecían fuera de pantalla o fuera de la ventana principal, sobre todo en configuraciones multi-monitor. Doble causa: la ventana principal nunca tenía una posición explícita al iniciar (ahora centrada explícitamente en la pantalla primaria al arrancar), y las builds `.exe`/`.msi` compiladas carecían de un manifiesto de Windows que declarara conciencia de DPI — presente de forma nativa al ejecutar desde el código fuente Python, pero ausente por defecto en la salida de PyInstaller/cx_Freeze, lo que podía falsear las coordenadas de ventana reportadas por Windows. El centrado de las ventanas emergentes ahora también se confina al monitor real que Windows reporta para la ventana principal, en lugar del tamaño de pantalla primaria que es lo único que Tk conoce.

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
