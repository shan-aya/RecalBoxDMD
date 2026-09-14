# FAQ y solución de problemas

[🇬🇧 English](FAQ.md) · [🇫🇷 Français](FAQ.fr.md) · **🇪🇸 Español**

## ⚠️ Alimentación — léelo si tu DMD se congela o se corrompe

El DMD toma **toda** su alimentación del puerto/cable USB — no hay una entrada de alimentación separada para el panel LED. Una imagen clara enciende muchos más LED a máxima intensidad que una oscura, lo que hace que el consumo de corriente del panel suba notablemente cada vez que la pantalla cambia a algo claro. En un puerto/cable USB que no pueda seguir ese pico, los síntomas pueden parecer un error de firmware: la animación se congela en un fotograma, la imagen se corrompe/muestra píxeles extraños, o en casos más severos el DMD desaparece por completo del PC (el puerto serie/COM falla con un error de Windows) durante un instante.

**Si ves congelamientos, corrupción o desconexiones aleatorias — sobre todo si parecen ocurrir con GIF claros/luminosos — prueba, en este orden:**

1. **Usa un cable USB más corto/de mejor calidad.** Un cable fino o largo es la causa más común de caída de tensión USB.
2. **Aliméntalo desde un hub USB alimentado** en lugar de un puerto directo del PC/portátil, o un adaptador USB de 5V tipo cargador de móvil (no hace falta datos una vez que está funcionando, solo para flashear/configurar). Algunos puertos de PC simplemente no pueden suministrar suficiente corriente ante un pico repentino.
3. **Baja el brillo** desde la página de configuración web — un valor de `brightness` más bajo reduce directamente el consumo de corriente por LED, lo que reduce el tamaño del pico en contenido claro. No hace falta llegar hasta el 10%; el objetivo es solo tener margen respecto a lo que tu fuente USB concreta pueda suministrar de forma fiable.

Esto no es un error de firmware y volver a flashear no lo arreglará — es el presupuesto de potencia físico del USB. Una vez que encuentres una combinación de cable/fuente de alimentación/brillo estable para tu instalación, se mantiene estable — es algo que se resuelve una sola vez por DMD, no algo que vuelva a aparecer después.

## ⚠️ Revisión del chip ESP32 — algunas placas pueden ser más sensibles que otras

<!-- TODO (utilisateur) : completer avec les infos precises sur les revisions ESP32-D0WD-V3 concernees, les references produit ou marchands a eviter/preferer, et tout autre retour d'experience -->

No todos los módulos ESP32 son idénticos por dentro — Espressif ha fabricado varias **revisiones de silicio** del chip que lleva el módulo ESP32-D0WD-V3 habitual (por ejemplo v3.0 frente a v3.1) a lo largo de los años, cada una con su propio conjunto de defectos corregidos/introducidos a nivel de hardware. En la práctica, algunas revisiones parecen ser **más sensibles a condiciones de alimentación al límite** que otras — el congelamiento/corrupción descritos arriba pueden aparecer mucho antes (o nunca) dependiendo únicamente de qué revisión tenga tu placa concreta, incluso con exactamente el mismo firmware y la misma instalación USB que alguien que nunca ha visto el problema.

Puedes comprobar qué revisión anuncia tu placa durante el flasheo — la salida del Web Installer / `esptool` muestra una línea como `Chip is ESP32-D0WD-V3 (revision vX.X)`. Actualmente no hay forma de cambiar esto después (viene grabado en el propio chip físico), así que si tu placa resulta ser una revisión más sensible, los consejos de alimentación de arriba se vuelven más importantes para ti de lo que podrían serlo para el DMD de otra persona.

## La calidad de la tarjeta microSD importa

El firmware lee la tarjeta SD constantemente — cada GIF, la playlist y su propio archivo de configuración viven ahí — así que una tarjeta al límite o defectuosa puede causar una variedad sorprendente de síntomas que no tienen relación aparente con el firmware en sí: volver en bucle a la pantalla de configuración WiFi en lugar de conectarse, ajustes que no se guardan de verdad, la pantalla quedándose fija en una imagen, y más. Usa una tarjeta microSD genuina, razonablemente moderna, de una marca conocida — las tarjetas muy baratas/sin marca son con diferencia la causa más frecuente de este tipo de problemas.

**Si sospechas de un error de la tarjeta SD:**

- Apaga el DMD, quita la tarjeta SD y vuelve a insertarla firmemente. Esto por sí solo resuelve la mayoría de los fallos de lectura transitorios.
- Reinicia el DMD después si el problema persiste.

## La página de configuración web carga lento, o no carga

La pila WiFi/red del DMD puede estar genuinamente ocupada en un momento dado (hablando con Recalbox, dibujando en pantalla, atendiendo otras peticiones) — que una página de la interfaz de configuración web cargue lento, o que una acción de guardado tarde en responder, es normal de vez en cuando, no necesariamente señal de un problema.

**Buenos hábitos en ese caso:**

- **No pulses recargar/reintentar repetidamente.** Cada intento es una nueva petición que compite con lo que el DMD ya está haciendo — machacar la recarga tiende a acumular peticiones en vez de acelerar nada.
- **Espera a que tu propio navegador muestre un error** (un timeout, "no se puede acceder a esta página", etc.) antes de volver a intentarlo. Un solo reintento después de eso suele bastar.
- **Si aun así no carga, reinicia el DMD.** Un reinicio limpio generalmente restaura el acceso inmediato a la página de configuración web.

## Corte de WiFi breve / el DMD parece desincronizado de Recalbox

