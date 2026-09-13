# Inicio rápido — RecalBoxDMD RawEdition v2.0

[🇬🇧 English](QUICK-START.md) · [🇫🇷 Français](QUICK-START.fr.md) · 🇪🇸 **Español**

> Esta página recoge solo la sección «Inicio rápido» del **[README.md](README.es.md)** completo — práctica para compartir o imprimir. Para funciones, hardware, configuración y todo lo demás, consulta el README completo.

<p align="center"><b>🚀 De cero a un marquee funcionando en 4 pasos 🚀</b></p>

<table align="center">
<tr>
<td align="center" width="70"><h2>1️⃣</h2></td>
<td>

**[Instala la caja de herramientas de PC](#instala-la-caja-de-herramientas-de-pc) + primer arranque**
Haz el scrape de tus juegos en Recalbox, apunta la herramienta a tu carpeta de ROMs, pulsa **Iniciar**.

</td>
</tr>
<tr>
<td align="center"><h2>2️⃣</h2></td>
<td>

**[Monta el DMD](README.es.md#hardware)**
Une los dos paneles, coloca la placa DMDos, cablea — **~5 minutos, sin soldadura**.

</td>
</tr>
<tr>
<td align="center"><h2>3️⃣</h2></td>
<td>

**[Flashea el firmware](README.es.md#firmware--compilar-y-flashear)**
Instalador web en un clic — **sin necesidad de Arduino IDE**.

</td>
</tr>
<tr>
<td align="center"><h2>4️⃣</h2></td>
<td>

**Inserta la tarjeta SD, enciende**
El primer arranque te guía por la configuración WiFi, y luego la **[página de configuración web](README.es.md#configuración-web--en-vivo-en-el-navegador)** se encarga de todo lo demás (brillo, playlists, temas de reloj...).

</td>
</tr>
</table>

## Instala la caja de herramientas de PC

Se descarga de 4 formas — elige la que prefieras en la **[página de Releases](https://github.com/shan-aya/RecalBoxDMD/releases)** (los archivos `.exe`/`.msi` compilados no están en el propio repositorio, solo publicados ahí):

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

**Opción D — Desde el código fuente Python**

```
1. Descarga la carpeta tools/
2. Haz doble clic en install_and_run.bat — instala Python (vía winget,
   si falta), Pillow y Markdown, y luego abre la GUI
   (o manualmente: pip install Pillow Markdown && python run_gui.py)
```

## Primer arranque

```
1. Haz el scrape de tus juegos en Recalbox (ver «¿Cómo hacer el scrape?» en la
   herramienta, según tu versión de Recalbox — logo, marquee o logo recortado)
2. Abre la caja de herramientas → pestaña Main
3. Elige tu versión de Recalbox (10.x / 9.x / legacy)
4. Elige tu carpeta de ROMs (ej.: D:\Recalbox\share\roms)
5. Haz clic en Iniciar — el MODO 1 encadena todo el proceso automáticamente
6. Inserta la tarjeta SD → el botón parpadeante se ofrece a copiarla por ti
```

A continuación: [monta el hardware](README.es.md#hardware) y [flashea el firmware](README.es.md#firmware--compilar-y-flashear) — luego inserta esa tarjeta SD y enciende.

---

📖 Documentación completa: **[README.md](README.es.md)** · Actualizar desde una versión anterior: **[UPGRADING.md](UPGRADING.es.md)** · Historial de versiones: **[CHANGELOG.md](CHANGELOG.es.md)**
