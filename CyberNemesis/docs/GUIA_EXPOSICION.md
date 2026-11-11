# Guía para la exposición técnica — CYBER-NEMESIS

Material para quien presenta el juego: qué decir, cómo prepararlo y cómo responder preguntas.
Los textos y números del juego están en `src/core/Story.h` y `src/core/Config.h`; adáptalos a los objetivos reales de tu proyecto.

## 1. El proyecto en 30 segundos

> «La IA suele explicarse con fórmulas y gráficos, y por eso parece magia. Este proyecto deja que **veas aprender a una IA mientras juegas**.
> Tú eres el jefe final; tu rival es una IA que mide cómo peleas (telemetría), fabrica una contramedida contra tu estilo y,
> cuando muere, **vuelve mejorada**. Morir y volver es, literalmente, entrenar: probar, fallar, analizar, mejorar.»

Tres ideas que el visitante debería llevarse:
1. **Los datos mandan:** la IA solo aprende de lo que tú haces.
2. **Morir y volver es entrenar:** error → análisis → mejora.
3. **Variar es la mejor defensa:** si cambias de estilo, sus contramedidas pierden valor (sobreajuste).

## 2. Preparación

1. Compilar (ver `README.md`): `cmake -S . -B build && cmake --build build --config Release` (Windows: `build_windows.bat`).
2. Abrir el juego a pantalla completa: `./build/CyberNemesis --fullscreen` (o **F11** ya dentro).
3. Comprobar: volumen (**M** apaga/enciende la música), teclado, y que la pantalla de inicio muestre los controles.
4. Hacer **una partida de ensayo** completa y medir cuánto dura: depende de lo bien que juegue cada visitante.

Checklist rápido: ☐ compilado ☐ pantalla completa ☐ sonido ☐ ensayo hecho ☐ cartel con el pitch de 30 s ☐ memoria de la IA borrada.

## 3. Consejos para presentarlo

- Es **el juego completo**, sin versión aparte: prólogo, 6 generaciones del Héroe, el Serafín con sus 4 vidas y el resumen final.
- **La IA recuerda al jugador anterior** (se guarda en `brain.data`). Eso también es una demostración: «mira, aún se acuerda de cómo jugaba la persona de antes». Para empezar desde cero con un visitante nuevo, usa **BORRAR MEMORIA DE LA IA** en el menú (se pulsa dos veces). Al terminar la partida completa, la memoria se reinicia sola.
- Si el visitante tiene poco tiempo, **F8** salta directo al Serafín; **F7** lo mata al instante para enseñar su muerte y su regreso (no borra tu progreso real al ganar).
- Para quien quiera entender el «por dentro», el **Archivo** del menú tiene 20 páginas (6 del proyecto, 6 de cómo aprende la IA, 5 técnicas, 2 de datos curiosos y 1 del código).

## 4. Demo guiada de unos 3 minutos (si el visitante no quiere jugar solo)

| Paso | Qué haces | Qué explicas |
|---|---|---|
| 1 | Menú → **ARCHIVO** → páginas 1–3 | Por qué un juego y por qué una IA que aprende. |
| 2 | **Iniciar combate**; dispara de lejos sin parar | «Estoy dándole datos: la IA cuenta mis disparos.» Pulsa **H**: se ve la telemetría en vivo. |
| 3 | Deja que el Héroe te venza | La cinemática: «Disparaste el X % de las veces, así que fabrica un escudo.» Es una **regla de umbral** (> 55 %). |
| 4 | Segunda pelea: el Héroe llega con escudo | «Contramedida a medida. Ahora cambio de estilo»: pasa al cuerpo a cuerpo y mira cómo el escudo deja de servir. |
| 5 | **F8**: saltar al Serafín | Un enemigo que muere y vuelve. Cada vida mejora ataque, esquiva y vida, y aprende mecánicas (copias, orbes). |
| 6 | **F7**: matar al Serafín | Tras volver habla: diagnostica **de dónde vino el daño que lo mató** (dato real) y suelta un dato de IA. |
| 7 | Archivo → páginas **TÉCNICO** | Reglas, cómo se mide el estilo, memoria, limitaciones y trabajo futuro. |

Extras que invitan a participar:
- En la cinemática del Héroe, el visitante **elige qué responderle a la IA** (hay respuestas que enseñan y otras que no, y la IA y la guía explican por qué). Es un buen momento para preguntar al público qué contestaría.
- Los **datos curiosos** (Pac-Man, Deep Blue, AlphaGo...) salen en la pausa, en la pantalla de derrota y en los diálogos; sirven para romper el hielo.

## 5. Qué enseña cada parte del juego

