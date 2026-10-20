# Iteración 126: aparición renderizable del Thief en ACT_ONE

`LevelGenerator::addRandomEnemies` (`0x0068d1e0`) ahora se reconstruye hasta
la rama que coloca un Thief: un draw inicial habilita presupuesto 1 cuando
`roll % 5 == 0` y `floorIndex > 0`; cada candidato debe estar al menos a 15
tiles de la primera entrada en ambos ejes, superar el filtro de tile no sólido
y pasar `tilePathsToEdgeOfRoom`. El spawn consume el draw horizontal y
ubica el Thief en `tile_x*16 + (roll & 15), tile_y*16`. En el seed 31, piso 1,
la ruta de salida simulada encuentra y exporta una instancia tipo 84. También
se pasa su tile al filtro de anclas de `addBoss` como entidad dinámica.

El límite sigue explícito: el generador usa hoy el endpoint reconstruido como
aproximación de `entranceLocations[0]`; aún no reproduce el conjunto nativo de
tiles ocupados, los efectos RNG del constructor/quest NPC ni todos los filtros
de candidatos. La rama y el draw de posición están implementados, pero la
generación sigue provisional y no se afirma paridad de seed.

El formato VWE3, el cargador C y el renderer privado ahora admiten `TYPE_THIEF`
(84), con cuerpo 7×12 y sus sprites originales de 18×18 en el paquete privado.
El runtime usa la locomoción genérica existente; no implementa todavía el
robo, teletransporte/desvanecimiento, diálogo, ataque ni comportamiento de
quest/shop del Thief. Los assets no se agregan al VPK.

## Validación

- Pruebas Python enfocadas: seed 31 exporta exactamente un Thief con posición
  derivada de los cuatro bits bajos, se serializa como subtipo 84 y aparece en
  el ledger de bloqueos dinámicos del jefe. Las pruebas de gate/candidatos,
  layout y paquete pasan.
- Pruebas host C: 12/12; se valida la carga VWE3 del subtipo 84 y el cuerpo
  nativo 7×12.
- Paquete privado de tres pisos: manifest válido con 136 archivos; auditoría
  de runtime válida con seis familias de enemigos, incluido Thief, 96 imágenes
  verificadas y 23 atlas de referencia de jefe. Floors 0/1/2 se exportaron con
  seeds 1/31/4; sólo el segundo está elegido para demostrar el spawn Thief.
- VitaSDK Windows compiló
  `_local/build-act-one-126/Vagante2Vita-iteration126.vpk` (836,870 bytes).
  El archivo ZIP pasa CRC, contiene sólo los siete archivos de aplicación
  permitidos y su SFO es `TITLE_ID=VAGV00006`, `APP_VER=01.26`; SHA-256:
  `5f4268b55c52806ca191e5ed7123eb13a25e41f2cf931f7b7ab2fb7a45f0c8d7`.
- Smoke automatizado de Vita3K con perfil aislado, VPK 01.26, paquete ACT_ONE
  de tres pisos y replay generada (65 segmentos, 1,760 ticks nominales) terminó
  en `before_native_session`: el emulador agotó 45 s sin crear log nativo; los
  logs stdout/stderr/Vita3K quedaron vacíos. Resultado en
  `_local/vita3k-iteration126/runs/iteration126/run-000001/result.json`.
  Esto no es una prueba de gameplay.

El primer mundo aún no está completo: generación exacta, IA/comportamiento de
Thief, jefes integrados, combate e interacciones completas, transiciones
nativas y sesión de Vita3K/hardware siguen pendientes. El nuevo VPK prueba que
el runtime compila, no que ACT_ONE haya sido jugado con éxito.
