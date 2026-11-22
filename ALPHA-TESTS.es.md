# 🧪 Plan de pruebas alfa

Informe de cada resultado con **`/alphatest`** en Discord (véase el [README](README.es.md)). ⚠️ = precaución.

## T01 · Orden de colores del panel (ESP32)  —  *ESP32*

**Pasos**

1. Página web del DMD → **Ajustes** → *Orden de colores*.
2. Elegir otro orden (p. ej. GRB) → *Aplicar y reiniciar*.
3. Mirar el título de arranque / un logo: ¿los colores siguen el orden elegido?
4. Volver al orden que muestra los colores correctos; reiniciar una vez más.

**Esperado** : El DMD se reinicia, los colores cambian con el orden y el orden elegido se conserva tras otro reinicio.

## T02 · Orden de colores del panel (Raspberry Pi)  —  *Raspberry Pi*

**Pasos**

1. Página web de la Pi → **Ajustes** → *Orden de colores*.
2. Elegir otro orden → aplicar (el servicio se reinicia).
3. Comprobar los colores en el panel y restaurar el orden correcto.

**Esperado** : El panel vuelve con el nuevo orden; el ajuste sobrevive a un reinicio de la Pi.

## T03 · Diseño de la página Ajustes  —  *ESP32 / Pi / Toolkit*

**Pasos**

1. Abrir **Ajustes** en la ESP32 y en la Pi (móvil y PC).
2. Comprobar las secciones: *Canal del firmware*, *Pantalla* (orden de colores sobre el brillo), *Arranque y modos*.
3. Comprobar que los tres botones (cambiar de canal / aplicar y reiniciar / configurar) tienen el mismo tamaño y están alineados.
4. Cambiar el idioma de la página FR / EN / ES.

**Esperado** : Mismo aspecto en ambos dispositivos, sin solapamientos, textos traducidos, controles usables en el móvil.

## T04 · Cambio de canal alfa ↔ estable (ESP32)  —  *ESP32*

**Espere al menos 60 segundos de funcionamiento normal entre dos cambios.**

**Pasos**

1. **Ajustes** → *Cambiar al canal STABLE…* → confirmar.
2. Esperar unos 1,5 minutos (descarga, reinicios).
3. Comprobar el título en el panel (`STABLE`) y la versión en la página.
4. Esperar 60 s y volver a ALPHA.

**Esperado** : Ambos cambios funcionan, las páginas web vuelven solas y se conservan el Wi-Fi y los ajustes.

## T05 · Cambio de canal alfa ↔ estable (Raspberry Pi)  —  *Raspberry Pi*

**Pasos**

1. **Ajustes** de la Pi → *Cambiar al canal STABLE…* → confirmar.
2. Esperar el reinicio del servicio; comprobar el título `Pi STABLE` y la versión.
3. Volver a ALPHA.

**Esperado** : Se instala el demonio del canal elegido cada vez; la Pi nunca cambia de canal por sí sola.

## T06 · Los avisos de actualización se quedan en el canal alfa  —  *ESP32 / Pi / Toolkit*

**Pasos**

1. Abrir las páginas web (ESP32, Pi) y la caja de herramientas.
2. Leer los avisos de actualización (firmware, Pi, toolkit).
3. Seguir los enlaces.

**Esperado** : Los avisos solo citan versiones alfa (pre-releases); nunca un número ni una descarga estable.

## T07 · Las páginas web vuelven tras actualizar el firmware  —  *ESP32*

**Pasos**

1. Actualizar el firmware por Wi-Fi (página web o Modo 14 del toolkit).
2. Esperar 1 a 2 minutos sin tocar nada.
3. Recargar la página web.

**Esperado** : Las páginas vuelven solas. Si aparece la página de rescate, informe de la línea de causa (captura).

## T08 · Los ajustes sobreviven a los reinicios  —  *ESP32*

**Pasos**

1. Cambiar algunos ajustes (brillo, idioma, un widget) y guardar.
2. Reiniciar el DMD tres veces (botón / alimentación).
3. Comprobar los ajustes y que vuelve a su Wi-Fi.

**Esperado** : Todo se conserva; el DMD nunca vuelve a su propio punto de acceso (modo AP).

## T09 · Cambiar la red Wi-Fi  —  *ESP32*

**Pasos**

1. Página **Red** → elegir o volver a introducir su Wi-Fi → guardar.
2. Esperar la reconexión.
3. Recargar la página Ajustes.

