# 🎮 RecalBoxDMD Toolkit v3.5 — Ayuda completa

> **RecalBoxDMD** = Recalbox + panel LED DMD 128x32 para su arcade!

---

## 📋 Tabla de contenidos

1. [Presentación](#-presentación)
2. [🆕 Novedades](#-novedades)
3. [Requisitos de hardware](#-requisitos-de-hardware)
4. [Requisitos de software](#-requisitos-de-software)
5. [Instalación rápida](#-instalación-rápida)
6. [Interfaz gráfica (GUI)](#️-interfaz-gráfica-gui)
   - [Pestaña Main](#-pestaña-main)
   - [Pestaña Avanzado](#-pestaña-avanzado)
   - [Pestaña Logs](#-pestaña-logs)
   - [Pestaña Configuración](#-pestaña-configuración)
7. [Detalle de los modos](#-detalle-de-los-modos)
8. [Flujo de trabajo recomendado](#-flujo-de-trabajo-recomendado)
9. [Tutorial de scraping en Recalbox](#-tutorial-de-scraping-en-recalbox)
10. [🔧 Montaje del panel DMD (DMDos)](#-montaje-del-panel-dmd-dmdos)
11. [Firmware ESP32 — Raw565 Edition](#-firmware-esp32--raw565-edition)
    - [¿Por qué raw565?](#-por-qué-raw565)
    - [Compilación con Arduino IDE](#-compilación-con-arduino-ide)
    - [Configuración (config.ini)](#️-configuración-configini)
    - [Instalación vía navegador (WebInstaller)](#-instalación-vía-navegador)
12. [El formato raw565 en detalle](#-el-formato-raw565-en-detalle)
    - [.raw565 (imagen fija PNG)](#-raw565-imagen-fija-png)
    - [.raw565pack + .meta (GIF animado)](#-raw565pack--meta-gif-animado)
    - [Caché bigrama — indexación acelerada](#-caché-bigrama--indexación-acelerada)
    - [Sistema de máscara (mask)](#-sistema-de-máscara-mask)
13. [Estructura de la tarjeta SD](#-estructura-de-la-tarjeta-sd)
14. [Solución de problemas](#-solución-de-problemas)
15. [Archivos generados por el script](#-archivos-generados-por-el-script)
16. [Créditos](#-créditos)

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

> **💡 Restricciones**: Es imprescindible usar el script para preparar la SD. No se pueden añadir imágenes a la SD sobre la marcha.

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
         │     RecalBoxDMD Toolkit (Prepara la SD)      │
         │  Extrae imágenes de los gamelists.xml         │
         │  Convierte PNG → .raw565 (formato RGB565)     │
         │  Convierte GIF → .raw565pack + .meta          │
         │  Construye el caché bigrama (games_cache.bin) │
         │  Marca sistemas lentos (flag "L")             │
         │  Copia todo a la tarjeta SD                   │
         └─────────────────────────────────────────────┘
```

---

## 🆕 Novedades

### 🎯 Gestión de versiones de Recalbox (Modo 1 y Modo 3)

Recalbox guarda las imágenes marquee/logo de forma diferente según la versión instalada **y** según las opciones elegidas en la pestaña Scraper de Recalbox. Para evitar obtener el visual equivocado (caratula en lugar del logo, por ejemplo), los modos 1 y 3 ahora muestran un panel **« Versión de Recalbox »**:

- Un selector **10.x / 9.x / legacy** que determina automáticamente la etiqueta correcta del `gamelist.xml` (`<logo>`, `<thumbnail>` o `<image>`) y la carpeta de medios correcta (`media/wheels/`, `media/thumbnails/` o `media/images/`).
- Un botón **« Cómo hacer el scrape? »** que muestra, para la versión elegida, una captura de pantalla anotada de la pestaña Scraper de Recalbox indicando exactamente qué campo ajustar (ver [Tutorial de scraping en Recalbox](#-tutorial-de-scraping-en-recalbox)).
- Un botón **« Limpiar carpetas antes del scrape »**: elimina (con confirmación y una vista previa del número de archivos) las imágenes ya presentes en la carpeta de medios correspondiente, sistema por sistema, para partir de un scrape de Recalbox limpio y completo.
- La versión elegida se recuerda (archivo de preferencias) y se comparte entre el Modo 1 (pestaña Main) y el Modo 3 (pestaña Avanzado).

### 🔎 Modo 8 — Verificación de imágenes faltantes

Nuevo modo en la pestaña Avanzado: recorre los `gamelist.xml` de la carpeta ROMs e informa, para cada sistema y cada juego, si la imagen esperada (según el perfil de versión elegido) existe realmente en el disco. Se genera un informe, y una opción adicional permite comparar con la carpeta de trabajo temporal (y, opcionalmente, con la tarjeta SD física) para distinguir una imagen faltante del lado ROMs, no convertida, o no copiada a la SD.

### 💾 Reanudación de la copia SD tras una interrupción

La copia a la tarjeta SD física (botón parpadeante tras un procesamiento) ahora resiste una interrupción (desconexión, fallo): escritura atómica de archivos, detección de una copia anterior incompleta en el siguiente inicio, y un botón dedicado para reintentar solo los archivos fallidos en lugar de toda la copia.

---

## 🧰 Requisitos de hardware

| Componente | Referencia | Precio aprox. |
|------------|-----------|---------------|
| 🧠 Microcontrolador | ESP32 DevKit V1 USB-C (38 pines) | ~5€ |
| 🖥️ Panel LED | 2× paneles matriciales RGB HUB75 **P4, 64x32, 256x128mm**, unidos horizontalmente (→ 128x32) | ~15-25€/panel |
| 💾 Lector SD | Módulo adaptador Micro SD (SPI) — integrado en la DMDos Board | ~2€ |
| 🔌 Placa de conexión | **DMDos Board V3** (recomendada, simplifica el cableado e incluye el lector SD) | ~15€ |
| ⚡ Alimentación | 5V 4A+ (según tamaño del panel) | ~10€ |

> **💡 Consejo**: ¡La DMDos Board de [Mortaca](https://www.mortaca.com/) integra el lector SD y evita la soldadura! Los enlaces de compra actualizados para cada componente (ESP32, DMDos Board, microSD, paneles P4) están centralizados en la página [Hardware de dmdos.net](https://www.dmdos.net/).
>
> Vea también la sección [🔧 Montaje del panel DMD (DMDos)](#-montaje-del-panel-dmd-dmdos) para la guía de montaje completa (traducida del sitio oficial).

---

## 💻 Requisitos de software

- **Python 3.8+** instalado en su PC (solo necesario para ejecutar desde las fuentes)
- **Pillow** (se instala automáticamente si falta)
- **Arduino IDE** (solo para recompilar el firmware)
- Navegador Chrome/Edge (para WebInstaller)

---

## 🚀 Instalación rápida

Descarga la versión que prefieras desde la **[página de Releases](https://github.com/shan-aya/RecalBoxDMD/releases)** (los `.exe`/`.msi` compilados no están en el repositorio, solo publicados ahí):

**Opción A — Instalador de Windows (recomendado)**

```
1. Descarga RecalBoxDMD_Toolkit_Setup.exe desde la página de Releases
2. Ejecútalo — acceso directo en el menú Inicio, icono de escritorio opcional, desinstalador real
3. Abre «RecalBoxDMD Toolkit» desde el menú Inicio
```

**Opción B — Ejecutable portable (sin instalación)**

```
1. Descarga RecalBoxDMD_GUI.exe desde la página de Releases
2. Ejecútalo directamente — sin instalación, sin necesidad de Python, archivo único
```

**Opción C — .msi (para despliegue mediante script/GPO)**

```
1. Descarga el .msi desde la página de Releases
2. msiexec /i "RecalBoxDMD Toolkit-1.0.0-win64.msi"   (o doble clic)
```

**Opción D — Desde las fuentes Python**

```
1. Descargue los archivos de la carpeta tools/
2. Haz doble clic en install_and_run.bat — instala Python (vía winget,
   si falta), Pillow y Markdown, y luego abre la GUI
   (o manualmente: pip install Pillow Markdown && python run_gui.py)
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
│ │ 🎛️ Versión de Recalbox (scrape marquee/logo)           │ │
│ │ [ 10.x ▾ ]                                            │ │
│ │ [ Cómo hacer el scrape? ]                              │ │
│ │ [ Limpiar carpetas antes del scrape ]                  │ │
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
0. Elección de la **versión de Recalbox** (10.x / 9.x / legacy) — determina la etiqueta del `gamelist.xml` y la carpeta de medios usadas
1. Extracción de imágenes desde sus `gamelist.xml`
2. Conversión PNG → raw565 (formato RGB565 bruto, 8192 bytes)
3. Conversión GIF → raw565pack + meta (tramas concatenadas)
4. Descarga de `systems/_defaults` desde GitHub
5. Construcción de `games_cache.bin` (índice bigrama)
6. Generación de `systems_cache.dat` (índice sistemas + flag lento/rápido)
7. Detección automática de sistemas grandes (flag `L` para MAME, FBNeo...)

Una vez terminado el procesamiento, un botón parpadea para ofrecer la **copia directa a la tarjeta SD física** (detección automática de unidades extraíbles).

### 🔬 Pestaña Avanzado

> Contiene los **modos 2 a 8** para operaciones específicas.

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
│ │ ○ MODO 8 — Verificar imágenes faltantes               │ │
│ │                                                       │ │
│ │ [ 🟢 Iniciar ]                                        │ │
│ └────────────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────────┘
```

> El **Modo 3** también muestra el panel **« Versión de Recalbox »** (mismo selector, mismos botones de ayuda/limpieza que el Modo 1), ya que también extrae imágenes desde los `gamelist.xml`.
> El **Modo 8** muestra un panel propio con su propio selector de versión, el botón para abrir el informe, y un botón de comparación con el soporte final (carpeta temporal / tarjeta SD).

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

También permite cambiar el tema visual, y elegir la **« Versión de Recalbox »** por defecto (10.x / 9.x / legacy) — el mismo valor compartido que usan los paneles de los Modos 1/3/8: un cambio aquí (o en cualquiera de esos modos) se refleja en todas partes de inmediato y se guarda en `RecalBoxDMD_prefs.json`.

### 📖 Pestaña AYUDA

Muestra este README directamente en la interfaz.

---

## 📖 Detalle de los modos

| Modo | Nombre | Acción | Duración est. |
|------|--------|--------|---------------|
| **1** | **TODO** | Versión de Recalbox + Extracción + Conversión raw565 + Caché + _defaults | Larga (5-30 min) |
| **2** | Descargar _defaults | Obtiene imágenes de sistemas desde GitHub | Corta (~1 min) |
| **3** | Extracción solamente | Versión de Recalbox + lee gamelist.xml, copia imágenes | Variable |
| **4** | Conversión raw565 | PNG→raw565 + GIF→raw565pack/meta | Media |
| **5** | Conversión 128x32 | Redimensiona PNGs a 128x32 (formato imagen) | Media |
| **6** | Build Games Cache | Genera games_cache.bin (índice bigrama) | Rápida |
| **7** | Generar systems_cache.dat | Índice sistemas + flags lento/rápido | Rápida |
| **8** | **Verificación de imágenes faltantes** | Compara gamelist.xml ↔ imágenes presentes (+ comparación opcional con soporte final) | Variable |

> **📎 Copia a la tarjeta SD**: no es un modo aparte — tras un procesamiento (Modo 1, 3, 4, 5...), un botón parpadea automáticamente en la pestaña Main ofreciendo copiar a una unidad extraíble detectada. La copia resiste interrupciones y ofrece reintentar solo los archivos fallidos.

---

## ✅ Flujo de trabajo recomendado

```
1. 🔄 Haga scrape de sus juegos en Recalbox (vea el Tutorial de scraping
   más abajo según su versión de Recalbox: logo dedicado, marquee o logo recortado)
   ↓
2. 📂 Ejecute RecalBoxDMD Toolkit (acceso directo instalado, RecalBoxDMD_GUI.exe portable, o python run_gui.py) → pestaña Main
   ↓
3. 🎛️ Elija su « Versión de Recalbox » (10.x / 9.x / legacy)
      ¿Necesita ayuda? Haga clic en « Cómo hacer el scrape? »
      ¿Carpeta ya scrapeada con una config antigua? « Limpiar carpetas antes del scrape »
   ↓
4. 🗂️ Elija su carpeta de ROMs (ej: D:\Recalbox\share\roms)
   ↓
5. 🟢 Haga clic en Iniciar (Modo 1)
   ↓
6. ⏳ Deje que el script trabaje. Hará:
      - Extraer imágenes de gamelist.xml (según la versión elegida)
      - Convertir PNG → .raw565 (formato RGB565 rápido)
      - Convertir GIF → .raw565pack + .meta
      - Descargar _defaults desde GitHub
      - Construir el caché bigrama (indexación de juegos)
      - Marcar sistemas grandes (flag "L" para MAME, FBNeo)
   ↓
7. ✅ Terminado! La carpeta sd_card/ está lista en %TEMP%/RecalBoxDMD
   ↓
8. 💾 Inserte su tarjeta SD, el botón parpadeante ofrece copiar
      (opcional: Modo 8 para verificar que no falte ninguna imagen)
   ↓
9. 🎮 Inserte la SD en el ESP32 y encienda!
      - Visualización casi instantánea incluso con 30,000 juegos MAME
```

---

## 🎨 Tutorial de scraping en Recalbox

Para que el panel LED muestre los visuales correctos, primero debe hacer **scrape** de sus ROMs en Recalbox — y configurar bien la pestaña Scraper **según su versión**. Esto es exactamente lo que hace el botón **« Cómo hacer el scrape? »** del Modo 1/3 (capturas de pantalla anotadas directamente en la app), resumido aquí:

### Recalbox 10.x (campo logo dedicado)

Desde Recalbox 10.x, el menú **SCRAPER → SCRAPE NOW** tiene un campo dedicado **« SELECT LOGO TYPE »** (a veces aún en inglés en versiones alpha/beta):

1. Ajuste **« SELECT LOGO TYPE »** a **« CLEAR »**
2. No ajuste ese valor en **« Seleccione el tipo de imagen »** — ese campo es para el visual principal (pantalla de juego, caratula...), no para el logo
3. Lance el scrape

Los archivos se guardan en `media/wheels/` de cada sistema, referenciados por la etiqueta `<logo>` del `gamelist.xml`. → Elija el perfil **10.x** en la herramienta.

### Recalbox 9.x (sin campo logo dedicado)

En versiones de Recalbox sin campo « logo » dedicado:

1. Ajuste **« Seleccione el tipo de miniatura »** a **« MARQUEE »**
2. No ajuste **« Seleccione el tipo de imagen »** a ese valor — se usa para otro visual (caratula, pantalla de título...)
3. Lance el scrape

Los archivos se guardan en `media/thumbnails/` de cada sistema, referenciados por la etiqueta `<thumbnail>`. → Elija el perfil **9.x** (recomendado) en la herramienta.

### Perfil « legacy » (vía el campo imagen)

Si su configuración de Recalbox escrapea el logo/marquee mediante **« Seleccione el tipo de imagen »** (ajustado a **« LOGO RECORTADO »** o **« MARQUEE »**), los archivos se guardan en `media/images/`, referenciados por la etiqueta `<image>`. → Elija el perfil **legacy**. Resérvelo para configuraciones que no usan el campo miniatura para el logo.

```
📁 /recalbox/share/roms/
   ├── 📁 mame/
   │   ├── 📄 gamelist.xml
   │   ├── 📁 media/
   │   │   ├── 📁 wheels/         ← perfil 10.x (<logo>)
   │   │   ├── 📁 thumbnails/     ← perfil 9.x (<thumbnail>)
   │   │   └── 📁 images/         ← perfil legacy (<image>)
   │   └── ...
   ├── 📁 snes/
   │   ├── 📄 gamelist.xml
   │   ├── 📁 media/
   │   │   └── 📁 wheels/
   │   │       └── 🖼️ supermetroid.png
   └── ...
```

> **💡 Consejo**: ¡Use Screenscraper.fr o TheGamesDB para mejores resultados!
>
> **💡 ¿Ya hizo scrape con otra configuración?** Use el botón **« Limpiar carpetas antes del scrape »** (Modo 1/3) antes de relanzar un scrape de Recalbox, para no mezclar visuales antiguos con los nuevos.
>
> **💡 ¿Duda sobre las imágenes obtenidas?** Lance el **Modo 8** para generar un informe de imágenes faltantes o inconsistentes respecto al perfil elegido.

---

## 🔧 Montaje del panel DMD (DMDos)

El montaje del hardware (paneles LED + DMDos Board + ESP32 + microSD) es idéntico al descrito en el sitio oficial **[dmdos.net](https://www.dmdos.net/)** de Mortaca. Instrucciones traducidas a continuación — para las imágenes de la guía y los enlaces de compra actualizados, consulte directamente las páginas [Hardware](https://www.dmdos.net/) y [Montaje](https://www.dmdos.net/) del sitio.

> ⚠️ **DMDos** (el sitio web) ofrece su propio sistema operativo/firmware para este mismo hardware. **No flashee el firmware DMDos** en su ESP32 si desea usar el firmware **RecalBoxDMD** de este repositorio — aquí solo se reutilizan el **hardware** (paneles, DMDos Board, mueble) y la **guía de montaje físico**. El firmware y el contenido de la tarjeta SD deben provenir de este Toolkit (vea [Firmware ESP32](#-firmware-esp32--raw565-edition)).

### 🛒 Hardware (enlaces de compra)

| Componente | Descripción |
|-----------|-------------|
| **Placa ESP32 USB-C** | ESP32 DevKit V1, conexión USB-C |
| **DMDos Board V3** | Diseñada por **MORTACA** — conecta el ESP32 al panel DMD, integra el lector microSD y la alimentación de los paneles |
| **MicroSD 32 GB** | Se recomienda tipo RaspberryPi (evita corrupciones), pero cualquier tarjeta que tenga sirve |
| **2× Paneles P4 64x32** | Se unen horizontalmente para obtener un panel 128x32 (256x128 mm cada uno) |

👉 Enlaces de compra actualizados (AliExpress, Mortaca) en la página **[Hardware de dmdos.net](https://www.dmdos.net/)**.

### 🪛 Pasos de montaje

**Paso 1 — Unir los dos paneles**

Siguiendo estos pasos podrá montar su panel DMD en menos de cinco minutos, incluso si no tiene experiencia en bricolaje. Lo primero será unir los dos paneles con las piezas de unión que vienen junto a la DMDos Board. Los tornillos no están incluidos, pero puede usar cualquier tornillo de rosca M3 que tenga por casa; por ejemplo, puede tomar alguno de una regleta eléctrica.

**Paso 2 — Posicionar la DMDos Board**

Una los paneles fijándose en que la orientación de los componentes traseros sea la misma en ambos lados. Una vez unidos, verá dos conectores idénticos donde colocar la DMDos Board: uno es de **entrada** y el otro de **salida**. Coloque la placa en el conector de **entrada**, en la orientación que permita situarla fácilmente sin chocar con el soporte de plástico. ⚠️ Solo funcionará en el conector de entrada.

**Paso 3 — Cableado de la alimentación**

Una vez conectada la placa, y **antes** de colocar encima el ESP32, ponga los cables de corriente rojo y negro de ambos paneles en las terminales de la placa DMDos, tal y como se indica en la serigrafía: rojo a un lado y negro al otro. Puede mantener los conectores que traen los cables y atornillar solo una patilla; también puede cortar la sobrante o pelar todo el cable para que entre por completo en el terminal. Conecte también los paneles entre sí con la cinta plana que los acompaña.

**Paso 4 — SD, ESP32 y alimentación**

Por último, inserte la tarjeta micro-SD previamente preparada (vea [Flujo de trabajo recomendado](#-flujo-de-trabajo-recomendado)) y encaje encima el ESP32 ya flasheado con el firmware **RecalBoxDMD** (vea [Firmware ESP32](#-firmware-esp32--raw565-edition)). Solo quedará conectarlo a la corriente mediante el puerto USB-C.

### 📦 Mueble / Carcasa

- **Impresión 3D**: modelo (cuadrado o redondeado) creado por **Janibol** de [Retromojones](https://www.youtube.com/@retromojones), disponible en [Thingiverse](https://www.thingiverse.com/thing:6704880). Requiere una impresora 3D de gran formato (base ≥ 300×300×400 mm). También se puede encargar un kit impreso (piezas + tapas traseras + tornillos + interruptor + conector USB-C exterior, sin incluir vidrio/metacrilato) — vea la página **[Mueble de dmdos.net](https://www.dmdos.net/)**.
- **Mueble de madera a medida**: algunos artesanos ofrecen muebles « old school » con cristal parsol incluido — vea los contactos en la página **[Mueble de dmdos.net](https://www.dmdos.net/)**.

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

### ❌ Problema: Aparece la imagen equivocada (caratula en lugar del logo/marquee)
- Suele ser un mal ajuste del campo Scraper de Recalbox — verifique el perfil **« Versión de Recalbox »** (Modo 1/3) y haga clic en **« Cómo hacer el scrape? »** para la marcha exacta según su versión
- Use **« Limpiar carpetas antes del scrape »** y relance un scrape completo en Recalbox con el ajuste correcto
- Lance el **Modo 8** para verificar qué imágenes están realmente presentes

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
| `images_manquantes.txt` | Texto | Lista de imágenes faltantes (Modo 1/2/3) |
| `reports/mode8_report_*.txt` | Texto | Informe del Modo 8 (verificación gamelist ↔ imágenes) |

---

## 🤝 Créditos

- **Proyecto original RetroBoxLED**: [Jamyz](https://github.com/Jamyz/RetroBoxLED) — el firmware ESP32 y la idea base
- **Raw565 Edition**: Shan_ayA — optimización del formato raw565, caché bigrama, sistema de máscara, interfaz gráfica, gestión de versiones de Recalbox
- **Inspiración**: [RetroPixelLED](https://github.com/fjgordillo86/RetroPixelLED) por fjgordillo86
- **Comunidad**: [Recalbox](https://www.recalbox.com/)
- **Hardware y guía de montaje**: [Mortaca - DMDos Board](https://www.mortaca.com/) y [dmdos.net](https://www.dmdos.net/)
- **Mueble 3D**: Janibol — [Retromojones](https://www.youtube.com/@retromojones)

---

## ☕ Apoyo

Si este proyecto le es útil:
👉 [Donar vía PayPal](https://www.paypal.com/paypalme/felysaya)

---

> **RecalBoxDMD Raw565 Edition** = Recalbox + Panel DMD LED + Visualización rapido incluso con 30,000 juegos MAME! 🎮⚡
