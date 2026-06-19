# 🎮 RecalBoxDMD Toolkit v3.4 — Ayuda completa

> **RecalBoxDMD** = Recalbox + panel LED DMD 128x32 para su arcade!

---

## 📋 Tabla de contenidos

1. [Presentación](#-presentación)
2. [Requisitos de hardware](#-requisitos-de-hardware)
3. [Requisitos de software](#-requisitos-de-software)
4. [Instalación rápida](#-instalación-rápida)
5. [Interfaz gráfica (GUI)](#️-interfaz-gráfica-gui)
   - [Pestaña Main](#-pestaña-main)
   - [Pestaña Avanzado](#-pestaña-avanzado)
   - [Pestaña Logs](#-pestaña-logs)
   - [Pestaña Configuración](#-pestaña-configuración)
6. [Detalle de los modos](#-detalle-de-los-modos)
7. [Flujo de trabajo recomendado](#-flujo-de-trabajo-recomendado)
8. [Tutorial de scraping en Recalbox](#-tutorial-de-scraping-en-recalbox)
9. [Firmware ESP32 — Raw565 Edition](#-firmware-esp32--raw565-edition)
   - [¿Por qué raw565?](#-por-qué-raw565)
   - [Compilación con Arduino IDE](#-compilación-con-arduino-ide)
   - [Configuración (config.ini)](#️-configuración-configini)
   - [Instalación vía navegador (WebInstaller)](#-instalación-vía-navegador)
10. [El formato raw565 en detalle](#-el-formato-raw565-en-detalle)
    - [.raw565 (imagen fija PNG)](#-raw565-imagen-fija-png)
    - [.raw565pack + .meta (GIF animado)](#-raw565pack--meta-gif-animado)
    - [Caché bigrama — indexación acelerada](#-caché-bigrama--indexación-acelerada)
    - [Sistema de máscara (mask)](#-sistema-de-máscara-mask)
11. [Estructura de la tarjeta SD](#-estructura-de-la-tarjeta-sd)
12. [Solución de problemas](#-solución-de-problemas)
13. [Créditos](#-créditos)

---

## 🎯 Presentación

**RecalBoxDMD Toolkit** es un fork optimizado de **RetroBoxLED** de Jamyz, diseñado específicamente para resolver los problemas de **lentitud de visualización** en sistemas con muchos archivos (fullset MAME, FBNeo, etc.).

### 🔥 El problema resuelto

La versión original de Jamyz leía los archivos **PNG y GIF** directamente desde la tarjeta SD. En sistemas como **MAME fullset** (30,000+ juegos), este enfoque causaba:

```
❌ Problema original:
   - Abrir la carpeta /systems/mame/ con 30,000 archivos
   - El ESP32 lista todos los archivos → se congela por segundos
   - Decodificación PNG en el ESP32 → lenta, memoria insuficiente
   - Entre cada juego: pantalla negra o latencia visible
```

**La « Raw565 Edition »** pre-convierte todas las imágenes en el PC (mediante el script Python) a un **formato RGB565 bruto** directamente mostrable por el ESP32:

```
✅ Solución raw565:
   1. El script convierte cada PNG → archivo .raw565 (8192 bytes, listo para usar)
   2. Cada GIF → archivo .raw565pack (tramas concatenadas) + .meta (timings)
   3. El ESP32 lee el raw565 y lo envía directamente al panel LED
   4. Sin decodificación, sin latencia: visualización casi instantánea
```

| Métrica | Original PNG/GIF | Raw565 Edition |
|---------|-----------------|----------------|
| Tiempo de visualización | 500ms – 3s+ | **5–15ms** |
| RAM necesaria | 50-100 KB | **8 KB** |
| Latencia MAME fullset | Congelación 5-10s | **0** |
| Compatibilidad | Todos los sistemas | **Sistemas marcados « L »** |

### El sistema de máscara (mask)

Para los sistemas muy grandes marcados **« L »** (Large / Lentos, como MAME o FBNeo), el firmware utiliza un mecanismo de máscara:

1. El script detecta sistemas con >800 archivos individuales
2. Los marca como **« L »** en `systems_cache.dat`
3. Cuando el ESP32 recibe un juego de este sistema:
   - Muestra inmediatamente el **default raw565** del sistema en caché RAM
   - En paralelo, una tarea asíncrona decodifica el PNG específico
   - Cuando termina, la imagen específica reemplaza la máscara
4. Resultado: **el usuario nunca ve una pantalla negra**, el panel permanece activo

> **💡 Resultado**: No más segundos de espera entre juegos. La visualización es casi instantánea incluso con un fullset MAME de 30,000 archivos.

### 🔄 Cómo funciona todo junto

```
┌─────────────────────────────────────────────────────────────┐
│                     RECALBOX                                 │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ Lanza un juego → MQTT envía "mame/kof98" al ESP32   │   │
│  └──────────────┬───────────────────────────────────────┘   │
│                 │                                           │
│          Evento MQTT                                       │
│                 ▼                                           │
│  ┌──────────────────────────────────────────────────────┐   │
│  │            ESP32 + Panel LED 128x32                    │   │
│  │                                                       │   │
│  │  Recibe "mame/kof98":                                 │   │
│  │  1. Busca /systems/mame/kof98.raw565 (instantáneo)    │   │
│  │  2. ¿No encontrado? Busca en games_cache.bin          │   │
│  │  3. ¿No encontrado? Muestra _defaults/mame.raw565     │   │
│  │  4. ¿No encontrado? Muestra _defaults/default.raw565  │   │
│  │                                                       │   │
│  │  ⏱️ Tiempo total: 5 a 15 ms en lugar de 500ms-3s!    │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘

         ┌─────────────────────────────────────────────┐
         │     RecalBoxDMD_tool3.4.py (Prepara la SD)   │
         │  Extrae imágenes de los gamelists.xml         │
         │  Convierte PNG → .raw565 (formato RGB565)     │
         │  Convierte GIF → .raw565pack + .meta          │
         │  Construye el caché bigrama (games_cache.bin) │
         │  Marca sistemas lentos (flag "L")             │
         │  Copia todo a la tarjeta SD                   │
         └─────────────────────────────────────────────┘
```

---

## 🧰 Requisitos de hardware

| Componente | Referencia | Precio aprox. |
|------------|-----------|---------------|
| 🧠 Microcontrolador | ESP32 DevKit V1 (38 pines) | ~5€ |
| 🖥️ Panel LED | Matricial RGB HUB75 P4 64x32 o 128x32 | ~15-25€ |
| 💾 Lector SD | Módulo adaptador Micro SD (SPI) | ~2€ |
| 🔌 Placa de conexión | DMDos Board V3 (opcional, simplifica cableado) | ~15€ |
| ⚡ Alimentación | 5V 4A+ (según tamaño del panel) | ~10€ |

> **💡 Consejo**: La DMDos Board de [Mortaca](https://www.mortaca.com/) integra el lector SD y evita la soldadura!

---

## 💻 Requisitos de software

- **Python 3.8+** instalado en su PC
- **Pillow** (se instala automáticamente si falta)
- **Arduino IDE** (solo para recompilar el firmware)
- Navegador Chrome/Edge (para WebInstaller)

---

## 🚀 Instalación rápida

```
1. Descargue los archivos de la carpeta tools/
2. Ejecute RecalBoxDMD_tool3.4.py
3. Siga las instrucciones en pantalla o use la interfaz gráfica
```

---

## 🖥️ Interfaz gráfica (GUI)

La interfaz gráfica se inicia automáticamente. Está organizada en **5 pestañas**:

### 📌 Pestaña Main

> Contiene solo el **Modo 1** — lo esencial para la mayoría de usuarios.

```
┌────────────────────────────────────────────────────────────┐
│ [Main]  [Avanzado]  [Logs]  [Config]  [AYUDA]            │
├────────────────────────────────────────────────────────────┤
│ ┌────────────────────────────────────────────────────────┐ │
│ │ 📋 Modo                                               │ │
│ │                                                       │ │
│ │ ◉ MODO 1 — Extracción + Conversión + Build Cache      │ │
│ │   (TODO en un clic!)                                  │ │
│ │                                                       │ │
│ │ ┌──────────────────────────────────────────────────┐  │ │
│ │ │ 📂 Elegir carpeta ROMs                         │  │ │
│ │ │ D:\Recalbox\share\roms                          │  │ │
│ │ └──────────────────────────────────────────────────┘  │ │
│ │                                                       │ │
│ │ [ 🟢 Iniciar ]                                        │ │
│ │ [ ❌ Salir ]                                          │ │
│ └────────────────────────────────────────────────────────┘ │
│                                                            │
│ ┌────────────────────────────────────────────────────────┐ │
│ │ Progreso: ████████████░░░░░░ 75%                       │ │
│ │ Extrayendo: mame/king_of_fighters.png                  │ │
│ │ ⏸️ Pausa  ▶️ Reanudar  ⏭️ Saltar  ⏹️ Parar             │ │
│ └────────────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────────┘
```

**👉 Modo 1 = el modo completo** que encadena:
1. Extracción de imágenes desde sus `gamelist.xml`
2. Conversión PNG → raw565 (formato RGB565 bruto, 8192 bytes)
3. Conversión GIF → raw565pack + meta (tramas concatenadas)
4. Descarga de `systems/_defaults` desde GitHub
5. Construcción de `games_cache.bin` (índice bigrama)
6. Generación de `systems_cache.dat` (índice sistemas + flag lento/rápido)
7. Detección automática de sistemas grandes (flag `L` para MAME, FBNeo...)

### 🔬 Pestaña Avanzado

> Contiene los **modos 2 a 7** para operaciones específicas.

```
┌────────────────────────────────────────────────────────────┐
│ [Main]  [Avanzado]  [Logs]  [Config]  [AYUDA]            │
├────────────────────────────────────────────────────────────┤
│ ┌────────────────────────────────────────────────────────┐ │
│ │ 📋 Modo                                               │ │
│ │                                                       │ │
│ │ ○ MODO 2 — Descargar _defaults desde GitHub           │ │
│ │ ○ MODO 3 — Extracción Gamelist solamente              │ │
│ │ ○ MODO 4 — Conversión PNG→raw565 + GIF→raw565pack     │ │
│ │ ○ MODO 5 — Conversión 128x32 solamente                │ │
│ │ ○ MODO 6 — Build Games Cache solamente                │ │
│ │ ○ MODO 7 — Generar systems_cache.dat                  │ │
│ │                                                       │ │
│ │ [ 🟢 Iniciar ]                                        │ │
│ └────────────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────────┘
```

### 📝 Pestaña Logs

Muestra toda la salida del script en tiempo real. Útil para seguir el progreso y detectar errores.

Botones de control:
- **⏸️ Pausa** — Pausa el procesamiento
- **▶️ Reanudar** — Reanuda el procesamiento
- **⏭️ Saltar** — Salta al siguiente paso
- **⏹️ Parar** — Detiene el script

### ⚙️ Pestaña Configuración

Permite cambiar el idioma de la interfaz:
- 🇫🇷 Francés
- 🇬🇧 Inglés
- 🇪🇸 Español

### 📖 Pestaña AYUDA

Muestra este README directamente en la interfaz.

---

## 📖 Detalle de los modos

| Modo | Nombre | Acción | Duración est. |
|------|--------|--------|---------------|
| **1** | **TODO** | Extracción + Conversión raw565 + Caché + _defaults | Larga (5-30 min) |
| **2** | Descargar _defaults | Obtiene imágenes de sistemas desde GitHub | Corta (~1 min) |
| **3** | Extracción solamente | Lee gamelist.xml, copia imágenes | Variable |
| **4** | Conversión raw565 | PNG→raw565 + GIF→raw565pack/meta | Media |
| **5** | Conversión 128x32 | Redimensiona PNGs a 128x32 (formato imagen) | Media |
| **6** | Build Games Cache | Genera games_cache.bin (índice bigrama) | Rápida |
| **7** | Generar systems_cache.dat | Índice sistemas + flags lento/rápido | Rápida |
| **8** | **Copiar SD** | Copia carpeta de trabajo a la tarjeta SD | Rápida |

---

## ✅ Flujo de trabajo recomendado

```
1. 🔄 Haga scrape de sus juegos en Recalbox (logo recortado o marquee)
   ↓
2. 📂 Ejecute RecalBoxDMD_tool3.4.py → pestaña Main
   ↓
3. 🗂️ Elija su carpeta de ROMs (ej: D:\Recalbox\share\roms)
   ↓
4. 🟢 Haga clic en Iniciar (Modo 1)
   ↓
5. ⏳ Deje que el script trabaje. Hará:
      - Extraer imágenes de gamelist.xml
      - Convertir PNG → .raw565 (formato RGB565 rápido)
      - Convertir GIF → .raw565pack + .meta
      - Descargar _defaults desde GitHub
      - Construir el caché bigrama (indexación de juegos)
      - Marcar sistemas grandes (flag "L" para MAME, FBNeo)
   ↓
6. ✅ Terminado! La carpeta sd_card/ está lista en %TEMP%/RecalBoxDMD
   ↓
7. 💾 Inserte su tarjeta SD, el script ofrece copiar
   ↓
8. 🎮 Inserte la SD en el ESP32 y encienda!
      - Visualización casi instantánea incluso con 30,000 juegos MAME
```

---

## 🎨 Tutorial de scraping en Recalbox

Para que el panel LED muestre los juegos correctos, primero debe hacer **scrape** de sus ROMs en Recalbox:

1. Desde Recalbox, vaya a **Configuración → Scraper**
2. Elija tipo de imagen **LOGO RECORTADO** o **MARQUEE**
3. Haga scrape de todos sus sistemas
4. Las imágenes se guardarán en `/recalbox/share/roms/...` con `gamelist.xml`

```
📁 /recalbox/share/roms/
   ├── 📁 mame/
   │   ├── 📄 gamelist.xml
   │   ├── 📁 images/
   │   │   ├── 🖼️ kof98.png       ← Imagen scapeada
   │   │   ├── 🖼️ sf2.png
   │   │   └── ...
   │   └── ...
   ├── 📁 snes/
   │   ├── 📄 gamelist.xml
   │   ├── 📁 images/
   │   │   ├── 🖼️ supermetroid.png
   │   │   └── ...
   └── ...
```

> **💡 Consejo**: Use Screenscraper.fr o TheGamesDB para mejores resultados!

---

## ⚡ Firmware ESP32 — Raw565 Edition

### 💡 ¿Por qué raw565?

El formato **raw565** es el elemento clave de esta versión:

```
┌────────────────────────────────────────────────────────────────┐
│                       TARJETA SD                                │
│                                                                │
│  PNG (50 KB)  →  Conversión PC →  .raw565 (8,192 bytes)       │
│  GIF (200 KB) →  Conversión PC →  .raw565pack + .meta          │
│                                                                │
│  El ESP32 solo necesita:                                       │
│    1. Abrir el archivo .raw565 (8 KB)                          │
│    2. Leer 8192 bytes en RAM                                   │
│    3. drawRGBBitmap() → visualización directa                  │
│                                                                │
│  🚀 Tiempo total: 5-15 ms (vs 500ms-3s para PNG/GIF)          │
└────────────────────────────────────────────────────────────────┘
```

### 🔧 Compilación con Arduino IDE

1. Abra `RecalBox_DMD.ino` en Arduino IDE
2. Instale las siguientes bibliotecas:

| Biblioteca | Enlace | Propósito |
|------------|--------|-----------|
| ESP32-HUB75-MatrixPanel-I2S-DMA | [GitHub](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA) | Control DMA del panel LED |
| AnimatedGIF | [GitHub](https://github.com/bitbank2/AnimatedGIF) | Decodificación GIF (fallback) |
| pngle | [GitHub](https://github.com/kikuchan/pngle) | Decodificación PNG (fallback) |
| WiFiManager | [GitHub](https://github.com/tzapu/WiFiManager) | Configuración WiFi |
| Adafruit GFX Library | [GitHub](https://github.com/adafruit/Adafruit-GFX-Library) | Visualización texto y formas |
| ArduinoJson | [GitHub](https://github.com/bblanchon/ArduinoJson) | Configuración y web |

3. Seleccione **ESP32 Dev Module** en **Herramientas → Placa**
4. Conecte el ESP32, seleccione el puerto COM correcto
5. Haga clic en **Subir** (⚡)

### ⚙️ Configuración (config.ini)

Coloque este archivo en la raíz de la tarjeta SD:

```ini
# Info
info=0                     # 0 = sin info al inicio

# Playlist
playlist=TODO.txt          # Playlist a leer en /playlist
random=1                   # 0 = orden, 1 = aleatorio

# WiFi
wifi_enabled=1
wifi_ssid=miwifi
wifi_password=miclave
wifi_static_enabled=1
wifi_static_ip=192.168.1.240
wifi_gateway=192.168.1.1
wifi_subnet=255.255.255.0

# MQTT
recalbox_ip=192.168.1.104   # IP fija de su Recalbox
image_folder=logo_detoure   # o "marquee"
```

### 🌐 Instalación vía navegador

> [👉 Instalar desde la página WebInstaller](https://jamyz.github.io/RetroBoxLED/)

1. Use Chrome o Edge
2. Conecte el ESP32 por USB
3. Haga clic en **Install** y seleccione el puerto COM
4. **Importante**: Marque **« Erase device »** para borrar completamente la memoria!

### 📡 MQTT — El cerebro del sistema

MQTT permite a Recalbox comunicarse con el ESP32 en tiempo real:

```
Recalbox → marquee[rungame,...].sh → MQTT → ESP32 → Panel LED

1. Lanza "King of Fighters '98"
2. El script bash detecta el evento → envía "mame/kof98" vía MQTT
3. ESP32 recibe → busca en orden:
   a. /systems/mame/kof98.raw565      ← instantáneo (5 ms)
   b. games_cache.bin → bigrama       ← indexación acelerada
   c. /systems/_defaults/mame.raw565  ← fallback sistema
   d. /systems/_defaults/default.raw565 ← fallback global
4. Mostrado en menos de 15 ms!
```

Coloque `marquee[rungame,endgame,systembrowsing,...].sh` en:
```
/recalbox/share/userscripts/
```

### 🔌 Telnet

El firmware incluye un terminal Telnet para probar el ESP32. Conéctese con:
```
telnet 192.168.1.240
```
Escriba `help` para la lista de comandos.

---

## 🗂️ El formato raw565 en detalle

### 📄 .raw565 (imagen fija PNG)

```
Tamaño: 128 × 32 × 2 = 8,192 bytes exactamente
Formato: RGB565 bruto (16 bits por pixel)
         Bit R: bits 15-11 (5 bits)
         Bit G: bits 10-5  (6 bits)
         Bit B: bits 4-0   (5 bits)

Lectura ESP32:
  f.read(buffer, 8192);
  drawRGBBitmap(0, 0, buffer, 128, 32);
  // 1 operación SD + 1 draw → 5 ms
```

### 🎞️ .raw565pack + .meta (GIF animado)

El archivo **`.raw565pack`** concatena todas las tramas del GIF:

```
[NombreArchivo].raw565pack
├── Frame 0  →  8,192 bytes (raw565)
├── Frame 1  →  8,192 bytes
├── Frame 2  →  8,192 bytes
├── ...
└── Frame N  →  8,192 bytes

[NombreArchivo].meta
├── delay_0  →  2 bytes (uint16, milisegundos)
├── delay_1  →  2 bytes
├── delay_2  →  2 bytes
└── ...
```

**Ventajas**:
- **1 sola apertura SD** para leer una trama (seek + bulk read)
- **Sin decodificación GIF** en el ESP32
- Lectura acelerada mediante caché RAM de delays (`.meta` cargado una vez)
- Control de velocidad integrado (`GIF_RAW_PACK_SPEED_PERCENT`)

### ⚡ Caché bigrama — indexación acelerada

Para evitar listar carpetas SD (muy lento con 30,000 archivos), el script construye un **índice bigrama**:

```
games_cache.bin
├── [Cabecera] → número de sistemas indexados
├── [Sistema 0] → nombre + offset tabla bigrama
├── [Sistema 1] → nombre + offset
└── ...

Tabla bigrama (703 entradas = 2812 bytes por sistema)
├── Índice 0   → '#'  (dígitos, símbolos)
├── Índice 1   → 'A'  (juegos que empiezan con A)
├── Índice 2   → 'AA' (juegos que empiezan con AA)
├── Índice 3   → 'AB'
├── ...
└── Índice 702 → 'ZZ'

Cada entrada = offset en el archivo de caché hacia la lista de juegos
```

Cuando el ESP32 busca `kof98`:
1. Calcula índice bigrama: `K` → índice `11*27+1=298` (prefijo `KO`)
2. Carga el segmento correspondiente en RAM (lectura bulk)
3. Escanea nombres de juegos en este segmento
4. Encuentra `kof98` y su tipo (`p` para raw565) → **instantáneo**

### 🎭 Sistema de máscara (mask)

Para sistemas muy grandes (MAME, FBNeo, etc.), el mecanismo de máscara evita pantallas negras:

```
┌─────────────────────────────────────────────────────────────┐
│  Paso 1: Script Python analiza el sistema                  │
│  → Cuenta archivos individuales                             │
│  → >800 archivos? Flag "L" en systems_cache.dat            │
│                                                             │
│  Paso 2: ESP32 recibe "mame/kof98"                        │
│  → Sistema MAME marcado "L"?                               │
│    SÍ → Muestra INMEDIATAMENTE el default raw565            │
│           (precargado en RAM, 0 ms de espera)              │
│                                                                   │
│  Paso 3: En paralelo, tarea asíncrona:                    │
│  → Busca el .raw565 específico                             │
│  → ¿No encontrado? Decodifica el PNG en segundo plano     │
│  → Cuando esté listo: reemplaza la máscara por la imagen  │
│                                                             │
│  Resultado: El usuario nunca ve una pantalla negra!        │
└─────────────────────────────────────────────────────────────┘
```

---

## 📁 Estructura de la tarjeta SD

```
📁 SD CARD (FAT32)
   │
   ├── 📄 config.ini              ← Configuración ESP32
   │
   ├── 📁 systems/
   │   ├── 📁 mame/
   │   │   ├── 🖼️ kof98.raw565          ← PNG convertido (8 KB)
   │   │   ├── 🖼️ sf2.raw565
   │   │   ├── 🖼️ intro.raw565pack      ← GIF convertido (tramas)
   │   │   ├── 🖼️ intro.meta            ← Timings GIF
   │   │   └── ...
   │   ├── 📁 snes/
   │   │   ├── 🖼️ supermetroid.raw565
   │   │   └── ...
   │   └── 📁 _defaults/
   │       ├── 🖼️ default.raw565        ← Imagen de reemplazo global
   │       ├── 🖼️ mame.raw565           ← Fallback para sistema MAME
   │       ├── 🖼️ snes.raw565
   │       └── ...
   │
   ├── 📁 gifs/                   ← Playlists de animaciones
   │   ├── 📁 Arcade/
   │   ├── 📁 BEST_OF_TOP_30/
   │   └── 📁 Pixel_Art/
   │
   ├── 📁 playlists/
   │   ├── 📄 Arcade.txt
   │   └── 📄 TODO.txt
   │
   ├── 📄 games_cache.bin         ← Caché bigrama (indexación juegos)
   └── 📄 systems_cache.dat       ← Índice sistemas + flags lento/rápido
```

---

## 🔧 Solución de problemas

### ❌ Problema: "Pillow no está instalado"
- El script intenta instalarlo automáticamente
- Si falla, ejecute manualmente: `pip install Pillow`

### ❌ Problema: "API de GitHub inaccesible"
- Verifique su conexión a internet
- Los modos _defaults requieren conexión

### ❌ Problema: "No se detectó ninguna unidad extraíble"
- Inserte su tarjeta SD
- Verifique que sea detectada por Windows (Explorador → Este PC)
- Vuelva a intentar

### ❌ Problema: El ESP32 no muestra nada
- Verifique la alimentación (5V 4A mínimo)
- Verifique que config.ini esté en la raíz de la SD
- Verifique el cableado HUB75 (pinout ESP32)
- Pruebe Telnet: escriba `help` para probar

### ❌ Problema: "ESP32 no detectado" (puerto COM faltante)
Instale los drivers USB:

| Chip USB | Controladores |
|----------|--------------|
| CP2102 | [Silicon Labs](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) |
| CH340/CH341 | [SparkFun](https://learn.sparkfun.com/tutorials/how-to-install-ch340-drivers/all) |

### ❌ Problema: Visualización lenta o pantalla negra entre juegos
- Asegúrese de usar el **Modo 1** (conversión raw565 completa)
- Los sistemas grandes (MAME, FBNeo) deben estar marcados **« L »** — verifique en `systems_cache.dat`
- Si el flag es `N` (rápido) pero el sistema es lento, aumente `SPRITE_LIMIT` en el .ino
- Verifique que los archivos `.raw565` existan en `systems/<sys>/`

### ❌ Problema: La imagen no se muestra en el panel
- Asegúrese de que la imagen esté en formato 128x32
- Los PNG deben estar en RGB (no RGBA) para la conversión
- Los raw565 tienen prioridad sobre los raw565pack

---

## 📁 Archivos generados por el script

| Archivo | Formato | Propósito |
|---------|--------|-----------|
| `sd_card/systems/.../*.raw565` | 8192 bytes RGB565 | PNG convertido (visualización 5ms) |
| `sd_card/systems/.../*.raw565pack` | Tramas concatenadas | GIF animado convertido |
| `sd_card/systems/.../*.meta` | uint16[] × nb_frames | Timings de tramas GIF |
| `sd_card/systems/_defaults/*.raw565` | 8192 bytes | Imágenes de reemplazo por sistema |
| `sd_card/games_cache.bin` | Índice bigrama 703 entradas | Caché de juegos (búsqueda acelerada) |
| `sd_card/systems_cache.dat` | Texto (val + nombre + flag) | Índice sistemas + flags L/N |
| `missing_images.log` | Texto | Lista de imágenes faltantes |

---

## 🤝 Créditos

- **Proyecto original RetroBoxLED**: [Jamyz](https://github.com/Jamyz/RetroBoxLED) — el firmware ESP32 y la idea base
- **Raw565 Edition**: Shan_ayA — optimización del formato raw565, caché bigrama, sistema de máscara, interfaz gráfica
- **Inspiración**: [RetroPixelLED](https://github.com/fjgordillo86/RetroPixelLED) por fjgordillo86
- **Comunidad**: [Recalbox](https://www.recalbox.com/)
- **Hardware**: [Mortaca - DMDos Board](https://www.mortaca.com/)

---

## ☕ Apoyo

Si este proyecto le es útil:
👉 [Donar vía PayPal](https://www.paypal.com/paypalme/felysaya)

---

> **RecalBoxDMD Raw565 Edition** = Recalbox + Panel DMD LED + Visualización rapido incluso con 30,000 juegos MAME! 🎮⚡
