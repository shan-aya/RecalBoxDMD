# 🎮 RecalBoxDMD Toolkit — Ayuda completa

> **RecalBoxDMD** = Recalbox + un letrero LED real de 128×32 para tu mueble arcade, controlado desde tu PC con esta herramienta.

---

## 📋 Índice

1. [Visión general](#-visión-general)
2. [Requisitos de hardware](#-requisitos-de-hardware)
3. [Requisitos de software](#-requisitos-de-software)
4. [Inicio rápido](#-inicio-rápido)
5. [Interfaz gráfica (GUI)](#️-interfaz-gráfica-gui)
   - [Pestaña Main](#-pestaña-main)
   - [Pestaña Playlist](#-pestaña-playlist)
   - [Pestaña Avanzado](#-pestaña-avanzado)
   - [Pestaña Logs](#-pestaña-logs)
   - [Pestaña Ajustes](#️-pestaña-ajustes)
   - [Pestaña Ayuda](#-pestaña-ayuda)
6. [Referencia de modos](#-referencia-de-modos)
7. [Flujo de trabajo recomendado](#-flujo-de-trabajo-recomendado)
8. [Tutorial de scraping de Recalbox](#-tutorial-de-scraping-de-recalbox)
9. [Montaje del panel DMD](#-montaje-del-panel-dmd)
10. [Firmware ESP32](#-firmware-esp32)
    - [Compilar desde el código fuente](#-compilar-desde-el-código-fuente)
    - [Configuración (config.ini)](#️-configuración-configini)
    - [Instalador Web](#-instalador-web)
    - [Enlace UDP y Telnet](#-enlace-udp-y-telnet)
11. [Overlays en partida — Hi-Score, info del juego, logros y RB Challenge](#-overlays-en-partida--hi-score-info-del-juego-logros-y-rb-challenge)
12. [El formato raw565 en detalle](#-el-formato-raw565-en-detalle)
13. [Estructura de la tarjeta SD](#-estructura-de-la-tarjeta-sd)
14. [Solución de problemas](#-solución-de-problemas)
15. [Archivos generados por el script](#-archivos-generados-por-el-script)
16. [Créditos](#-créditos)

---

## 🎯 Visión general

**RecalBoxDMD Toolkit** es la aplicación de Windows que prepara todo lo que necesita un letrero LED real, a partir de tu colección de ROMs de Recalbox ya existente, hasta una tarjeta SD lista para usar — con un clic, incluso con un fullset de MAME de 30.000 juegos.

### 🔥 El problema que resuelve

Un firmware ingenuo que lee archivos PNG/GIF directamente desde la tarjeta SD se ahoga con colecciones grandes:

```
❌ Sin esta herramienta:
   - El ESP32 abre /systems/mame/ (30.000+ archivos) → se congela varios segundos
   - Decodificación de PNG/GIF en el propio ESP32 → lenta, consume mucha memoria
   - Entre cada juego: pantalla negra o retraso visible
```

La herramienta convierte todo de antemano en tu PC, en un formato que el ESP32 puede mostrar **sin decodificar nada**:

```
✅ Con esta herramienta:
   1. PNG → .raw565 (8192 bytes, listo para mostrar)
   2. GIF → .raw565pack (fotogramas concatenados) + .meta (tiempos)
   3. El ESP32 lee el archivo y lo envía directamente al panel
   4. Sin decodificación, sin retraso: 5-15 ms por visualización
```

| Métrica | Sin conversión | Con RecalBoxDMD Toolkit |
|--------|--------------------|---------------------------|
| Tiempo de visualización | 500 ms – 3 s+ | **5-15 ms** |
| RAM necesaria en el ESP32 | 50-100 KB | **8 KB** |
| Fullset de MAME (30.000 juegos) | se congela 5-10 s | **sin congelamiento, sin pantalla negra** |
| Configuración | manual, archivo por archivo | **un clic en "Iniciar"** |

### El sistema de máscara, en resumen

Los sistemas muy grandes (MAME, FBNeo...) se marcan como **"L"** en la herramienta. Cuando el firmware recibe un juego de ese tipo, muestra **al instante** la imagen por defecto del sistema desde la caché mientras la imagen real se decodifica en segundo plano — el panel nunca se queda en negro, incluso navegando rápido por una colección enorme. Detalle completo en [El formato raw565 en detalle](#-el-formato-raw565-en-detalle).

### 🔄 Cómo encaja todo

```
┌─────────────────────────────────────────────────────────────┐
│                          RECALBOX                            │
│   Lanza un juego → marquee[...].sh envía "mame/kof98"          │
│                          vía UDP                               │
└──────────────────────────────┬────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────┐
│              ESP32 + Panel LED HUB75 128×32                    │
│                                                                 │
│  Recibe "mame/kof98":                                           │
│   1. /systems/mame/kof98.raw565 (o .raw565pack) → instantáneo  │
│   2. ¿no encontrado? busca en games_cache.bin (índice bigrama) │
│   3. ¿aún no? muestra /systems/_defaults/mame.raw565           │
│   4. ¿aún no? muestra /systems/_defaults/default.raw565        │
│                                                                 │
│   ⏱️ 5-15 ms en total, sea cual sea el tamaño de la colección    │
└─────────────────────────────────────────────────────────────┘

           ┌───────────────────────────────────────────────┐
           │  RecalBoxDMD Toolkit (esta app) — prepara la SD    │
           │  Extrae los letreros desde gamelist.xml           │
           │  PNG → .raw565   /   GIF → .raw565pack + .meta   │
           │  Construye la caché bigrama de juegos              │
           │  Marca los sistemas lentos ("L") para la máscara  │
           │  Descarga los recursos gratuitos (_defaults + 600 GIFs) │
           │  Instala los scripts de Recalbox (Modo 9)          │
           │  Copia todo a la tarjeta SD (con reanudación)      │
           └───────────────────────────────────────────────┘
```

---

## 🧰 Requisitos de hardware

| Componente | Referencia | Precio aprox. |
|-----------|-----------|---------------|
| 🧠 Microcontrolador | ESP32 DevKit V1 USB‑C (38 pines) | ~5 $ |
| 🖥️ Panel LED | 2× paneles HUB75 RGB **P4, 64×32, 256×128 mm**, unidos uno al lado del otro (→ 128×32) | ~15-25 $/panel |
| 🔌 Placa de conexión | **DMDos Board V3** (recomendada — incluye el lector SD, sin soldadura) | ~15 $ |
| 💾 Lector SD | Adaptador Micro SD SPI (integrado en la DMDos Board) | ~2 $ |
| ⚡ Fuente de alimentación | 5V 4A+ | ~10 $ |

> **💡 Consejo**: La DMDos Board de [Mortaca](https://www.mortaca.com/) incluye el lector SD y no requiere soldadura. Enlaces de compra actualizados centralizados en [dmdos.net](https://www.dmdos.net/). Ver [Montaje del panel DMD](#-montaje-del-panel-dmd) para la guía completa.

---

## 💻 Requisitos de software

- **Windows 10/11** (esta herramienta es exclusiva de Windows)
- **Python 3.8+** — solo si se ejecuta desde el código fuente (el instalador/portable llevan el suyo propio)
- Chrome o Edge — para el instalador de firmware basado en navegador
- Conexión a internet — para el paquete `_defaults`, el paquete de 600 GIFs, y la instalación de los scripts de Recalbox (Modo 9)

---

## 🚀 Inicio rápido

Descarga la versión que prefieras desde la **[página de Releases](https://github.com/shan-aya/RecalBoxDMD/releases)** (los `.exe`/`.msi` compilados no están en el propio repositorio, solo publicados allí):

**Opción A — Instalador de Windows (recomendado)**
```
1. Descarga RecalBoxDMD_Toolkit_Setup.exe desde la página de Releases
2. Ejecútalo — acceso directo en el menú Inicio, icono de escritorio opcional, desinstalador propio
3. Lanza "RecalBoxDMD Toolkit" desde el menú Inicio
```

**Opción B — .exe portable (sin instalación)**
```
1. Descarga RecalBoxDMD-<build>-portable.exe desde la página de Releases
2. Ejecútalo directamente — sin instalación, sin necesidad de Python, un solo archivo
```

**Opción C — .msi (para despliegue por script/directiva de grupo)**
```
1. Descarga el .msi desde la página de Releases
2. msiexec /i "RecalBoxDMD Toolkit-<versión>-win64.msi"   (o doble clic)
```

**Opción D — Desde el código fuente Python**
```
1. Consigue la carpeta tools/
2. Haz doble clic en install_and_run.bat — instala Python (vía winget, si
   falta), Pillow y Markdown, y luego lanza la GUI
   (o manualmente: pip install Pillow Markdown && python run_gui.py)
```

---

## 🖥️ Interfaz gráfica (GUI)

La GUI se abre directamente con **6 pestañas**: **Main · Playlist · Avanzado · Logs · Ajustes · Ayuda**.

### 📌 Pestaña Main

Contiene el **Modo 1** — el que necesita la mayoría de la gente. Apúntalo a tu carpeta de ROMs y haz clic en **Iniciar**; todo lo demás es automático.

Antes de lanzar el proceso, el Modo 1 ofrece dos pasos opcionales previos:

- **Configuración WiFi del DMD** — elige tu red de 2,4 GHz de una lista escaneada en vivo, escribe la contraseña, y la herramienta la **verifica de verdad** (conecta brevemente tu PC a esa red) antes de escribirla en la tarjeta SD. Así el DMD puede unirse a tu WiFi desde su primer arranque, sin pasar nunca por su pantalla de punto de acceso de respaldo. Paso omitible — sin él, el DMD recurre a su propia página de configuración WiFi del primer arranque.
- **IP fija (avanzado, opcional, desmarcada por defecto)** — permite que el DMD use una IP estática en lugar de una asignada por DHCP. Se muestra una advertencia: usa **o bien** una reserva DHCP en tu router **o bien** una IP fija aquí, **nunca las dos a la vez** — combinarlas puede causar problemas de conexión reales. Si marcas esta opción, los campos de puerta de enlace/máscara/DNS se rellenan previamente con la configuración de red de este PC como punto de partida (conviene comprobarlos igualmente).

Después viene el panel **Versión de Recalbox** (10.x / 9.x / legacy — ver [Tutorial de scraping de Recalbox](#-tutorial-de-scraping-de-recalbox)), tu **carpeta de ROMs**, y **Iniciar**.

Cuando termina un proceso, un botón parpadea en esta pestaña ofreciendo una **copia directa a una unidad extraíble detectada** — la copia resiste una interrupción (desconexión/fallo) y puede reintentar solo los archivos que fallaron.

### 🎬 Pestaña Playlist

Crea tus propias rotaciones de GIFs para el modo de espera/attract: mezcla cualquier combinación del **paquete de 600 GIFs** (Modo 11, un clic para descargarlo) y de **tus propios GIFs** (arrastra una carpeta del PC), nombra la playlist, y está lista para seleccionarse como activa. Funciona directamente sobre una tarjeta SD ya preparada, o **en pleno Modo 1**, sobre la carpeta de trabajo antes incluso de que se copie a la tarjeta.

### 🔬 Pestaña Avanzado

Agrupa cada operación individual en 5 categorías, para cuando no quieras ejecutar todo el proceso del Modo 1 — ver la tabla de [Referencia de modos](#-referencia-de-modos) más abajo para el detalle de cada uno:

```
📥 GitHub     — Modo 2 (_defaults), Modo 11 (paquete de 600 GIFs)
🗂️ Gamelist   — Modo 3 (solo extracción), Modo 8 (verificación de imágenes faltantes)
🖼️ Imágenes   — Modo 4 (conversión raw565), Modo 5 (redimensionado 128×32), Modo 10 (imagen de respaldo)
🧮 Cachés     — Modo 6 (caché de juegos), Modo 7 (caché de sistemas)
📜 Scripts    — Modo 9 (instalación de scripts de Recalbox)
```

Los modos 3 y 8 también muestran el panel **Versión de Recalbox** (mismo selector que el Modo 1), ya que ambos trabajan a partir de `gamelist.xml`.

### 📝 Pestaña Logs

Muestra toda la salida del script en tiempo real. Botones de control mientras se ejecuta un modo:

- **⏸️ Pausar** / **▶️ Reanudar** — pausa y reanuda el proceso
- **⏭️ Saltar** — pasa al siguiente paso
- **⏹️ Detener** — detiene el script

### ⚙️ Pestaña Ajustes

- Idioma de la interfaz: 🇫🇷 Francés · 🇬🇧 Inglés · 🇪🇸 Español
- Tema visual (9 apariencias: SNES, Mega Drive, Dreamcast, PlayStation, N64, Neo Geo, Game Boy, Atari 2600, o Aleatorio)
- **Versión de Recalbox** por defecto (10.x / 9.x / legacy) — el mismo valor compartido usado en todas partes (Modo 1/3/8); cambiarlo aquí o en cualquiera de esos modos lo actualiza en todas partes y lo guarda en `RecalBoxDMD_prefs.json`
- Umbral de sistema lento (archivos por encima del cual un sistema se marca como **"L"**, 5000 por defecto) — súbelo si tu tarjeta SD es lo bastante rápida como para no necesitar el sistema de máscara tan pronto

### 📖 Pestaña Ayuda

La estás leyendo ahora mismo — esta página se genera directamente desde este archivo.

---

## 📖 Referencia de modos

| Modo | Categoría | Nombre | Qué hace |
|------|----------|------|---------------|
| **1** | *(pestaña Main)* | **AUTO — todo** | Configuración WiFi/IP → detección de versión de Recalbox → extracción de gamelist → conversión raw565 → caché bigrama → descarga de `_defaults` → instalación de scripts de Recalbox → copia a la SD |
| 2 | 📥 GitHub | Descargar `_defaults` | Obtiene las imágenes de respaldo por defecto para cada sistema conocido |
| 11 | 📥 GitHub | Paquete de 600 GIFs | Descarga en un clic de la colección gratuita de GIFs seleccionados (Arcade, Consolas, Ordenadores, Pinball, Halloween, Navidad, Logo, y más) |
| 3 | 🗂️ Gamelist | Solo extracción | Lee `gamelist.xml`, copia el letrero/logo correcto según tu perfil de versión de Recalbox |
| 8 | 🗂️ Gamelist | Verificación de imágenes faltantes | Informa, por sistema/juego, si la imagen esperada existe realmente (carpeta de ROMs / carpeta de trabajo / tarjeta SD) |
| 4 | 🖼️ Imágenes | Conversión raw565 | PNG → `.raw565`, GIF → `.raw565pack` + `.meta` |
| 5 | 🖼️ Imágenes | Redimensionado 128×32 | Redimensiona los PNG a la resolución del panel (formato imagen, sin conversión raw565) |
| 10 | 🖼️ Imágenes | Imagen de respaldo | Define/genera la imagen por defecto global mostrada cuando nada más coincide |
| 6 | 🧮 Cachés | Caché de juegos | Construye `games_cache.bin` (índice bigrama, 703 entradas por sistema) |
| 7 | 🧮 Cachés | Caché de sistemas | Construye `systems_cache.dat` (índice de sistemas + indicadores lento/rápido **"L"/"N"**) |
| 9 | 📜 Scripts | Instalar scripts de Recalbox | Copia el puente de letreros + los scripts de [Hi-Score/Info del juego/Logros/Challenge](#-overlays-en-partida--hi-score-info-del-juego-logros-y-rb-challenge), además de los scripts manuales de recuperación WiFi/config web/reinicio/brillo, al recurso compartido de red de Recalbox — limpiando de paso cualquier nombre de script antiguo de una instalación previa |

> **📎 Copia a la tarjeta SD**: no es un modo separado — después de cualquier proceso (Modo 1, 3, 4, 5...), un botón parpadea en la pestaña Main ofreciendo copiar a una unidad extraíble detectada.

---

## ✅ Flujo de trabajo recomendado

```
1. 🔄 Escrapea tus juegos en Recalbox (ver el Tutorial de scraping abajo,
   según tu versión de Recalbox)
   ↓
2. 📂 Lanza RecalBoxDMD Toolkit → pestaña Main
   ↓
3. 📶 (Opcional) Configura el WiFi del DMD, y una IP fija si quieres una
   ↓
4. 🎛️ Elige tu "Versión de Recalbox" (10.x / 9.x / legacy)
      ¿Necesitas ayuda? Haz clic en "¿Cómo hacer scraping?"
      ¿Carpeta ya escrapeada con una config antigua? "Limpiar carpetas antes de escrapear"
   ↓
5. 🗂️ Elige tu carpeta de ROMs (ej. D:\Recalbox\share\roms)
   ↓
6. 🟢 Haz clic en Iniciar (Modo 1) y déjalo correr
   ↓
7. ✅ ¡Listo! La carpeta de trabajo está preparada
   ↓
8. 💾 Inserta tu tarjeta SD — el botón parpadeante ofrece copiarla
      (opcional: Modo 8 para comprobar que no falta nada)
   ↓
9. 🎮 ¡Inserta la tarjeta SD en el DMD y enciéndelo!
```

---

## 🎨 Tutorial de scraping de Recalbox

Para que el panel muestre el arte correcto, primero hay que **escrapear** tus ROMs en Recalbox con la pestaña Scraper configurada correctamente **para tu versión**. El botón **"¿Cómo hacer scraping?"** de las pestañas Main/Avanzado muestra capturas de pantalla anotadas para esto; resumido aquí:

### Recalbox 10.x (campo de logo dedicado)

1. Pon **"SELECT LOGO TYPE"** en **"CLEAR"**
2. NO pongas ese mismo valor en **"Select image type"** — ese campo es para el visual principal (captura, carátula...), no para el logo
3. Ejecuta el scraping

Se guarda en `media/wheels/`, referenciado por la etiqueta `<logo>`. → elige el perfil **10.x**.

### Recalbox 9.x (sin campo de logo dedicado)

1. Pon **"Select thumbnail type"** en **"MARQUEE"**
2. NO pongas ese mismo valor en **"Select image type"**
3. Ejecuta el scraping

Se guarda en `media/thumbnails/`, referenciado por la etiqueta `<thumbnail>`. → elige el perfil **9.x** (recomendado).

### Perfil "legacy"

Si tu configuración escrapea el logo/letrero vía **"Select image type"** (puesto en **"Clear Logo"** o **"Marquee"**), los archivos van a `media/images/`, referenciados por la etiqueta `<image>`. → elige el perfil **legacy**.

> **💡 Consejos**: Screenscraper.fr o TheGamesDB dan los mejores resultados. ¿Ya escrapeado con otra configuración? Usa primero **"Limpiar carpetas antes de escrapear"**. ¿No estás seguro de lo que realmente obtuviste? Ejecuta el **Modo 8**.

---

## 🔧 Montaje del panel DMD

El montaje del hardware (2 paneles + DMDos Board + ESP32 + microSD) es rápido y no requiere soldadura:

1. **Une los dos paneles** con las piezas de unión que vienen con la DMDos Board (cualquier tornillo M3 sirve).
2. **Coloca la DMDos Board** en el conector de **entrada** (no salida) — mantén la orientación de los componentes traseros igual en ambas mitades.
3. **Cablea la alimentación** (rojo/negro según la serigrafía) *antes* de colocar el ESP32 encima, y conecta los dos paneles con el cable plano incluido.
4. **Inserta la tarjeta SD** que preparaste con esta herramienta, conecta el ESP32 ya flasheado, alimenta todo por el puerto USB‑C.

> ⚠️ El sitio **DMDos** ([dmdos.net](https://www.dmdos.net/)) ofrece su propio firmware separado. Reutiliza solo su **hardware y guía de montaje** — el firmware y el contenido de la tarjeta SD deben venir de esta herramienta, no de DMDos.

Guía ilustrada completa con fotos: [dmdos.net → Montaje/Assembly](https://www.dmdos.net/#montaje). Marco imprimible en 3D por Janibol ([Retromojones](https://www.youtube.com/@retromojones)) en [Thingiverse](https://www.thingiverse.com/thing:6704880).

---

## ⚡ Firmware ESP32

### 🔧 Compilar desde el código fuente

1. Abre `RecalBox_DMD.ino` en el **Arduino IDE**.
2. Instala estas bibliotecas (Sketch → Incluir Librería → Administrar Librerías):

| Biblioteca | Función |
|---------|---------|
| ESP32-HUB75-MatrixPanel-I2S-DMA | Control DMA del panel LED |
| AnimatedGIF | Decodificación de GIF (ruta de respaldo) |
| pngle | Decodificación de PNG (ruta de respaldo, incluida) |
| WiFiManager | Configuración WiFi |
| Adafruit GFX Library | Renderizado de texto/formas |
| PubSubClient | MQTT (mantenido inactivo — UDP es el transporte por defecto) |
| ArduinoJson | (De)serialización de config y página web |

3. Herramientas → Placa: **ESP32 Dev Module**, tamaño de flash **4 MB**, esquema de partición **Huge APP**.
4. Selecciona el puerto COM correcto, luego **Subir**.

### ⚙️ Configuración (config.ini)

Nunca necesitas escribir este archivo a mano — el Modo 1 lo hace por ti, y la página web del DMD te permite editar cada valor en vivo después. Como referencia:

```ini
# Pantalla
brightness=40                 # brillo del panel 0-100 %

# Playlist
playlist=RecalBox_intros.txt  # reproducida desde /playlist
random=1                      # 0 = secuencial, 1 = aleatorio

# WiFi
wifi_enabled=1
wifi_ssid=miwifi
wifi_password=micontraseña
wifi_static_enabled=1         # 1 = usa los campos de IP fija de abajo en vez de DHCP
wifi_static_ip=192.168.1.240
wifi_gateway=192.168.1.1
wifi_subnet=255.255.255.0

# Enlace con Recalbox (UDP)
recalbox_ip=192.168.1.104     # IP fija de tu Recalbox

# Reloj (temas de reloj retro)
[CLOCK]
CLOCK_ENABLED=1
CLOCK_THEME=-1                # -1=aleatorio, 0=Mario ... 9=Level 1-1
CLOCK_INTERVAL=5              # numero de GIFs antes de mostrar el reloj
CLOCK_DURATION=60             # segundos que el reloj permanece en pantalla
```

### 🌐 Instalador Web

> [👉 Instalar desde el instalador Web por navegador](https://shan-aya.github.io/RecalBoxDMD/)

Chrome o Edge, conecta el ESP32 por USB, haz clic en **Install**, elige el puerto COM — cerca de un minuto, sin necesidad de Arduino IDE. Marca **"Erase device"** en una primera instalación, o al venir de otro firmware.

### 📡 Enlace UDP y Telnet

```
Recalbox → marquee[...].sh → UDP → ESP32 → Panel LED

1. Lanzas "King of Fighters '98"
2. El script bash detecta el evento → envía "mame/kof98"
3. El ESP32 busca, en orden:
   a. /systems/mame/kof98.raw565 (o .raw565pack)   ← instantaneo
   b. indice bigrama de games_cache.bin              ← acelerado
   c. /systems/_defaults/mame.raw565                 ← respaldo de sistema
   d. /systems/_defaults/default.raw565               ← respaldo global
4. Se muestra en menos de 15 ms
```

Instala el script con el **Modo 9**, o copia `marquee[...].sh` manualmente en `/recalbox/share/userscripts/`.

**Control manual desde el menú de Recalbox** — el Modo 9 también instala scripts que puedes activar a mano (**START → Ajustes avanzados → Scripts de usuario**): WiFi Recovery DMD (vuelve al modo punto de acceso de recuperación), Config Web DMD (abre la página de configuración web), Reboot DMD, y brillo ±10%.

Hay una consola **Telnet** integrada para depuración en el dispositivo: `telnet <ip-esp32>` y luego `help`.

---

## 🏆 Overlays en partida — Hi-Score, info del juego, logros y RB Challenge

Mientras un juego está realmente en marcha (nunca durante el modo de espera/playlist), el panel puede alternar automáticamente el letrero con hasta **cuatro** overlays comunitarios — siempre autolimitados, volviendo al letrero con un temporizador que vive enteramente en el propio DMD:

- 🏆 **Hi-Score (MAME/FBNeo)** — decodifica el archivo de puntuación guardado por el propio emulador contra un manifiesto comunitario (miles de juegos cubiertos) y muestra la clasificación real.
- ℹ️ **Info del juego** — descripción, género, desarrollador y año, directamente desde tu `gamelist.xml` existente.
- 🎖️ **RetroAchievements** — aparece en el momento en que desbloqueas un logro en plena partida.
- 📅 **RB Challenge** — la clasificación oficial del Challenge comunitario mensual de Recalbox.

**Cero configuración del lado del DMD.** Instala los scripts de Recalbox una vez — **Modo 9** (o la instalación automática integrada en el **Modo 1**) — y cada overlay empieza a funcionar por sí solo para cualquier juego/sistema que tenga datos que mostrar.

---

## 🗂️ El formato raw565 en detalle

### 📄 .raw565 (imagen fija desde un PNG)

```
Tamaño: 128 × 32 × 2 = 8192 bytes exactamente
Formato: RGB565 en bruto (16 bits por píxel)

Lectura del ESP32:
  f.read(buffer, 8192);
  drawRGBBitmap(0, 0, buffer, 128, 32);
  // 1 operacion SD + 1 dibujo → 5 ms
```

### 🎞️ .raw565pack + .meta (GIF animado)

```
[Nombre].raw565pack             [Nombre].meta
├── Fotograma 0 → 8192 bytes     ├── delay_0 → 2 bytes (uint16, ms)
├── Fotograma 1 → 8192 bytes     ├── delay_1 → 2 bytes
└── ...                          └── ...
```

Una apertura SD + un seek por fotograma, cero decodificación de GIF en el dispositivo.

### ⚡ Caché bigrama — indexación acelerada

`games_cache.bin` indexa cada sistema por prefijo de 2 letras (703 entradas, `#`, `A`, `AA`, `AB`... `ZZ`) — una búsqueda salta directamente al fragmento correcto de la caché en lugar de recorrer una carpeta con decenas de miles de archivos.

### 🎭 Sistema de máscara

Los sistemas marcados como **"L"** (por encima del umbral configurable, pestaña Ajustes — 5000 por defecto) muestran **de inmediato** su imagen por defecto en caché mientras una tarea en segundo plano decodifica y sustituye por la imagen real. El panel nunca se queda en negro.

---

## 📁 Estructura de la tarjeta SD

```
📁 TARJETA SD (FAT32)
├── config.ini
├── systems/
│   ├── <sistema>/
│   │   ├── <juego>.raw565           ← letrero fijo
│   │   ├── <juego>.raw565pack       ← letrero animado (fotogramas)
│   │   └── <juego>.meta             ← letrero animado (tiempos)
│   └── _defaults/
│       ├── default.raw565           ← respaldo global
│       └── <sistema>.raw565         ← respaldo por sistema
├── gifs/                            ← playlists de attract-mode (aquí cae el paquete de 600 GIFs)
│   ├── Arcade/  Consoles/  Computers/  Pinball_Short/  Pinball_Story/
│   └── Halloween/  XMAS/  Logo/  Other/ ...
├── playlists/
│   └── <nombre_playlist>.txt
├── games_cache.bin                  ← índice bigrama
└── systems_cache.dat                ← índice de sistemas + indicadores L/N
```

---

## 🔧 Solución de problemas

Para congelamientos, corrupción de pantalla, bucles en la configuración WiFi, o un DMD que parece desincronizado de Recalbox, consulta primero la página dedicada **[FAQ y solución de problemas](FAQ.es.md)** en GitHub — cubre las causas más comunes (alimentación USB, calidad de la tarjeta microSD, revisión del chip ESP32) con más detalle del que cabe aquí.

| Problema | Solución |
|---|---|
| "Pillow no está instalado" | Se instala automáticamente en el primer arranque; si falla: `pip install Pillow` |
| "API de GitHub inaccesible" | Las descargas de `_defaults`/paquete de 600 GIFs necesitan conexión a internet; reintenta más tarde (límite de tasa) |
| No se detecta ninguna unidad extraíble | Inserta/vuelve a comprobar que la tarjeta SD es visible en el Explorador de Windows |
| El ESP32 no muestra nada | Comprueba la alimentación (5V 4A mín.), `config.ini` en la raíz de la SD, el cableado HUB75; prueba Telnet `help` |
| ESP32 no detectado (sin puerto COM) | Instala los drivers USB: CP2102 (Silicon Labs) o CH340/CH341 |
| Visualización lenta / pantalla negra entre juegos | Confirma que ejecutaste el **Modo 1**; comprueba que el sistema esté marcado como `L` en `systems_cache.dat`; sube el umbral de sistema lento (pestaña Ajustes) si tu tarjeta SD es rápida |
| Aparece la imagen equivocada (carátula en vez de logo) | Comprueba el perfil de **Versión de Recalbox** y usa **"¿Cómo hacer scraping?"**; ejecuta el **Modo 8** para verificar qué hay realmente presente |
| No se puede configurar el WiFi durante el Modo 1 | Paso opcional — sáltalo y usa la página WiFi del primer arranque del DMD en su lugar |
| Problemas de conexión tras configurar una IP fija | Asegúrate de no usar **a la vez** una reserva DHCP para el DMD en tu router — usa solo uno de los dos métodos |

---

## 📁 Archivos generados por el script

| Archivo | Formato | Función |
|------|--------|---------|
| `systems/.../*.raw565` | 8192 bytes RGB565 | PNG convertido (visualización 5 ms) |
| `systems/.../*.raw565pack` | Fotogramas concatenados | GIF animado convertido |
| `systems/.../*.meta` | uint16[] × núm. de fotogramas | Tiempos de los fotogramas del GIF |
| `systems/_defaults/*.raw565` | 8192 bytes | Imágenes de respaldo por sistema |
| `games_cache.bin` | Índice bigrama, 703 entradas/sistema | Caché de juegos (búsqueda acelerada) |
| `systems_cache.dat` | Texto | Índice de sistemas + indicadores `L`/`N` |
| `images_manquantes.txt` | Texto | Lista de imágenes faltantes (Modo 1/2/3) |
| `reports/mode8_report_*.txt` | Texto | Informe del Modo 8 (verificación gamelist ↔ imágenes) |
| `config.ini` | Texto | Configuración del DMD (WiFi, playlist, brillo...) |

---

## 🤝 Créditos

- **Proyecto original RetroBoxLED**: [Jamyz](https://github.com/Jamyz/RetroBoxLED) — la base del firmware ESP32 y la idea
- **RawEdition**: **Shan_ayA** — formato raw565, caché bigrama, sistema de máscara, toolkit de PC, temas de reloj, gestión de versiones de Recalbox, vista previa en vivo por web
- **Inspiración**: [RetroPixelLED](https://github.com/fjgordillo86/RetroPixelLED) de fjgordillo86
- **Paquete de 600 GIFs**: **eLLuiGi** / [RpiTeaM](https://rpiteam.carrd.co/) — muestra gratuita de su colección de GIFs retro
- **Manifiesto Hi-Score**: formato comunitario **hi2txt-xml**, construido en torno al propio proyecto `hiscore.dat` de MAME
- **Hardware y guía de montaje**: [Mortaca — DMDos Board](https://www.mortaca.com/) / [dmdos.net](https://www.dmdos.net/)
- **Marco 3D**: Janibol — [Retromojones](https://www.youtube.com/@retromojones)
- **Comunidad**: [Recalbox](https://www.recalbox.com/)

Página completa del proyecto, capturas de pantalla e historial de versiones: [github.com/shan-aya/RecalBoxDMD](https://github.com/shan-aya/RecalBoxDMD)

---

> **RecalBoxDMD Toolkit** — Recalbox + un letrero LED real, ¡instantáneo incluso con 30.000 juegos de MAME! 🎮⚡
