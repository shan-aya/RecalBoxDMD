# RecalBox DMD — Firmware ESP32 para Panel Marquesina

## Descripción General

**RecalBox_DMD** es un firmware ESP32 completo para controlar un **panel HUB75 LED (128×32 px)** que muestra imágenes marquesina de juegos Recalbox. El firmware soporta PNG y GIF, caché de juegos, integración MQTT en tiempo real, Telnet para depuración y Bluetooth.

---

## Características

- **Soporte de Imágenes**: PNG y GIF con caché inteligente
- **Optimización de Rendimiento**: formatos raw565 / raw565pack para renderizado rápido
- **Caché de Juegos**: búsqueda indexada por bigramas para acceso rápido
- **Caché de Sistemas**: detección de tipo de sistema y fallback automático
- **Red**:
  - WiFi (estática o DHCP)
  - MQTT (visualización en tiempo real del juego/sistema)
  - Telnet (depuración y control manual)
- **Audio**: soporte Bluetooth Serial
- **Configuración**: controlada por `config.ini`
- **Fallback**: retorno automático a imágenes por defecto si faltan

---

## Requisitos de Hardware

### Microcontrolador
- **ESP32** (probado en DEVKIT-C, 4 MB de flash recomendado)

### Panel de Visualización
- **Panel HUB75 LED** 64×32 píxeles, **2 paneles encadenados** (128×32 visibles)

### Almacenamiento
- **Tarjeta SD** (microSD vía adaptador SPI, **formateada FAT32**)

### Alimentación
- 5V para paneles LED (~2A típico)
- 5V USB para ESP32 (puede compartir alimentación vía regulador separado)

---

## Configuración de Pines

### SPI (Tarjeta SD)
| Pin | GPIO |
|-----|------|
| CS  | 5    |
| MISO| 19   |
| MOSI| 23   |
| SCLK| 18   |

### Pines Panel HUB75
| Señal | GPIO |
|-------|------|
| CLK   | 16   |
| OE    | 15   |
| LAT   | 4    |
| A     | 33   |
| B     | 32   |
| C     | 22   |
| D     | 17   |
| E     | -1   |

### Pines de Color (Semi Canales)
| Mitad Superior | GPIO | Mitad Inferior | GPIO |
|----------------|------|----------------|------|
| R1             | 25   | R2             | 14   |
| G1             | 26   | G2             | 12   |
| B1             | 27   | B2             | 13   |

---

## Estructura de Tarjeta SD

```
(Raíz SD)
├── config.ini                          # Configuración principal
├── systems_cache.dat                   # Índice de sistemas (generado)
├── games_cache.bin                     # Caché de imágenes de juegos (generado)
├── systems/
│   ├── _defaults/
│   │   ├── default.png                # Imagen de respaldo (128×32 PNG)
│   │   ├── default.raw565             # Respaldo rápido (128×32 raw565)
│   │   ├── <system>.png               # Logos de sistemas
│   │   ├── <system>.gif
│   │   ├── <system>.raw565
│   │   ├── <system>.raw565pack        # GIF pre-codificado en raw frames
│   │   └── <system>.meta              # Metadatos de retraso raw565pack
│   │
│   └── <system>/
│       ├── <game>.png                 # Imágenes marquesina de juegos
│       ├── <game>.gif
│       ├── <game>.raw565              # Formato rápido pre-convertido
│       └── <game>.raw565pack          # Pack de raw frames
│
└── playlists/
    ├── <playlist_name>.txt            # Lista de archivos GIF
    ├── <playlist_name>.cache          # Caché compilado
    ├── <playlist_name>.idx            # Índice de offsets
    └── <playlist_name>.sig            # Firma hash
```

---

## Bibliotecas Arduino Requeridas

Instalar vía **Sketch → Include Library → Manage Libraries**:

| Biblioteca | Autor | Versión |
|------------|-------|---------|
| `ESP32-HUB75-MatrixPanel-I2S-DMA` | mrfaptastic | ≥1.0.0 |
| `AnimatedGIF` | Larry Bank | ≥2.0.0 |
| `SD` | Arduino | (integrada) |
| `SPI` | Arduino | (integrada) |
| `WiFi` | Arduino | (integrada) |
| `PubSubClient` | Nick O'Leary | ≥2.8.0 |
| `BluetoothSerial` | Espressif | (núcleo ESP32) |

**Nota**: `pngle.h` está incluida directamente en el sketch para decodificación PNG.

---

## Configuración (`config.ini`)

Ejemplo:

```ini
# Playlist a mostrar al inicio
playlist=marquee.txt

# Configuración WiFi
wifi_enabled=1
wifi_ssid=MiSSID
wifi_password=contraseña123
wifi_static_enabled=0
# IP estática opcional:
# wifi_static_ip=192.168.1.100
# wifi_gateway=192.168.1.1
# wifi_subnet=255.255.255.0
# wifi_dns1=8.8.8.8
# wifi_dns2=8.8.4.4

# Broker MQTT Recalbox
recalbox_ip=192.168.1.50

# Topic MQTT (recibe eventos de Recalbox)
mqtt_event_topic=marquee/event

# Bluetooth
bluetooth_enabled=0
bluetooth_name=ESP32-GIF

# Pantalla splash al inicio
info=1

# Modo reproducción de playlist
random=1
```

---

## Compilación y Flasheo

