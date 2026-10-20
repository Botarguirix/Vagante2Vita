# Iteración 128: ataque del Thief y método de reconstrucción

El Thief del primer ACT_ONE ya puede atacar. El runtime baja su temporizador de
ataque, inicia un golpe cuando el jugador está a menos de 11 px horizontal y 6
px verticales, aplica 1–2 puntos una sola vez durante el golpe y respeta la
recarga de 60 ticks. Ghidra confirma esos umbrales, los valores del constructor
y `attackCooldownTimer=60` en `Thief::runAI` / `Thief::attack`
(`_local/decompiled/007cc5d0.c`, `007ce5a0.c`, `007c8370.c`). La ventana de 8
ticks y la caja de impacto frontal son aproximaciones locales: VWE3 no aporta
la visibilidad nativa, el estado del arma ni sus polígonos de animación. Esto
no se etiqueta como paridad nativa.

`decomp.wiki` (Decompedia) describe principalmente decompilación matching:
reconstruir código que genere el mismo binario con el compilador/configuración
originales. No promete recuperar el código fuente; además, sus patrones de
compilador son específicos de plataforma. Para el proyecto usamos esa
distinción para clasificar evidencia por función, conservar los pseudocódigos
locales de Ghidra como referencia y llamar aproximaciones a los comportamientos
que requieren estado no presente en nuestros paquetes. No hay evidencia aquí
de que Vagante pueda reconstruirse byte por byte ni de que los ejemplos GCC de
la wiki apliquen a su ejecutable Windows.

## Validación

- `tools/run_host_tests_windows.ps1`: 12/12 pruebas C pasan, incluida la prueba
  del ataque, daño único y cooldown del Thief.
- `python -m unittest discover -s tests -v`: 194/194.
- El paquete privado regenerado en `_local/iteration128/campaign` valida sus
  142 archivos. Las rutas físicas seed 1/49/4 vuelven a completar en 438/890/
  546 ticks; se actualizaron los route proofs porque el hash del simulador
  cambió al añadir el estado de combate del Thief.
- VPK 1.28: `_local/build-act-one-128/Vagante2Vita-iteration128.vpk`,
  837,098 bytes, SHA-256
  `f1f113d32046d49b1ce5bd1ba00501dbb7f6a2ff10f71ebefc224ccb73e45a72`.
  ZIP CRC correcto; incluye `VAGV00006`, `Vagante2Vita`, APP_VER `01.28` y
  únicamente las siete entradas de aplicación permitidas.
- Vita3K 0.2.1.4190, perfil de prueba separado, OpenGL/Qt/CLI, cargó el replay
  de campaña pero expiró tras 45 s antes de crear un log de sesión nativa.
  Resultado en `_local/vita3k-act-one-127/runs/iteration128/run-000003`;
  stdout/stderr/log del emulador están disponibles ahí. La ejecución Vita3K
  sigue sin validar el VPK en este equipo.

## Referencias de Decompedia

- [Decompedia](https://decomp.wiki/) explica la meta matching y enlaza recursos
  comunitarios como decomp.dev, decomp.me y Ghidra compartido.
- [Compiladores GCC](https://decomp.wiki/compilers/GCC) muestra por qué los
  patrones de generación de código dependen de versión/plataforma; sus ejemplos
  de consolas antiguas no se transfieren directamente al binario Windows de
  Vagante.
- [Shiftability](https://decomp.wiki/resources/glossary/shiftability) explica
  que reorganizar código tras dividir un ejecutable requiere tratar con cuidado
  las reubicaciones; esto no es hoy un objetivo de nuestro port independiente.
