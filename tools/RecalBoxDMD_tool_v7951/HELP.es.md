# 🎮 Caja de herramientas PC RecalBoxDMD — Ayuda

🇪🇸 **Español** · 🇬🇧 English (`HELP.md`) · 🇫🇷 Français (`HELP.fr.md`) — el idioma de esta página sigue el idioma elegido en la pestaña **Configuración**.

> Este es el **manual de uso de la caja de herramientas PC** (la aplicación de Windows que estás usando ahora). Para el proyecto en sí — firmware, página de configuración web, soporte de Visual Pinball, hardware — consulta la [página del proyecto en GitHub](https://github.com/shan-aya/RecalBoxDMD) y su [FAQ](https://github.com/shan-aya/RecalBoxDMD/blob/main/FAQ.es.md).

---

## 📋 Contenido

1. [Qué hace la caja de herramientas](#1-qué-hace-la-caja-de-herramientas)
2. [Antes de empezar](#2-antes-de-empezar)
3. [La ventana de un vistazo](#3-la-ventana-de-un-vistazo)
4. [Primer uso — la forma fácil (Modo 1)](#4-primer-uso--la-forma-fácil-modo-1)
5. [El Modo 1 en detalle](#5-el-modo-1-en-detalle)
6. [La pestaña Avanzado — Modos 2 a 11](#6-la-pestaña-avanzado--modos-2-a-11)
7. [La pestaña Playlist](#7-la-pestaña-playlist)
8. [Las pestañas Configuración, Logs y Ayuda](#8-las-pestañas-configuración-logs-y-ayuda)
9. [Scripts de Recalbox (Modo 9)](#9-scripts-de-recalbox-modo-9)
10. [Tras la copia — primer arranque del DMD](#10-tras-la-copia--primer-arranque-del-dmd)
11. [Solución de problemas](#11-solución-de-problemas)

---

## 1. Qué hace la caja de herramientas

El DMD es un panel LED de 128 × 32 controlado por un ESP32. Lee sus imágenes de una tarjeta microSD, en un formato compacto que no necesita decodificación en el panel. La caja de herramientas **construye esa tarjeta SD por ti**:

- lee los archivos `gamelist.xml` de tu Recalbox y toma la imagen **marquee / logo** de cada juego (la que has obtenido con el scraper);
- lo convierte todo al formato del panel (`.raw565` para imágenes fijas, `.raw565pack` + `.meta` para GIFs) y construye los dos archivos de índice del firmware (`games_cache.bin`, `systems_cache.dat`);
- descarga las imágenes por defecto (logos de sistemas, géneros, imagen de respaldo) y, si quieres, el pack gratuito de unos 600 GIFs para las playlists de reposo;
- puede configurar el WiFi del DMD, instalar los **scripts de Recalbox** que enlazan el juego que lanzas con el panel, y copiarlo todo a la tarjeta SD.

**No** necesitas conocer los formatos de archivo: el **Modo 1** automático lo hace todo y te hace algunas preguntas por el camino.

---

## 2. Antes de empezar

- **Windows 10 u 11.** La caja de herramientas existe en versión portable o instalada; ambas funcionan igual.
- **Una tarjeta microSD de al menos 8 GB, formateada en FAT32** (la caja de herramientas comprueba ambas cosas y rechaza lo demás).
- **Tu carpeta de ROMs de Recalbox**, accesible desde este PC — un recurso de red (`\\RECALBOX\share\roms`, o tu NAS) o una copia en un disco. Los sistemas deben haberse **escrapeado** para que cada juego tenga una imagen marquee/logo (ver [Scraping](#scraping--obtener-las-imágenes-marquee)).
- **Tu Recalbox encendida y en la misma red** si quieres que la caja de herramientas instale los scripts por ti (si no, puedes copiarlos a mano, ver el [Modo 9](#9-scripts-de-recalbox-modo-9)).
- **El nombre y la contraseña de tu WiFi** — una red de **2,4 GHz**; el ESP32 no puede usar 5 GHz.

### Scraping — obtener las imágenes marquee

La caja de herramientas lee la imagen declarada en cada `gamelist.xml`. Dónde la guarda Recalbox depende de tu versión — elige el perfil correspondiente en **Versión de Recalbox** (pestaña Main o Configuración) y usa el botón **Como hacer el scrape?** para ver las capturas exactas del menú:

| Perfil | Ajuste del scraper de Recalbox | Carpeta / etiqueta leída |
|---|---|---|
| **10.x** (recomendado) | campo **SELECCIONAR TIPO DE LOGO** = **CLEAR** | `media/wheels/`, etiqueta `<logo>` |
| **9.x** | tipo de miniatura = **MARQUEE** | `media/thumbnails/`, etiqueta `<thumbnail>` |
| **legacy** | tipo de imagen = **LOGO RECORTADO** (Clear Logo) o **MARQUEE** | `media/images/`, etiqueta `<image>` |

El botón **Limpiar carpetas antes del scrape** borra las imágenes ya obtenidas de los sistemas seleccionados (solo esas imágenes — nunca las ROMs ni los `gamelist.xml`), útil antes de repetir el scraping con otro ajuste. Pide confirmación.

---

## 3. La ventana de un vistazo

La ventana tiene **seis pestañas**:

| Pestaña | Para qué sirve |
|---|---|
| **Main** | El **Modo 1** automático: elegir la carpeta de ROMs, los sistemas, la versión de Recalbox y pulsar **INICIAR**. |
| **Playlist** | Construir tus propias playlists de reposo con los GIFs de la tarjeta SD o de carpetas de GIFs de tu PC. |
| **Avanzado** | Los modos 2 a 11 por separado, agrupados por tema (descargas de GitHub, gamelist, herramientas de imágenes, cachés, scripts). |
| **Logs** | Toda la salida de texto de lo que hace la caja de herramientas, con un filtro de nivel. |
| **Configuración** | Idioma (francés / inglés / español), tema de colores, perfil de versión de Recalbox, umbral flag L (sistemas lentos). |
| **Ayuda** | Esta página. |

Abajo, el panel **Progreso** muestra el paso actual con cuatro botones: **Pausar / Reanudar**, **Saltar** (salta el paso actual) y **Parar**. El número de la barra de título es el **número de compilación** de tu caja de herramientas — indícalo al pedir ayuda.

Una carpeta de trabajo temporal (`sd_card`) sirve para preparar todo antes de copiarlo a la tarjeta SD. Al salir, la caja de herramientas ofrece borrarla (marca *Conservar la carpeta temporal* para mantenerla).

---

## 4. Primer uso — la forma fácil (Modo 1)

1. Abre la pestaña **Main**.
2. **Elegir carpeta ROMs** — la carpeta que contiene una subcarpeta por sistema (`snes`, `mame`, …). Si las ROMs están en un NAS que pide credenciales, aparece un cuadro **Credenciales del NAS**: escribe el usuario y la contraseña.
3. Pulsa **Detectar sistemas (gamelist.xml)**. Se rellena la lista *Sistemas a procesar*. Haz clic para seleccionar los sistemas que quieras (o **Seleccionar todo**). *Si no seleccionas ninguno, la caja de herramientas te lo dice y se detiene.*
4. Comprueba el perfil **Versión de Recalbox** (ver [Scraping](#scraping--obtener-las-imágenes-marquee)).
5. Pulsa **INICIAR** y responde a las preguntas (se hacen todas de entrada, para que luego puedas dejar el PC trabajando).
6. Cuando diga **¡Terminado!**, elige **Explorar la tarjeta SD** para ver el resultado, pon la tarjeta en el DMD y enciéndelo (ver [Primer arranque](#10-tras-la-copia--primer-arranque-del-dmd)).

---

## 5. El Modo 1 en detalle

### Las preguntas al pulsar INICIAR

En este orden:

1. **WiFi del DMD (opcional)** — elige la red de **2,4 GHz**, escribe la contraseña y pulsa **Verificar**: el PC se conecta brevemente a esa red para comprobar que la contraseña es correcta. *Omitir (configurar más tarde)* es válido: el DMD ofrecerá entonces su propio punto de acceso en el primer arranque.
   - **Definir también una IP fija para el DMD (avanzado)** — solo si tu router *no* da ya una dirección fija al DMD. Usa **uno solo** de los dos métodos (una reserva en el router, **o** esta IP fija), nunca ambos. Los campos se rellenan según la red de tu PC, a modo orientativo: compruébalos.
2. **Recalbox** — la caja de herramientas busca tu Recalbox en la red y muestra su **dirección IP** para confirmarla (importante si hay varias Recalbox encendidas). *No* / no encontrada → escribe su IP o nombre a mano; si no se puede alcanzar, puedes **reintroducir la IP** o elegir **Modo 9 más tarde**.
3. **Imagen de respaldo** — la imagen que muestra el panel cuando un juego no tiene ninguna. Sí → elige una de las propuestas o importa la tuya (se redimensiona sola). No → se mantiene la actual.
4. **Idioma de las imágenes de sistemas** — francés, inglés o español para las insignias de sistemas/géneros (Favoritos, Últimos jugados, …).
5. **Tarjeta SD** — elige la unidad (**Actualizar** si no aparece). FAT32 y 8 GB como mínimo.
6. **Pack de GIFs gratuito** — descarga los ~600 GIFs (arcade, consolas, ordenadores, pinball, Halloween, Navidad, …) desde GitHub para las playlists de reposo. La descarga empieza enseguida en segundo plano.
7. **GIFs personalizados** — *Sí* te lleva a la pestaña **Playlist** en modo temporal: **Añadir una carpeta del PC…**, **Copiar selección**, opcionalmente **Construir playlist**, y luego pulsa el botón naranja parpadeante **Continuar** para reanudar.

### Lo que hace después el proceso automático

1. **Prepara** la carpeta de trabajo y escribe el idioma y el indicador de «primer arranque» en el `config.ini` del DMD.
2. **Instala los scripts de Recalbox** (ver [Modo 9](#9-scripts-de-recalbox-modo-9)) — primero una copia local en `recalbox_userscripts`, luego en la propia Recalbox si se confirmó. La IP de la Recalbox se escribe en el `config.ini` para que la página web del DMD venga rellenada. Después copia los archivos de récords (ver [Modo 9](#9-scripts-de-recalbox-modo-9)).
3. **Extrae** la imagen marquee de cada juego de los `gamelist.xml` (la lista de imágenes que faltan se guarda en `images_manquantes.txt`).
4. **Convierte** al formato bruto de 128 × 32 y elimina los `.png`/`.gif` originales que se convirtieron.
5. **Construye `games_cache.bin`**, descarga las imágenes **`_defaults`** (con tu idioma y tu imagen de respaldo), el **pack de GIFs** y las **playlists** (una playlist por defecto más `ALL.txt`), y después construye **`systems_cache.dat`**.
6. **Copia a la tarjeta SD.** Si ya hay archivos, se te pregunta si *sobrescribir* u *omitir*; una copia interrumpida se puede **reanudar** la próxima vez.

Según el tamaño de tu colección, puede tardar desde unos minutos hasta un buen rato (un set de MAME de 30 000 juegos es el caso lento). Puedes **pausar**, **saltar** un paso o **detener** en cualquier momento.

---

## 6. La pestaña Avanzado — Modos 2 a 11

La columna izquierda agrupa los modos en cinco categorías desplegables; el centro muestra las opciones del modo elegido; **Detalles del modo** lo explica. Úsala para rehacer solo una parte del trabajo.

| Categoría | Modo | Qué hace |
|---|---|---|
| **DOWNLOAD FROM GITHUB** | **2 — `_defaults`** | Extrae los marquees *y* descarga las imágenes por defecto (`systems/_defaults`) desde GitHub. Pregunta si se sobrescriben los archivos existentes. |
| | **11 — Pack de 600 GIFs** | Descarga el pack de GIFs gratuito en `gifs/`. |
| **GAMELIST.XML** | **3 — Extracción** | Solo extrae las imágenes marquee de los `gamelist.xml` (eliges la carpeta de ROMs y los sistemas). |
| | **8 — Imágenes faltantes** | Comprueba qué juegos **no** tienen imagen y escribe un informe (`mode8_report_*.txt`); un segundo botón compara ese informe con la carpeta de trabajo y la tarjeta SD (`mode8_final_report_*.csv/.txt`). |
| **IMAGES TOOLS** | **4 — PNG/GIF → raw565** | Convierte los PNG (→ `.raw565`) y GIF (→ `.raw565pack` + `.meta`) de la carpeta que elijas. |
| | **5 — 128×32** | Solo redimensiona/convierte las imágenes a 128 × 32. |
| | **10 — Imagen de respaldo** | Elige (o restablece) la imagen que se muestra cuando nada más coincide. |
| **CACHES** | **6 — `games_cache.bin`** | Reconstruye el índice de juegos a partir de la carpeta `systems`. |
| | **7 — `systems_cache.dat`** | Reconstruye el índice de sistemas (requiere la carpeta `systems` que contiene `_defaults`). |
| **SCRIPTS RECALBOX** | **9 — Instalar scripts** | Instala / actualiza los scripts de Recalbox (ver más abajo). |

Cada modo tiene su propio botón **Elegir carpeta …** y su propio botón **INICIAR**. Los modos 3, 4, 5, 6 y 7 trabajan sobre la carpeta de trabajo actual salvo que los apuntes a otra.

**Umbral flag L (sistemas lentos)** (pestaña Configuración): los sistemas con más archivos convertidos que este número se marcan como *lentos*; el panel muestra entonces una imagen por defecto mientras carga la real. Súbelo si tu tarjeta SD es rápida, bájalo si es lenta.

---

## 7. La pestaña Playlist

Una **playlist** es la lista de GIFs que reproduce el DMD cuando no ocurre nada en la Recalbox (reposo / modo atracción).

- **Tarjeta SD** — elige la unidad (🔄 *Actualizar*). La pestaña muestra las carpetas `gifs/` y sus archivos.
- **Marca una carpeta entera** para tomar todos sus GIFs, o **haz clic en su nombre** para marcar archivos uno a uno (pasar el ratón por un archivo = vista previa animada). Una fila **naranja** indica una selección parcial; la carpeta mostrada aparece subrayada.
- **Añadir una carpeta del PC…** importa una o varias carpetas de GIFs de tu ordenador (varias a la vez es posible). Desmarca los archivos que no quieras y pulsa **Copiar selección**.
- **Eliminar** quita las carpetas/archivos marcados (definitivo, con confirmación; las playlists que los usaban se actualizan).
- **Nombre de la playlist** + **Construir playlist** guarda tu selección con ese nombre (existente o nuevo).
- **Regenerar caché de playlist** (botón naranja) reconstruye `cache_master_gifs.dat`, un resumen de todos los GIFs de la tarjeta (uso interno).

En la página web del DMD (página Playlist) se elige **cuál** playlist está activa.

---

## 8. Las pestañas Configuración, Logs y Ayuda

- **Configuración** — *Idioma* (Français / English / Español, se aplica a toda la aplicación), *Tema* (colores de la ventana), *Versión de Recalbox* (el perfil de scraping, compartido con la pestaña Main) y el *Umbral flag L (sistemas lentos)*. Tus elecciones se recuerdan.
- **Logs** — todo lo que la caja de herramientas escribe mientras trabaja. El filtro **Nivel** muestra *Todo*, *Alertas+Errores* o solo *Errores*. Cuando algo va mal, es el primer sitio donde mirar y lo que hay que copiar al informar de un problema.
- **Ayuda** — este manual. **Abrir en el navegador** lo muestra en tu navegador web.

---

## 9. Scripts de Recalbox (Modo 9)

El DMD solo muestra lo que la Recalbox le dice. El enlace es un conjunto de pequeños **scripts** que Recalbox ejecuta en sus eventos (juego seleccionado, juego iniciado, juego terminado, salvapantallas…). **El Modo 1 los instala por ti**; el **Modo 9** los instala o los **actualiza** en cualquier momento (hazlo tras cada actualización de la caja de herramientas).

**Cómo:** pestaña *Avanzado* → *SCRIPTS RECALBOX* → **Modo 9**, escribe la **IP o el nombre de red** de la Recalbox y pulsa **Instalar / Actualizar**. La caja de herramientas copia los archivos a la carpeta `share/userscripts` de la Recalbox — por el recurso de red, o automáticamente por SSH si el recurso está bloqueado.

**Qué se instala**

- **Scripts de eventos** (se ejecutan solos): el puente *marquee*, el script de *hi-score / info del juego / Challenge RB*, el script de *RetroAchievements* y `dmd_vpx_config`, que ajusta los parámetros de DMD de Visual Pinball **solo si** la opción **Modo Pinball (VPX)** está marcada en la página web del DMD. Sus archivos auxiliares van en `dmd_helpers/`.
- **Scripts manuales** (en el menú de Recalbox **Scripts de usuario**): *DMD Config Web*, *DMD Brillo +10 % / −10 %*, *DMD Reboot* y *DMD WiFi Recovery*.
- **Archivos de récords (`.hi`)**: el Modo 1 y el Modo 9 también copian unos 3000 archivos de récords (FBNeo y MAME 0.278) en la carpeta `share/saves` de la Recalbox, para que el DMD tenga puntuaciones que mostrar de juegos que aún no has jugado. **Un `.hi` que ya existe en tu Recalbox nunca se sobrescribe** — solo se añaden los que faltan. El registro termina con una línea del tipo *N .hi copiados, M ya presentes*. Las tablas de récords (JSON) viajan con los scripts, en `dmd_helpers/`.

> **Reinicia EmulationStation** (o la Recalbox) tras instalar, si no el menú **Scripts de usuario** sigue en gris: Recalbox solo busca los scripts al arrancar.

**Si la Recalbox no es alcanzable** (apagada, IP equivocada…), la caja de herramientas conserva una versión lista para copiar en la carpeta `recalbox_userscripts` de la carpeta de trabajo: copia su contenido tú mismo en `share/userscripts` de tu Recalbox.

---

## 10. Tras la copia — primer arranque del DMD

1. Pon la tarjeta en el DMD y enciéndelo. Muestra su título (`RawEdition v…`) y el panel inicia la playlist de reposo.
2. Si introdujiste el WiFi en el Modo 1, el DMD se une solo a tu red. Si no, abre su propio punto de acceso WiFi: conéctate y sigue la página (elegir la red y la IP de la Recalbox).
3. Abre la **página de configuración web** del DMD (su IP se muestra en el panel) para ajustar el brillo, la playlist, el reloj y el enlace con la Recalbox. La página del proyecto explica cada opción.
4. Inicia un juego en la Recalbox: el panel debe cambiar al marquee de ese juego.

---

## 11. Solución de problemas

| Problema | Qué hacer |
|---|---|
| **La tarjeta SD no aparece / se rechaza** | Debe ser **FAT32** y **≥ 8 GB**. Pulsa **Actualizar**. Reformatea en FAT32 si hace falta (una tarjeta de 64 GB o más debe formatearse con una herramienta FAT32). |
| **«Ningún sistema detectado»** | La carpeta de ROMs es incorrecta (elige la que contiene las carpetas de sistemas) o faltan las carpetas de imágenes. Con un NAS, introduce **usuario / contraseña del NAS** y vuelve a pulsar *Detectar sistemas*. |
| **Muchos juegos sin imagen** | El perfil de scraping no coincide con cómo lo hiciste (ver [Scraping](#scraping--obtener-las-imágenes-marquee)) — cambia la **Versión de Recalbox** y usa **Como hacer el scrape?**. El Modo 8 lista las que faltan. |
| **«Recalbox inalcanzable»** | Comprueba que está encendida y en la misma red, y su IP. Reintenta con la IP, o termina y usa el **Modo 9** más tarde, o copia `recalbox_userscripts` a mano. Sin los scripts, el DMD solo muestra la playlist y el reloj. |
| **El menú Scripts de usuario está en gris** | Reinicia EmulationStation (o la Recalbox) — ver [Modo 9](#9-scripts-de-recalbox-modo-9). |
| **El DMD no se conecta al WiFi** | Usa una red de **2,4 GHz** y comprueba la contraseña (el Modo 1 puede verificarla). Si definiste una IP fija, asegúrate de que el router no reserve otra (un solo método). |
| **El DMD no muestra nada al iniciar un juego** | Scripts ausentes o antiguos → vuelve a ejecutar el **Modo 9** y reinicia EmulationStation. Comprueba la IP de la Recalbox en la página web del DMD. |
| **La ventana se corta / texto demasiado pequeño (pantalla de alta densidad)** | La interfaz sigue la escala de pantalla de Windows; cierra sesión y vuelve a entrar tras cambiar la escala. El número de compilación de la barra de título nos ayuda a reproducir un problema. |
| **Una copia a la tarjeta SD se detuvo** | Vuelve a lanzar la copia del Modo 1: la caja de herramientas ofrece **reanudar** donde se detuvo. |

**¿Sigues atascado?** Copia el contenido de la pestaña **Logs** y el número de compilación de la barra de título, y abre una incidencia en la [página de GitHub](https://github.com/shan-aya/RecalBoxDMD) — o lee antes la [FAQ](https://github.com/shan-aya/RecalBoxDMD/blob/main/FAQ.es.md) (alimentación USB, calidad de la tarjeta SD, bucles de WiFi, IP fija…).