### 1. Requisitos Previos
- Arduino IDE ≥ 1.8.0
- Paquete de Placa ESP32 (≥2.0.0)
  - URL del gestor de placas: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`

### 2. Abrir Sketch
- File → Open → `RecalBox_DMD.ino`

### 3. Configurar Placa
- Tools → Board: Seleccionar **ESP32 Dev Module** (o su variante)
- Tools → Flash Size: Seleccionar **4MB** (o el tamaño de su placa)
- Tools → Upload Speed: **921600** (o menor si es inestable)
- Tools → Port: Seleccionar puerto COM

### 4. Instalar Dependencias
- Sketch → Include Library → Manage Libraries
- Buscar e instalar cada biblioteca de la tabla anterior

### 5. Compilar
- Sketch → Verify (Ctrl+R)

### 6. Cargar
- Sketch → Upload (Ctrl+U)
- O usar **esptool.py** (línea de comandos):
  ```bash
  esptool.py --chip esp32 --port COM3 --baud 921600 write_flash -z 0x10000 RecalBox_DMD.ino.bin
  ```

---

## Uso

### Inicio
1. Encender ESP32
2. Si `info=1`, mostrar pantalla splash (2,5 segundos)
3. Si WiFi activado, conectar y mostrar estado
4. Cargar playlist (si está configurada) o mostrar por defecto
5. Listo para comandos MQTT / entrada Telnet

### Topics MQTT (Entrada)
- `marquee/cmd/stop` — Detener reproducción
- `marquee/cmd/default` — Reanudar playlist
- `marquee/cmd/system <system_name>` — Mostrar logo de sistema
- `marquee/cmd/game <system>/<game>` — Mostrar imagen del juego
- `marquee/event` — Suscribirse a eventos Recalbox (manejado automáticamente)

### Comandos Telnet (Puerto 23)
```
help               — Listar todos los comandos
ip                 — Mostrar dirección IP
wifi               — Mostrar estado WiFi
wifiinfo           — Información WiFi detallada
next               — Siguiente GIF
count              — Mostrar número de GIFs cargados
playlist           — Mostrar playlist actual
random on|off      — Activar/desactivar reproducción aleatoria
reboot             — Reiniciar ESP32
show <path>        — Mostrar archivo (ej: /systems/ps2/game.png)
showsys <system>   — Mostrar logo del sistema
showgame <sys/rom> — Mostrar imagen del juego
default            — Mostrar imagen por defecto
black              — Limpiar pantalla
mode               — Mostrar modo de visualización actual
syscache           — Listar caché de sistemas
rebuildcache       — Reconstruir caché de sistemas
heap               — Mostrar memoria libre
```

---

## Pipeline de Procesamiento de Datos

### Caché de Sistemas
1. En el primer inicio (o `rebuildcache`), escanear carpeta `/systems/`
2. Detectar tipos de imagen disponibles por sistema (PNG/GIF/raw565pack)
3. Marcar sistemas con >800 juegos como "lentos" (flag 'L')
4. Guardar índice en `systems_cache.dat`

### Caché de Juegos
1. Herramientas Python (`tools/`) pre-procesan listas de juegos desde `gamelist.xml`
2. Extraer imágenes marquesina, convertir PNG a 128×32
3. Construir `games_cache.bin` con búsqueda indexada por bigramas
4. Copiar a raíz de tarjeta SD

### Flujo de Visualización
- **Sistema Rápido (N)**: decodificación PNG + dibujar, fallback raw565, fallback por defecto
- **Sistema Lento (L)**: verificar caché primero, fallback raw565 por defecto (omitir decodificación PNG)
- **Solicitud de Juego MQTT**: cargar máscara de sistema, luego imagen del juego, con cadena de fallback

---

## Herramientas Python

Los scripts de preparación están disponibles en la carpeta `tools/`:

- `RetroBoxLED_gui.py` — GUI para extracción de imágenes, conversión y preparación de SD
- `RetroBoxLED_tool3.3.py` — Procesador batch por línea de comandos

Ver `tools/README.es.md` para instrucciones detalladas.

---

## Solución de Problemas

| Problema | Solución |
|----------|----------|
| **Tarjeta SD no detectada** | Verificar pin CS (5), asegurar FAT32, probar con comando `dir` en Telnet |
| **Panel muestra basura** | Verificar que pines HUB75 coincidan con configuración; verificar polaridad del panel |
| **Carga lenta de imágenes** | Marcar sistema como lento (flag 'L') en config; pre-convertir PNG a raw565 |
| **WiFi / MQTT no funciona** | Verificar SSID, contraseña e IP del broker en `config.ini`; verificar Telnet `wifiinfo` |
| **Falta de memoria** | Reducir caché de bigramas o convertir PNG grandes a formato raw565pack |

---

## Notas de Rendimiento

- **Decodificación PNG**: ~200–500ms por imagen (depende del tamaño y velocidad del sistema)
- **Reproducción GIF**: 10–50 FPS (limitado por refresco del panel y velocidad SPI)
- **Blitting raw565**: ~50ms por refresco de pantalla completa (optimizado con lectura única en bloque)
- **Latencia MQTT**: ~100–500ms desde el mensaje hasta la actualización en pantalla

---

## Licencia

Este firmware se proporciona tal cual para uso personal y educativo.

---

## Soporte y Comentarios

Para problemas, preguntas o contribuciones, consulte el repositorio del proyecto o póngase en contacto con el mantenedor.

---

**Versión**: Raw565 Edition  
**Última actualización**: Junio 2026