Un corte de WiFi corto y temporal no necesita ninguna acción por tu parte. El firmware detecta la reconexión por sí solo, generalmente en unos segundos, y resincroniza automáticamente la pantalla del DMD con lo que Recalbox esté haciendo realmente en ese momento (juego en curso, navegación, modo demo...) — no hace falta reiniciar nada, ni en el DMD ni en Recalbox, para que un corte así se resuelva solo.

## Vuelve en bucle al propio punto de acceso WiFi del DMD (modo AP)

Si, después de guardar tu red WiFi en la propia página de configuración cautiva del DMD, sigue volviendo a emitir su propio punto de acceso (`RecalBox-DMD-Config`) en lugar de unirse a tu red, la solución más simple es evitar esa pantalla por completo: usa en su lugar el **paso de Wi-Fi integrado en el Modo 1 de la caja de herramientas de PC**. Escanea tus redes, verifica la contraseña probando realmente la conexión desde tu PC, y la escribe directamente en `config.ini` en la tarjeta SD antes de que la tarjeta llegue siquiera al DMD — el DMD se une entonces a tu red desde su primer arranque, y la pantalla de portal cautivo nunca necesita aparecer.

Si de verdad necesitas usar la propia página de configuración del DMD y sigue en bucle: revisa bien la contraseña (un error de tecleo es la causa más habitual), y consulta "la calidad de la tarjeta microSD" más arriba — una tarjeta defectuosa puede impedir silenciosamente que se guarden los ajustes WiFi, incluso cuando la configuración en sí parecía haber ido bien.

## Dale a tu DMD una IP fija — elige UNA sola forma de hacerlo, nunca las dos

Algunas integraciones — los scripts del lado de Recalbox que hablan con el DMD, por ejemplo — lo alcanzan mediante una dirección IP fija. Si la dirección de tu DMD cambia con el tiempo (la mayoría de routers/box reparten direcciones dinámicamente por DHCP, y pueden asignar una distinta tras un reinicio o reconexión), cualquier cosa que dependa de la dirección antigua deja de funcionar silenciosamente.

Hay dos formas de darle una dirección fija:

- **Una reserva DHCP en tu router/box**, que asocia una dirección fija a la MAC WiFi del DMD — el DMD sigue pidiendo una dirección de forma normal, tu router simplemente le da siempre la misma.
- **Una IP estática configurada directamente en el DMD** (`wifi_static_enabled` / `wifi_static_ip` en `config.ini`, o los campos equivalentes en la página de configuración web) — el DMD se asigna la dirección él mismo, sin preguntarle nada al router.

**Configura solo una de las dos, nunca ambas a la vez.** Configurar una reserva DHCP en el router *y* una IP estática en el DMD puede hacer que apunten a direcciones distintas y en conflicto, o fallar de una forma confusa de diagnosticar. Elige la que te resulte más cómoda — una reserva desde el router suele ser más simple y mantiene toda la configuración de red en un solo sitio — y deja la otra sin tocar.

## Los scripts de Recalbox no parecen hacer nada tras una actualización

Si el DMD deja de reaccionar a lo que ocurre en Recalbox después de actualizar el firmware o la caja de herramientas de PC, los scripts del lado de Recalbox ya instalados pueden estar desactualizados. Reinstálalos con el **Modo 9** (o un nuevo **Modo 1**) desde la caja de herramientas de PC — esto también elimina automáticamente las versiones antiguas de los scripts. Consulta [UPGRADING.md](UPGRADING.es.md) si vienes de una versión anterior: algunas actualizaciones cambian el protocolo subyacente que usan los scripts y el DMD para hablar entre sí, así que los scripts realmente necesitan reinstalarse, no solo volver a flashear el firmware.

## Solución de problemas general

| Síntoma | Causa probable | Prueba esto |
|---|---|---|
| La animación se congela en un fotograma, a veces con artefactos visuales | Pico de corriente USB en una imagen clara (ver arriba) | Cable más corto, hub/adaptador alimentado, brillo más bajo |
| El DMD desaparece del PC (errores de puerto COM, "el dispositivo no funciona") durante una prueba por USB | Igual que arriba — lo bastante severo como para afectar al propio enlace USB, no solo a la pantalla | Igual que arriba |
| La pantalla muestra "RecalBox conectada" aunque Recalbox esté apagada | Error visual cosmético, corregido en firmware v210+ | Actualizar el firmware |
| La página de configuración web tarda en cargar o da timeout | La pila WiFi/red del DMD está ocupada en ese momento | Esperar el error del navegador, reintentar una vez; reiniciar si persiste |
| Pantalla brevemente desincronizada tras un corte de WiFi | Normal — el firmware resincroniza solo | No hacer nada, esperar unos segundos |
| Vuelve en bucle a la configuración WiFi, los ajustes no se guardan, la pantalla se queda fija | Tarjeta microSD al límite/defectuosa | Quitar y volver a insertar la tarjeta firmemente, reiniciar si hace falta; probar otra tarjeta |
| Vuelve en bucle sin parar al punto de acceso WiFi del propio DMD | Error de tecleo en la contraseña, o el guardado del portal cautivo no se aplicó | Usar en su lugar el paso de Wi-Fi del Modo 1; revisar la contraseña; ver la nota de la tarjeta microSD de arriba |
| Los scripts de Recalbox dejaron de funcionar tras una actualización | Versiones antiguas de los scripts todavía en Recalbox | Reinstalar mediante el Modo 9 (o un nuevo Modo 1) |

<!-- TODO (utilisateur) : ajouter d'autres entrees FAQ au fur et a mesure des retours -->
