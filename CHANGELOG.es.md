# Changelog

Historial de **RecalBoxDMD — RawEdition v2.0**, cubriendo tanto el **firmware ESP32** (incluida su página de configuración web) como la **caja de herramientas de PC**, desde el primer commit hasta hoy. Las entradas se agrupan por fecha; cada punto está etiquetado con la parte del proyecto a la que afecta.

[🇬🇧 English](CHANGELOG.md) · [🇫🇷 Français](CHANGELOG.fr.md) · 🇪🇸 **Español**

Este es un resumen seleccionado del historial interno de versiones del proyecto (185+ revisiones de firmware, 64+ de la config web, 43+ de la caja de herramientas, 62+ de la GUI) — agrupado por los hitos que realmente importan si usas el proyecto, no un volcado en bruto de cada micro-arreglo.

---

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
