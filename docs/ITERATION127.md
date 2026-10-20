# Iteración 127: campaña con rutas físicas y revisión de referencias públicas

La probe de movimiento encontró que el piso 1 del seed 31 tiene un camino de
tiles que no completa la ruta bajo la física del runtime: cae cerca del
waypoint 14 y termina fuera del terreno. La probe no carga el paquete de
enemigos, por lo que el resultado no demuestra que el Thief lo cause. Una
búsqueda controlada de seeds del mismo generador encontró el seed 49 como
candidato de prueba: mantiene un Thief tipo 84 y alcanza la salida en 890
ticks. Esto es una ruta de prueba, no paridad nativa del seed ni prueba de
combate/IA del Thief.

`tools/run_act_one_campaign_routes_windows.ps1` compila la probe C de Windows
una sola vez, ejecuta cada paquete VWI/VWE del campaign, registra logs y
trazas VTR1 y genera proofs V3 ligados por hash a los paquetes y al simulador.
El runner continúa después de un fallo para conservar el diagnóstico de los
tres pisos. En esta iteración, los pisos 0/1/2 completaron físicamente en 438,
890 y 546 ticks, respectivamente.

La campaña privada de prueba quedó en
`_local/iteration126/physical-campaign127/campaign`. Sus tres proofs V3 y las
trazas laterales fueron integrados en el manifest; el replay combinado tiene
250 segmentos y 2,206 ticks nominales, con fuentes
`player_step:0,player_step:1,player_step:2`. El paquete valida 142 archivos y
el auditor verifica seis familias de enemigos, incluido Thief, 93 imágenes y
23 atlas de referencia de jefes. La campaña se probó con Vita3K 0.2.1.4183 en un
perfil local dedicado: tanto GUI instalada como modo consola con VPK por CLI
expiraron antes de crear sesión nativa. El proceso quedó sin CPU y todos los
logs del emulador vacíos. También se repitió el smoke con la copia del usuario
`G:\Vita3k` (versión 0.2.1.4190, SHA-256
`a576ac696b2fde9287b743a2aef0c4a2bb04053cd57a41216fdadda2ee599a56`) y con un
perfil limpio; obtuvo el mismo timeout. El binario local 0.2.1.4183 tiene
SHA-256 `054c3f54b871deb7bf30a4796688a4ceabd572c562a66754fa66484628f34683` y
coincide con otras copias recientes. Los logs históricos de una prueba básica
exitosa corresponden a otro binario (`6c125434...`), pero las pruebas ACT_ONE
guardadas con ese hash también fallan en la máquina actual. Por ahora, la
validación no demuestra ejecución del VPK en Vita3K. El registro Application de
Windows no reportó un evento de Vita3K durante las pruebas, y los logs locales
permanecieron vacíos. El issue oficial
[#3818](https://github.com/Vita3K/Vita3K/issues/3818) reporta fallos de
inicialización SDL en builds Windows 3912+; es una pista para investigar, no
una causa confirmada para este equipo, ya que no se pudo leer la versión del
sistema operativo ni obtener el mensaje SDL.

## Revisión de GitHub

- [`mtolly/vagante-extract`](https://github.com/mtolly/vagante-extract) es la
  referencia directa para extraer y reempaquetar `data.vra`. Conviene usarla
  para contrastar el lector local y comparar inventarios/hash de archivos;
  los recursos originales siguen fuera del repositorio y del VPK.
- [`iwannagooutside/thug-vita-port`](https://github.com/iwannagooutside/thug-vita-port)
  documenta un harness que compila, instala, juega, captura pantallas y lee
  logs de una Vita real. El principio ya reflejado localmente es ligar cada
  replay y log al build; el siguiente salto útil sería captura automática de
  imagen y control por ADB/USB/FTP, cuando el dispositivo esté disponible.
- [`ChatProductions/openmohaavita`](https://github.com/ChatProductions/openmohaavita)
  mantiene un workflow reproducible de build Vita en GitHub Actions y conserva
  símbolos del mismo build para analizar fallos. Es una buena plantilla para
  CI de compilación y artefactos de símbolos, aunque su CMake/dependencias no
  se deben copiar al proyecto que hoy compila con VitaSDK directo.
- [`Rinnegatamante/vitaGL`](https://github.com/Rinnegatamante/vitaGL) publica
  flags de profiling y diagnósticos de compatibilidad. Nuestro renderer actual
  usa GXM directo; por eso aporta ideas de diagnóstico, pero no justifica
  cambiar de backend ahora.
- [`Esodland/SRB2Kart-PSVita-PSTV`](https://github.com/Esodland/SRB2Kart-PSVita-PSTV)
  ilustra las diferencias entre vitaGL antiguo y moderno, costes de carga y
  dependencias de shaders. Es referencia para decisiones de port, no un motor
  reutilizable para el juego.

## Validación

- `python -m unittest discover -s tests -v`: 194/194.
- `prepare_smoke_replay.py`: las tres fuentes son proofs físicos, no
  estimaciones de tiles.
- `prepare_world_bundle.py --validate`: 142 archivos correctos.
- `audit_act_one_runtime_assets.py`: válido; seis familias, 93 imágenes y 23
  atlas de jefe verificados.
- Seed 49: prueba física independiente con traza VTR1, 890 ticks y salida
  alcanzada.
- VPK 1.27: build Windows correcta, 836,864 bytes, SHA-256
  `9633307eb03f5be1f69d20984e48a8f1b4d0b73f9da3023d27a18b51c6864db6`; SFO
  `VAGV00006` / `Vagante2Vita` / `01.27`, CRC ZIP correcto y siete entradas de
  aplicación permitidas.
- Vita3K 0.2.1.4183, perfiles separados: `run-000001` (GUI, instalado) y
  `run-000002` (consola, VPK por ruta) agotaron sus límites de 90 s y 60 s,
  respectivamente, sin crear sesión nativa. Resultados en
  `_local/vita3k-act-one-127/runs/iteration127/`.
- Smoke base de menú con el mismo VPK y otro perfil (`_local/vita3k-baseline127`)
  también agotó 45 s antes de sesión nativa. Esto confirma que la barrera actual
  precede a la carga de la campaña ACT_ONE y no se debe a su replay o niveles.
- El runner automático de rutas, ejecutado de nuevo sobre la campaña seed
  1/49/4, terminó con `passed: true` en los tres pisos (438/890/546 ticks).
  Sus proofs JSON y trazas VTR1 son byte a byte idénticos a los integrados en el
  manifest de campaña, comprobando que la generación de evidencia es
  reproducible.
- Vita3K sigue pendiente de crear una sesión nativa; WSL continúa inaccesible
  con `Wsl/EnumerateDistros/Service/E_ACCESSDENIED`. La probe Windows permitió
  obtener estas validaciones sin cambiar esos bloqueos.