| Lo que se ve | Concepto de IA real |
|---|---|
| Tus ataques, saltos y movimientos quedan medidos | Recolección de datos / telemetría / *dataset* |
| El Héroe llega con una contramedida a tu patrón | Adaptación: ajustar parámetros según los datos |
| El Héroe y el Serafín mueren y vuelven mejorados | Iteraciones de entrenamiento (episodios / épocas) |
| El Serafín diagnostica de dónde vino el daño | Análisis de errores |
| Señuelos y orbes guardianes | Redundancia: defensa en capas |
| Si cambias de estilo, la contramedida falla | Sobreajuste vs. generalización |
| Esquiva con porcentaje | Modelos probabilísticos |
| `brain.data` | Guardar un modelo entrenado (memoria persistente) |

## 6. Preguntas frecuentes del público técnico

**¿Es una red neuronal?**
No. Son reglas con umbrales sobre la telemetría, suavizadas con una media móvil exponencial. Es una versión simplificada del mismo ciclo (observar, medir, ajustar). El juego lo dice abiertamente.

**¿Qué algoritmo usa exactamente?**
`PatternTracker` mide la partida; `AdaptiveBrain::Adapt` aplica las reglas cuando el Héroe cae: disparos a distancia > 55 % → escudo; cuerpo a cuerpo > 55 % → dash evasivo y velocidad; predecibilidad > 60 % → puntería predictiva; altura > 50 % → propulsores. Además cada generación sube vida (+45), daño (×1,18), cadencia (×0,92) y esquiva (+7 %, máx. 85 %). El Serafín es un planificador de turnos (`FinalDirector`) con una tabla de parámetros por vida (vida, ataque, esquiva, invocaciones); hereda lo aprendido por los Héroes y mejora cada vez que muere.

**¿Cómo se mide «predecibilidad»?**
`0,65 × (proporción de muestras con el mismo movimiento) + 0,35 × (cuánto dominas una misma zona de la arena)`.

**¿Por qué no usaron aprendizaje por refuerzo de verdad?**
Porque un visitante juega minutos, no miles de episodios, y las reglas permiten **explicar cada decisión** en pantalla. Un trabajo futuro natural es sustituir los umbrales por un bandido multibrazo o Q-learning sobre la misma telemetría.

**¿Dónde se guarda lo aprendido?**
En `brain.data`, con cabecera (marca y versión) y validación al cargar: si está corrupto o es de otra versión, se descarta y la IA empieza de cero. Se conserva entre visitantes salvo que uses «Borrar memoria».

**¿Se puede engañar a la IA?**
Sí, y es el punto: si cambias de estilo, las contramedidas pierden valor (sobreajuste). Con pocos datos (menos de 6 ataques) solo mejoran las estadísticas base.

**¿Qué limitaciones tiene?**
Reglas fijas (no un modelo entrenado), pocos datos por pelea, reacción solo al pasado inmediato y balance no validado con muchos jugadores. Por eso el Archivo incluye una página de «límites y trabajo futuro».

**¿Por qué un videojuego?**
Porque aprender haciendo se queda más que escuchar, porque el visitante **es** el *dataset*, y porque equivocarse aquí no cuesta nada (en el mundo real, ajustar mal una IA puede salir caro).

**¿Con qué está hecho?**
C++17 y Raylib, CMake. No usa imágenes ni sonidos externos: los sprites se dibujan por código y la música se sintetiza.

**¿Esto se parece a las IA que usa la gente (chatbots, etc.)?**
En que aprenden de datos y se equivocan. Aquí la IA «aprende en línea» durante la partida con reglas muy simples; los modelos grandes se entrenan antes con enormes cantidades de datos. Lo que sí comparten es la lección: **sin buenos datos y sin ver errores, no hay aprendizaje**.

## 7. Teclas del operador

| Tecla | Acción |
|---|---|
| **F8** | Saltar directo al Serafín (no borra la memoria al ganar) |
| **F7** (en combate) | Matar al Héroe / Serafín al instante |
| **H** | Mostrar / ocultar los datos de la IA (telemetría en vivo, vida, habilidades) |
| **M** | Música on/off |
| **F10** | Intensidad de efectos de pantalla |
| **F11** | Pantalla completa |
| **ESC** | Volver al menú (en el menú, salir) |

## 8. Solución de problemas

- **Un visitante dejó la partida a medias:** **ESC** vuelve al menú; después **BORRAR MEMORIA DE LA IA** si quieres que el siguiente empiece desde cero.
- **Va lento:** baja los efectos con **F10**.
- **Sin sonido:** comprueba **M** y el volumen del sistema.
- **Se ve raro en Linux con Wayland:** prueba en X11 con `env -u WAYLAND_DISPLAY ./build/CyberNemesis`.
