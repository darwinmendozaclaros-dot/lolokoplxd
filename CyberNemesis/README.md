# CYBER-NEMESIS : Adaptive Boss Protocol

Juego 2D en **C++17 + Raylib** donde tú eres el **Jefe Final** (128x128) y el enemigo es un
**Héroe controlado por IA adaptativa** (64x64) que aprende de tus patrones y resucita más fuerte.

## Plataformas soportadas
| Sistema | Backend de ventana |
|---|---|
| Windows 10/11 | Win32 (nativo) |
| Linux con Wayland | Wayland **nativo** |
| Linux con X11 / XWayland | X11 |

En Linux el binario incluye **ambos** backends: al arrancar usa Wayland si hay una sesión Wayland y, si no, X11. Al configurar, CMake imprime una línea `backends de ventana -> Wayland: ON | X11: ON`; si alguno sale `OFF`, te faltan paquetes de desarrollo (ver abajo).

## Compilar

Requisitos: compilador C++17, CMake >= 3.14 y Git. Raylib 5.5 se descarga y compila sola la primera vez (hace falta internet).

```bash
cmake -S . -B build
cmake --build build --config Release
./build/CyberNemesis            # Windows: build\Release\CyberNemesis.exe
```
Si ya habías compilado antes sin Wayland, borra la carpeta `build` (`rm -rf build`) para que se regenere Raylib con los dos backends.

### Dependencias en Linux (X11 + Wayland)
- **Void**: `sudo xbps-install -S base-devel cmake git pkg-config MesaLib-devel wayland-devel wayland-protocols libxkbcommon-devel libX11-devel libXrandr-devel libXi-devel libXcursor-devel libXinerama-devel alsa-lib-devel`
- **Debian/Ubuntu**: `sudo apt install build-essential cmake git pkg-config libwayland-dev wayland-protocols libxkbcommon-dev libegl1-mesa-dev libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev libxcursor-dev libxinerama-dev libasound2-dev`
- **Arch**: `sudo pacman -S base-devel cmake git pkgconf wayland wayland-protocols libxkbcommon mesa libx11 libxrandr libxi libxcursor libxinerama alsa-lib`
- **Fedora**: `sudo dnf install gcc-c++ cmake git pkgconf wayland-devel wayland-protocols-devel libxkbcommon-devel mesa-libEGL-devel mesa-libGL-devel libX11-devel libXrandr-devel libXi-devel libXcursor-devel libXinerama-devel alsa-lib-devel`

Opcional en GNOME/Wayland: instalar `libdecor` para tener barra de título nativa.

### Opciones de CMake
| Opción | Efecto |
|---|---|
| `-DCN_WAYLAND=OFF` / `-DCN_X11=OFF` | Compila sin ese backend (`AUTO` por defecto, `ON` para forzar) |
| `-DCN_USE_SYSTEM_RAYLIB=ON` | Usa la Raylib del sistema (suele venir **sin** Wayland) |

Para probar X11 dentro de una sesión Wayland: `env -u WAYLAND_DISPLAY ./build/CyberNemesis`.

Ejecuta el juego desde la carpeta del ejecutable (ahí se copia `assets/` y se crea `brain.data`).

## Obtener el .exe y compartirlo
El `.zip` del proyecto contiene el **código fuente**: un `.exe` solo existe después de compilar una vez (no se puede crear sin un compilador). Estas son las formas de conseguirlo, de la más fácil a la más técnica. En todas, el resultado queda en la carpeta **`release/`**, con `CyberNemesis.exe` (autocontenido: sin DLLs, sin carpetas auxiliares) y `CyberNemesis-windows-x64.zip` listo para compartir.