**Esperado** : El DMD se reconecta y las páginas cargan; no se pierde nada más.

## T10 · GIF del juego durante la partida  —  *ESP32 / Pi / Toolkit*

**Pasos**

1. Página web *Basic* → activar *GIF del juego durante la partida*.
2. Volver a ejecutar el **Modo 9** (scripts Recalbox) del toolkit una vez.
3. Lanzar juegos que tengan un GIF con el mismo nombre (en `/gifs_jeux` de la tarjeta SD, o dejar el modo auto).
4. Observar: 1 a 5 GIF y luego el logo; dejar un juego en modo demo.

**Esperado** : Solo se reproducen coincidencias seguras (nunca el juego equivocado); 1 GIF por pasada en demo; un archivo de exclusiones excluye de verdad.

## T11 · Personalización de logos  —  *Toolkit*

**Pasos**

1. Toolkit → pestaña *Personalización* → elegir un juego.
2. Sustituir su logo por una imagen, luego por un GIF animado y luego por una tarjeta de texto.
3. Usar *Cancelar* (versión anterior y luego el original), *Restaurar mis logos* y la copia / restauración zip.
4. Repetir en la tarjeta SD y en una Raspberry Pi por Wi-Fi si tiene una.

**Esperado** : El logo cambia en el DMD, cada cancelación devuelve lo que debe y no se pierde nada.

## T12 · Etiqueta de destino y Playlist por Wi-Fi  —  *Toolkit*

**Pasos**

1. Abrir el toolkit con una tarjeta insertada y luego con una Pi en la red.
2. Comprobar la etiqueta de destino (ESP32 / Pi) y los modos en gris.
3. Pestaña *Playlist*: escribir en la tarjeta y luego por Wi-Fi a una Pi sin tarjeta (también las playlists de temporada).

**Esperado** : Se preselecciona el destino correcto, los modos no aplicables están en gris y las playlists llegan al dispositivo.

## T13 · Pestaña Instalación (Modo 1)  —  *Toolkit*

**Pasos**

1. Toolkit → pestaña *Instalación* → página *Sin Recalbox* y luego *Con una Recalbox*.
2. Recorrer Iniciar / Siguiente / Continuar: primero la carpeta de ROMs y luego la versión de Recalbox.
3. Leer los avisos sobre la versión de Recalbox y los medios scrapeados.

**Esperado** : Siguiente / Continuar rechazan una carpeta de ROMs no válida; los avisos son claros; la copia termina.

## T14 · Formatear una tarjeta SD en FAT32  —  *Toolkit*

**⚠️ Esto BORRA la tarjeta. Use una tarjeta de repuesto, nunca una que necesite.**

**Pasos**

1. Insertar una tarjeta de repuesto (idealmente de más de 32 GB).
2. Toolkit → Modo 1 → ventana Tarjeta SD → *Formatear en FAT32…*.
3. Aceptar la petición de administrador y las dos confirmaciones que nombran la unidad.
4. Poner la tarjeta en el DMD y comprobar que arranca.

**Esperado** : Solo se ofrecen tarjetas extraíbles; la tarjeta se formatea en FAT32 y funciona en el DMD.

## T15 · Aviso de canal distinto en el toolkit  —  *Toolkit*

**Pasos**

1. Poner un DMD en el canal STABLE (T04).
2. Abrir el toolkit alfa con ese DMD accesible.

**Esperado** : El toolkit avisa de la diferencia y ofrece el toolkit del canal del DMD.

## T16 · El botón de actualización de la Pi se queda en alfa  —  *Raspberry Pi*

**Pasos**

1. En la página web de la Pi (alfa), usar el botón de actualización cuando se anuncie un nuevo demonio alfa.
2. Comprobar después la versión y el título.

**Esperado** : La Pi se queda en alfa (`Pi ALPHA`) y nunca instala un demonio estable.

## T17 · Temas de Recalbox (Modos 12 / 13)  —  *Toolkit*

**Pasos**

1. Ejecutar el Modo 12 o 13 en una Recalbox alfa.
2. Comprobar que los logos del tema Recalbox por defecto no se sobrescriben con una conversión antigua.
3. Volver a descargar un tema: los archivos obsoletos deben desaparecer.

**Esperado** : El tema por defecto se mantiene al día con la Recalbox alfa; sin restos obsoletos.