1. **Windows, doble clic**: ejecuta `build_windows.bat`. Comprueba las herramientas, te dice exactamente qué comando usar si falta alguna, compila, crea `release\` y la abre al terminar. Si tienes Inno Setup instalado, también genera `CyberNemesis-Setup.exe`.
2. **En la nube, sin instalar nada**: sube el proyecto a GitHub. `.github/workflows/build.yml` compila en Windows y Linux y deja los zips en *Actions > Artifacts* (con un tag `v1.0.0` los publica en *Releases*).
3. **Desde Linux hacia Windows**: `./build_windows_desde_linux.sh` (necesita MinGW-w64; ver `cmake/toolchain-mingw-w64.cmake`).
4. **Linux nativo**: `./build_linux.sh`.

**Instalador (Inno Setup)**: ábrelo *después* de generar el `.exe`. Si no existe `release\CyberNemesis.exe`, el instalador se detiene con un mensaje claro que te dice qué hacer. Genera `release\CyberNemesis-Setup.exe`.

**Sin rutas ni nombres de usuario**: el CMake reemplaza las rutas de tu equipo dentro del binario (`-ffile-prefix-map` / `/d1trimfile`) y elimina los símbolos de depuración en Release (`-s`). El código fuente no contiene rutas absolutas. Las partidas se guardan en `%APPDATA%\CyberNemesis\brain.data` (Linux: `~/.local/share/CyberNemesis`); con `CYBERNEMESIS_SAVE_DIR=.` el juego guarda junto al `.exe` (modo portable).

Windows puede mostrar el aviso de SmartScreen al abrir un `.exe` sin firmar (el `LEEME.txt` explica cómo continuar). Si editas `assets/shaders/postfx.fs`, ejecuta `python3 tools/embed_shader.py` para volver a incrustarlo.

## Controles
| Tecla | Acción |
|---|---|
| A / D | Mover (con aceleración) |
| W / Espacio | Saltar. Mantener = salto alto, soltar = salto corto. **Doble salto** en el aire |
| Shift / L | **Dash** (suelo o aire) |
| S / Flecha abajo | Bajar de una plataforma |
| J / Clic derecho | Aplastar (AoE con aviso). **En el aire = picado** que aplasta al tocar suelo |
| K / Clic izquierdo | Disparar plasma hacia el mouse |
| P / M / F11 | Pausa / Música / Pantalla completa |
| ESC | En partida: volver al **menú**. En el menú: salir |
| H | Muestra / oculta los **datos de la IA** (numeros de vida, telemetria en vivo, habilidades del Heroe, estadisticas del Serafin). Por defecto la pantalla va limpia |
| F10 | Intensidad de los efectos de pantalla: apagados / **suaves (por defecto)** / medios / intensos |

## FASE FINAL: el Serafín (estilo Undertale / Deltarune)
Al derrotar a la **última generación** del Androide ya no ganas: se niega a caer y **resucita como el Serafín**, una forma alada dorada con 900 de vida que **vuela alrededor y también camina**. El Boss recupera toda su vida y empieza un combate por **turnos de balas**:

- **Estructura**: turno de ataque (7 a 9 s) → descanso en el que el Serafín baja y queda expuesto → siguiente turno. Cada 2 turnos alterna entre **VUELO** (ataca desde el aire) y **TIERRA** (camina y ataca desde el suelo).
- **10 patrones**: anillos expansivos, espiral giratoria, lluvia con carril libre, abanico apuntado, olas senoidales, **cañones tipo Gaster Blaster** (apuntan, cargan y disparan un rayo), muros de balas con hueco, ondas a ras de suelo, esferas rebotantes y pilares de energía.
- **3 niveles** (100 % / 66 % / 33 % de vida): más balas, más rápidas, cañones giratorios y, en el último, combinaciones de dos patrones a la vez. Al subir de nivel se despejan las balas y aparece un núcleo de curación.
- **Reglas de esquiva**: tras recibir una bala tienes **0.7 s de invulnerabilidad** (parpadeas); el **dash (Shift) es invulnerable**; el aplastamiento (J) **borra las balas cercanas**. Los rayos y balas avisan antes de hacer daño (círculos, líneas rojas, columnas fijas un instante).
- Los eventos de fondo y de combate se desactivan en esta fase para que las balas se lean bien. Si te derrotan, ENTER reintenta directamente la fase final.
- Solo al vencer al Serafín aparece la pantalla de victoria.

## Responder a la IA, datos curiosos y cinematicas
- **Respuestas elegibles**: en la cinematica en la que el Heroe se reconstruye, tras decirte que patron detecto, **eliges que responderle** (W/S o flechas + ENTER, o las teclas 1-3). Hay 13 escenas segun tu estilo (distancia, cuerpo a cuerpo, predecible, altura, sin patron). Cuando el Serafin vuelve, la pelea espera un momento y le respondes tambien (3 escenas con datos reales de su muerte). La IA y la guia contestan a cada opcion explicando algo; el resumen final cuenta cuantas veces elegiste una respuesta que ensena. Solo tiene efecto narrativo: no cambia el balance.
- **Datos curiosos** (26, sin repetirse): salen en la pausa, en la pantalla de derrota, en los dialogos de la guia (rotan con los consejos y las notas del proyecto) y en 2 paginas nuevas del Archivo.
- **Cinematicas mas dinamicas**: camara con zoom y paneo (acercamiento hacia donde se reconstruye el Heroe, empuje lento hacia el Boss en la fase final, golpe de camara al caer), **tus numeros de telemetria vuelan hacia el Heroe** mientras se arma, destellos, barras de cine y transiciones con fundido entre pantallas.

## Efectos visuales extra
Todos escalan con **F10** (efectos de pantalla); en "APAGADOS" no se muestra ninguno de los nuevos.
- **Brasas ambientales**: motas de energia que suben por la arena, con el color del tema de cada generacion.
- **Pulso de peligro**: con la vida de NEMESIS-X por debajo del 30 %, los bordes de la pantalla laten en rojo (mas fuerte cuanto menos vida).
- **Impactos**: destello rojo en los bordes y anillo de energia al recibir dano; anillo tambien cuando el Heroe / Serafin es golpeado y onda de impacto en caidas fuertes.
- **Lineas de velocidad** durante el dash.
- **Aura del Serafin**: plumas doradas que caen y estela de imagenes residuales cuando se mueve.
- **Columna de luz** (cian para el Heroe, dorada para el Serafin) y anillos de energia al aparecer al empezar cada ronda.
- **Sobrecarga**: borde dorado pulsante mientras NEMESIS-X esta sobrecargado.

## Calidad general
- **Imagen**: transiciones con fundido, vineta en los bordes, pausa rediseñada (controles + dato curioso), destellos y golpes de camara en los momentos clave.
- **Menu**: cada opcion tiene ahora una descripcion.
- **Manejo del personaje**: el dash se "guarda" si lo pulsas un instante antes de que recargue (buffer), giro mas agil al cambiar de direccion, "hang time" en lo alto del salto, polvo al aterrizar y chispas durante el dash. (Ya existian coyote time, buffer de salto, salto variable, doble salto, estela del dash, estirar/aplastar del sprite y sombras.)

## Para presentarlo en una exposicion
No hay una version aparte: todo esta en el juego completo. Lo pensado para que un visitante lo entienda rapido:
- **Pantalla de inicio** con frases que rotan y los controles siempre visibles; **prologo** que explica la idea del proyecto.
- **Archivo** (menu principal): 20 paginas, 6 sobre por que existe el proyecto y 5 **tecnicas** (reglas, telemetria, memoria, el Serafin y limites del sistema).
- **Dialogos con humor y datos reales de IA** durante las peleas, que nunca se repiten.
- **Resumen final** al vencer al Serafin: lo que la IA aprendio de ese jugador (estilo, contramedidas, veces que murio y volvio, reglas reales).
- **Memoria persistente**: la IA recuerda a quien jugo antes (`brain.data`). Para empezar de cero con un visitante nuevo usa **BORRAR MEMORIA DE LA IA** en el menu (se pulsa dos veces); al terminar la partida completa tambien se reinicia.
- **Teclas de ayuda para el presentador**: **F8** salta directo al Serafin y **F7** mata al Heroe / Serafin (para ensenar su muerte y regreso).
- **Guia del presentador** con pitch de 30 s, demo de 3 minutos y preguntas frecuentes: [`docs/GUIA_EXPOSICION.md`](docs/GUIA_EXPOSICION.md).
- Opcion de arranque `--fullscreen` (o F11 dentro del juego).

## Por que este juego (proyecto educativo)
La IA suele explicarse con formulas y graficos, y por eso parece magia. Este proyecto prueba otra idea: **dejar que la veas aprender mientras juegas**. Un jefe final contra una IA que se adapta es un laboratorio disfrazado de videojuego:
- **Aprender haciendo**: no lees que una IA se adapta, lo notas cuando el Heroe llega con un escudo justo contra tu estilo.
- **Tu eres el dataset**: la IA solo aprende de lo que haces; cambia tu estilo y observa como cambia ella (metodo cientifico con explosiones).
- **Morir y volver = entrenar**: probar, fallar, analizar, mejorar y repetir. Cada vida del Heroe y del Serafin es una iteracion.
- **Un sitio seguro para equivocarse**: en el mundo real ajustar mal una IA puede salir caro; aqui solo cuesta un Heroe.

| Lo que ves en el juego | Concepto de IA real |
|---|---|
| Tus ataques, saltos y movimientos quedan medidos | Recoleccion de datos / telemetria / dataset |
| El Heroe llega con una contramedida a tu patron | Adaptacion: ajustar parametros segun los datos |
| El Heroe y el Serafin mueren y vuelven mejorados | Iteraciones de entrenamiento (episodios / epocas) |
| El Serafin diagnostica de donde vino el dano que lo mato | Analisis de errores |
| Senuelos y orbes guardianes | Redundancia: defensa en capas |
| Si cambias de estilo, la contramedida falla | Sobreajuste vs. generalizacion |
| Esquiva con porcentaje | Modelos probabilisticos |
| `brain.data` | Guardar un modelo entrenado (memoria persistente) |

**Honestidad**: esta IA **no es una red neuronal**. Usa reglas con umbrales sobre tu telemetria (por ejemplo, si disparas mucho de lejos, crea un escudo). Es una version simplificada del mismo ciclo: observar, medir, ajustar. Las IA reales usan muchisimos mas datos y calculos. El juego lo dice abiertamente.

Donde se cuenta esto dentro del juego: el **prologo**, el **Archivo** (menu principal, 20 paginas: las 6 primeras explican el proyecto y 5 son tecnicas) y **notas de la guia** durante las peleas (se alternan con los consejos y nunca se repiten). Al caer el Serafin definitivamente hay un **cierre** del proyecto. Todos los textos estan en `src/core/Story.h` (`Prologue`, `ProjectNote`, `ProjectOutro` y `Codex`) y se pueden adaptar al motivo real de tu proyecto.

## Lore y aprendizaje de la IA (educativo)
El juego cuenta una historia y, a la vez, ensena como aprende una IA al morir y volver:

- **Prologo** (primer arranque, sin memoria previa): ANO 2187, la instalacion Cyber-Nemesis crea el *Protocolo Adaptativo* para eliminar a NEMESIS-X. Se salta con TAB.
- **Cuando el Heroe cae** ya no hay pantalla de estadisticas: se ve una **cinematica** (barras de cine) en la que el Heroe se reconstruye con fragmentos que convergen, reaparece y habla. Sus dialogos usan tu telemetria REAL ("disparaste desde lejos el 72% de las veces, asi que fabrico un escudo...") y un **CONCEPTO** distinto cada vez (telemetria, aprender de los errores, adaptacion, prediccion, memoria). ENTER salta la linea, TAB omite la cinematica.
- **Archivo** (menu principal, "ARCHIVO: COMO APRENDE LA IA"): 20 paginas. Las 6 primeras explican **por que existe el proyecto**; luego telemetria, aprendizaje por refuerzo, el Serafin, memoria persistente y como ganarle a una IA que aprende; la ultima cuenta como esta hecho el codigo. Se navega con A/D o las flechas.
- **Moraleja** al ganar: una IA solo aprende de los datos que tu le das.

## Dialogos en vivo (con humor y datos educativos)
Durante las peleas aparecen dialogos abajo, sin pausar y sin pedir ninguna tecla (**ENTER** salta la linea actual). En la caja se ven **tres iconos a la vez**: el **jugador** (NEMESIS-X, casco con cuernos), la **IA** (casco del Heroe o, en la fase final, el Serafin con aureola y alas) y la **GUIA** (una bombilla: la voz que explica como aprende la IA). El que habla se ilumina y los otros se atenuan.
- **Al inicio**: el Heroe cuenta con que contramedidas llega (segun lo que la IA aprendio de ti), NEMESIS-X responde con una burla y la GUIA suelta un dato.
- **A mitad de pelea**: la primera esquiva, la primera invocacion, un bloqueo con escudo, un dash, al bajar la vida a la mitad... y, si hace rato que nadie habla, un **consejo** de la guia.
- **Al final**: ultimas palabras al caer, al ganar la IA y al caer el Serafin definitivamente.
- **Nunca se repiten**: cada linea se elige entre variantes que aun no se han dicho (`Story::Variety`); si se agotan, simplemente se calla. Se reinicia al borrar la memoria o al terminar la partida.
- Cada dialogo mezcla humor con algo real sobre IA: telemetria, sobreajuste, redundancia, aprendizaje por refuerzo, sesgo, memoria...

## Pantalla limpia
Por defecto el HUD solo muestra las barras de vida y los cooldowns **mientras recargan**. Los numeros de vida, la telemetria en vivo, las habilidades del Heroe y las estadisticas del Serafin salen con la tecla **H**. Mientras se habla se ocultan las etiquetas de eventos de arriba.

## El Serafin: muere, vuelve y lo cuenta
No hay pantalla de derrota ni de estadisticas. El Serafin **hereda lo aprendido por los Heroes** (desde su primera vida ya esquiva e invoca) y **muere 4 veces**; cada vez que cae **vuelve enseguida a la pelea con sus mejoras** y lo cuenta con subtitulos mientras pelea:
1. **Diagnostico real**: de donde vino el dano que lo mato (plasma / cuerpo a cuerpo / otros), cuantos golpes esquivo y cuantos disparos bloquearon sus invocaciones. Lo mide el juego durante la pelea.
2. **Lo que mejoro**, dicho por el propio Serafin: ataque, esquiva, vida y mecanicas nuevas.
3. Una leccion de la **GUIA** (aprendizaje por refuerzo, analisis de errores, sobreajuste).

| Vida del Serafin | Vida | Ataque | Esquiva | Invocaciones para esquivar |
|---|---|---|---|---|
| 1 (al empezar la fase final) | 300 | 70 % | 12 % | **Senuelo**: copia holografica que absorbe 1 disparo (recarga 8 s). Ya conoce lluvia, muros, espirales y pilares |
| 2 | 380 | 80 % | 20 % | Senuelos resistentes (absorben 2) + **2 orbes guardianes** + canones + escudo si el Heroe lo habia aprendido |
| 3 | 440 | 90 % | 28 % | **Clones de espejo** (2 senuelos a la vez) + 3 orbes |
| 4 (forma completa) | 520 | 100 % | 35 % | 4 orbes + combinaciones de dos patrones |

- **Ataque**: escala la cantidad de balas, su velocidad, la cadencia y el dano, y desbloquea patrones. Los huecos de lluvia y muros son mas amplios al principio.
- **Esquiva pasiva**: probabilidad de que un golpe del Boss falle; el Serafin se desplaza hacia el centro, queda invulnerable ~0.35 s y aparece **ESQUIVA!**.
- **Invocaciones**: las usa cuando detecta un disparo tuyo yendo hacia el (o si estas muy cerca). **Tu aplastamiento (J) las destruye** y puedes agotarlas a base de disparos. El HUD muestra la recarga.
- Sin fases ni niveles: durante una vida su dificultad es fija; solo mejora al morir. Aparece un nucleo de energia para ti cada ~30 s en la fase final.
- **Teclas de debug**: en el menu, **F8** salta directo a la fase final (no borra tu memoria al ganar) y, en combate, **F7** mata al Heroe / Serafin para ver su muerte y su regreso.
- Si el Boss cae en la fase final, ENTER reintenta con el **mismo Serafin** (el progreso se conserva hasta volver al menu).
- Balance en `Seraph::ForCycle` (`src/core/Config.h`); invocaciones en `src/entities/Summons.*`; dialogos y lore en `src/core/Story.h`; subtitulos en vivo en `src/gfx/Subtitles.*` y `src/gfx/Portraits.*`; cuadro bloqueante del prologo en `src/gfx/Dialogue.*`.

## Menú y sprites (última versión)
- **Menú principal** con fondo animado tipo *aero shards*: cientos de fragmentos de cristal que fluyen en cintas diagonales que se cruzan, con profundidad de campo (los lejanos se desenfocan en nubes), marcas "+", una retícula de enfoque que salta entre fragmentos, parallax con el mouse y viñeta. Su color se tiñe con el tema de la generación actual del Héroe.
  Opciones (teclado o mouse): iniciar combate, efectos de pantalla, música, pantalla completa, borrar memoria de la IA (pide confirmación) y salir. Muestra la memoria del Héroe (generación, habilidades, arsenal) y a los dos combatientes en reposo.
- **Sprites rediseñados a 32x32** (antes 16x16) con sombreado biselado automático, contorno y una capa **emisiva** aparte (ojos, núcleo, neones) que brilla y pulsa.
- **Animación más fluida**: el Boss tiene 9 poses (reposo, 2 de marcha, salto, caída, preparación, golpe, disparo, picado) y el Héroe 14 (2 posturas de brazo x 7 de piernas, con ciclo de carrera de 4 fotogramas). Encima se aplican: paso sincronizado con la distancia recorrida, respiración, inclinación al moverse, *squash & stretch* al saltar y aterrizar, rebote al caminar, y una bufanda con física en el Héroe.
- **Más detalle en todo lo demás**: plataformas con paneles, remaches, franjas de peligro, luz corredora y propulsores con llama; proyectiles propios para cada tipo (plasma con chispas orbitando, dardos, perdigones, misiles con aletas y estela); drones con rotores difuminados; minas con cúpula y pinchos; núcleo de energía facetado.

## Última actualización
- **Shaders más suaves**: el post-procesado (curvatura CRT, aberración cromática, bloom, scanlines, viñeta y glitch) ahora se escala con un único valor. Por defecto está al 35 %; con **F10** pasas a apagado, suave, medio o intenso (el original). El relámpago de las tormentas también es menos cegador.
- **Oleadas más largas**: el Héroe parte con 220 de vida (antes 100) y gana +45 por generación (hasta 445 en la última).
- **Fases del Héroe**: al bajar de 66 % y 33 % de vida cambia de fase: se cura un 12 %, es invulnerable ~1.6 s, entra en sobrecarga (más rápido y dispara más) y empuja al Boss. Fase 2 = refuerzos con drones; fase 3 = bombardeo. Las marcas aparecen en su barra de vida.
- **Eventos secundarios en mitad del combate** (cada 17–27 s, aleatorios):
  - **Bombardeo**: columnas naranjas que dañan al Boss **y** al Héroe. Hay que salir de las zonas marcadas.
  - **Drones de refuerzo**: disparan al Boss; se derriban con plasma, aplastamiento o embistiéndolos.
  - **Flujo de gravedad**: gravedad reducida ~9 s (saltos mucho más altos).
  - **Núcleo de energía**: aparece sobre una plataforma (se mueve con ella). Solo el Boss lo recoge: cura 140 y da **sobrecarga** 8 s (+40 % de daño, recargas más rápidas).
- **El Boss se encoge con cada generación**: 96 px → 86 → 77 → 67 → 58 → 48 px. Cuanto más pequeño, más difícil de acertar y más veloz; el alcance del golpe crece para compensar. El encogimiento se anima durante la cuenta atrás de cada oleada y se muestra el porcentaje de tamaño junto a la barra del Boss.

## Novedades de esta versión
- **Boss más pequeño (96x96) y ágil**: doble salto, dash, coyote time, buffer de salto, salto variable y picado.
- **6 plataformas** que van y vienen (vaivén horizontal, ascensor vertical, una alta que cruza todo el mapa y dos cornisas fijas). Arrastran a quien esté encima.
- **Fondo vivo**: un tema de color por generación del Héroe y eventos aleatorios (tormenta eléctrica con relámpagos, lluvia de datos, meteoros, patrulla aérea con reflector y apagón), más farolas, cajas, carteles holográficos, antenas y drones.
- **Arsenal evolutivo del Héroe** (se desbloquea por generación o como respuesta a tu estilo):

| Ataque | Garantizado en | También se desbloquea si... |
|---|---|---|
| Ráfaga de 3 | Gen 2 | |
| Escopeta de plasma | Gen 3 | abusas del cuerpo a cuerpo |
| Misiles teledirigidos | Gen 4 | abusas del disparo, o eres muy móvil / te refugias en las alturas |
| Ataque orbital | Gen 5 | tu movimiento es predecible |
| Minas de proximidad | Gen 6 | abusas del cuerpo a cuerpo |

  Además: propulsores de salto (doble salto) y uso de plataformas como terreno elevado.
- Contrajuego: tu plasma derriba misiles, tu aplastamiento desactiva minas, saltar sobre una mina la evita y el ataque orbital se esquiva moviéndote durante su aviso.

## Cómo se mapea a la documentación
| Documento | Código |
|---|---|
| Módulo de Telemetría (`PatternTracker`) | `src/ai/PatternTracker.*` (melee vs. distancia, dirección de movimiento, saltos, dashes, tiempo en plataformas, zonas, predecibilidad) |
| Módulo de Adaptación y Decisión | `src/ai/AdaptiveBrain.*` → `Adapt()` |
| Distancia > 55 % → Escudo de Deflexión Neón (-70 % daño proyectiles) | `AdaptiveBrain::Adapt` + `Hero::TakeDamage` |
| Melee > 55 % → más velocidad + Dash Evasivo automático | `AdaptiveBrain::Adapt` + `Hero::StartDash` |
| Movimiento predecible → Apuntado de Predicción (Lead Shooting) | `AdaptiveBrain::Adapt` + `Hero::Fire` |
| Módulo de Persistencia (`brain.data`) | `AdaptiveBrain::Save/Load` (binario con cabecera + versión) |
| Boss se regenera al derrotar al Héroe | `Game::BeginReboot` / `Game::UpdateReboot` (cinematica) |
| Victoria del Boss = vencer todas las fases | `Cfg::MAX_GENERATION` (6) |
| Derrota del Boss = Héroe drena su vida en una ronda | estado `BossDown` |

## Estructura
```
src/
  main.cpp
  core/      Config.h (balance), Util.h, Game.* (bucle + máquina de estados)
  entities/  Entity.*, Arena.* (plataformas), Boss.*, Hero.* (IA de combate), Hazards.* (minas/orbital), Projectile.*, Summons.* (senuelos y orbes del Serafin)
  ai/        HeroTraits.h, PatternTracker.*, AdaptiveBrain.*
  gfx/       Sprites.* (pixel art 32x32), ShardField.* y MainMenu.* (menú), Particles.*, Background.*, PostFX.*, HUD.*
  audio/     AudioManager.* (sonido y música sintetizados, sin archivos .wav)
assets/shaders/postfx.fs   (CRT, bloom neón, aberración cromática, scanlines, glitch)
```

## Balance
Todo está en `src/core/Config.h` (vida, daño, cooldowns, umbrales 55 %/60 %, nº de generaciones).
El texto del juego usa ASCII (sin tildes) porque la fuente por defecto de Raylib no las incluye.
