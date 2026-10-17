# Vagante2Vita

Investigación técnica e ingeniería inversa reproducible para un posible port nativo de **Vagante a PS Vita**.

**Estado:** reconstrucción parcial; el VPK 00.81 se compiló con VitaSDK en WSL y Windows, y los siete archivos internos coinciden. El bundle privado ACT_ONE 082 valida; `player_step` alcanzó las salidas de los tres mapas actuales y produjo proofs v3 con trazas físicas. Pasan 12 pruebas C host y 159 Python. El smoke de campaña agota 45 s en Vita3K antes de crear una sesión nativa, por lo que el emulador no ha validado el VPK ni las transiciones. Generación exacta y comportamiento de enemigos siguen incompletos. Véanse el [estado de reconstrucción](docs/ROADMAP.md), la [iteración 081](docs/ITERATION81.md) y el [análisis de referencias](docs/PORTING_REFERENCE_REVIEW.md).

Se generó localmente pseudocódigo de **30 568 funciones**, incluidos código del juego y bibliotecas/runtime, con dos fallos por timeout después de reintentar. El título 00.02 fue probado en Vita por el usuario. La iteración 00.03 añade fondo animado, sprites por atlas, vista parcial de selección de clases y log de recursos, transiciones y rendimiento; falta probar esta versión en consola. Estos resultados no representan un porcentaje de reconstrucción ni prueban rendimiento del juego en Vita.

- [Viabilidad técnica y evidencia](docs/FEASIBILITY.md)
- [Proyectos relacionados](docs/REFERENCES.md)
- [Evaluación de Repentogxm y D2Vita](docs/PORTING_REFERENCE_REVIEW.md)
- [Plan de reconstrucción](docs/ROADMAP.md)
- [Iteración 066: RNG del agua ACT_ONE](docs/ITERATION66.md)
- [Iteración 067: ledger RNG de pedestales](docs/ITERATION67.md)
- [Iteración 068: RNG de skills en libros](docs/ITERATION68.md)
- [Iteración 069: shuffles de endpoints ACT_ONE](docs/ITERATION69.md)
- [Iteración 070: blockers G/H de endpoints](docs/ITERATION70.md)
- [Iteración 071: BreakableWall en ACT_ONE](docs/ITERATION71.md)
- [Iteración 072: Boulder independiente en ACT_ONE](docs/ITERATION72.md)
- [Iteración 073: sprites de enemigos y bundle ACT_ONE completo](docs/ITERATION73.md)
- [Iteración 074: daño de contacto Bat basado en Ghidra](docs/ITERATION74.md)
- [Iteración 075: ataque de Goblin basado en script y geometría Ghidra](docs/ITERATION75.md)
- [Iteración 076: ruta física del piso 0 validada con GCC en WSL](docs/ITERATION76.md)
- [Iteración 077: auditoría de rutas físicas para pisos 1–2](docs/ITERATION77.md)
- [Iteración 078: smoke automatizado de campaña ACT_ONE](docs/ITERATION78.md)
- [Iteración 080: trazas de player_step para smoke ACT_ONE](docs/ITERATION80.md)
- [Iteración 081: rutas físicas ACT_ONE y correcciones de entidades](docs/ITERATION81.md)
- [Iteración 032: transición entre pisos ACT_ONE](docs/ITERATION32.md)
- [Iteración 031: un impacto melee por ataque](docs/ITERATION31.md)
- [Entrega 013: primer recorrido interactivo del mundo inicial](docs/ITERATION13.md)
- [Entrega 008: paisaje en caché y diagnóstico de stuttering](docs/ITERATION8.md)
- [Primera prueba jugable: zona inicial, colisiones y jugador](docs/FIRST_GAMEPLAY.md)
- [Entrega 007: actualizar la misma aplicación](docs/ITERATION7.md)
- [Arte de LiveArea](docs/ITERATION6.md)
- [Optimización 005: resolución reducida y caché](docs/ITERATION5.md)
- [Conservación de iteraciones y logs](docs/ITERATION4.md)
- [Cambios del menú animado 003](docs/ITERATION3.md)
- [Cómo reproducir el análisis](docs/REPRODUCING.md)
- [Resultados de esta ejecución](docs/STATUS.md)
- [Compilar y usar el diagnóstico Vita](docs/VITA_DIAGNOSTIC.md)

## Herramientas

Python 3.11 o posterior; las herramientas Python no requieren paquetes externos.

```powershell
python tools/inspect_game.py 'C:\Program Files (x86)\Steam\steamapps\common\vagante'
python tools/vra.py 'C:\Program Files (x86)\Steam\steamapps\common\vagante\data.vra'
python tools/vra.py 'C:\Program Files (x86)\Steam\steamapps\common\vagante\data.vra' --extract _local/assets
python tools/vra.py 'C:\Program Files (x86)\Steam\steamapps\common\vagante\data.vra' --out _local/analysis/archive.json
python tools/setup_re.py
./tools/run_decompile.ps1
python -m unittest discover -s tests -v
```

Las salidas se guardan en `_local/`: inventario PE/PDB, símbolos, recursos extraídos, proyecto Ghidra y pseudocódigo C. Los resultados de Ghidra deben revisarse función por función; no equivalen al código C++ original ni se pueden compilar automáticamente para Vita.

Para registrar qué `data.vra` local produjo un paquete externo, añade
`--source-archive <ruta-a-data.vra>` a `tools/prepare_world_bundle.py`. El
manifiesto guarda su tamaño y SHA-256; el archivo comercial no se copia al
bundle ni al VPK.

**Última iteración de reconstrucción: 071.** `BreakableWall` recibe daño melee, destruye su grupo ortogonal y actualiza las tiles y colisiones; el VPK 00.71 compila. El smoke Vita3K de este paquete terminó por timeout sin sesión nativa, por lo que el gameplay no quedó validado en emulador. Los Boulders independientes y varias reglas nativas de pared siguen pendientes. [ITERATION71.md](docs/ITERATION71.md).

## Alcance del repositorio

Se publican herramientas originales, documentación y el diagnóstico Vita. Cada usuario aporta su copia del juego. No se incluyen ejecutables, PDB, gráficos, audio, guardados ni pseudocódigo comercial. La licencia de las herramientas no concede derechos sobre Vagante. Proyecto no afiliado con Nuke Nine.

La entrega actual 013 conserva la misma aplicación y separa entregas/logs: [instrucciones](docs/ITERATION13.md).


Última entrega: **iteración 014**, misma aplicación VAGV00006. Optimiza escalado, transferencia de pantalla y terreno; objetivo 60 Hz pendiente de validación física. [Instalación y resultados](docs/ITERATION14.md).


Última entrega local de esa serie: **015**, con mundo inicial incluido en el VPK privado y fallback `app0:world`. [Instalación, controles y alcance](docs/ITERATION15.md). Los recursos comerciales no forman parte del repositorio; builds públicos requieren preparar la copia del usuario.


**Entrega vigente: 016.** VPK sin recursos comerciales, mundo externo, pausa START y corrección de aterrizajes. Sin métricas integradas. [Detalles](docs/ITERATION16.md).


**Entrega vigente: 017.** Primera adaptación de bordes, recursos hang externos opcionales. [Instalación y alcance](docs/ITERATION17.md). Objetivo ampliado al mundo completo; generación, entidades y combate siguen pendientes.


**Entrega vigente: 018.** Fondo de menú animado por defecto, R alterna perfil ligero para medición externa. [Detalles](docs/ITERATION18.md).


**Entrega vigente: 019.** Selección tras comparaciones locales: ligero por defecto, animado preparado disponible por R. Intro jugable; todavía no juego completo. [Validación e instalación](docs/ITERATION19.md).


**Entrega vigente: 020.** Entidades del mundo inicial original cargadas desde `world/intro-entities.vwe`, decorado y atlas animados. Copiar el world externo de la nueva entrega. [Instalación y limitaciones](docs/ITERATION20.md).


**Entrega vigente: 021.** SELECT muestra estadísticas derivadas, equipo base y habilidades iniciales. Ficha consultable; combate/equipo funcional aún pendientes. [Controles y validación](docs/ITERATION21.md).


**Entrega vigente: 022.** Cuadrado ejecuta la secuencia de estocada original, personaje/arma y geometría de impacto. Copiar world externo actualizado. Daño a entidades y efectos/proyectiles aún pendientes. [Instalación y pruebas](docs/ITERATION22.md).


**Entrega vigente: 023.** Rogue carga y dispara su arco con R, consume munición y las flechas colisionan con terreno. Copiar world externo actualizado. Combate con entidades y generación de pisos siguen pendientes. [Instalación y pruebas](docs/ITERATION23.md).


**Entrega vigente: 024.** Cargador VWI2 de tamaño variable, cámara en dos ejes y menor duplicación de terreno en memoria. Conserva la intro original; primer piso aún sin ensamblar. [Instalación, pruebas y límites](docs/ITERATION24.md).


**Entrega vigente: 026.** Ensamblado ACT_ONE 80×40 exportado como mundo opcional; la auditoría verifica la caja completa de spawn (8×12 px), conectividad A* topológica (58 pasos para la semilla auditada) y un smoke host de 300 frames sin colisión sólida. El recorrido hasta la meta con física y las entidades funcionales siguen pendientes. Vita3K termina con `0xC0000005` antes del logger tanto con el VPK 026 como con el VPK control 025 en el runtime dedicado derivado de `_local/vita3k`. [Iteración 026](docs/ITERATION26.md) y [estado de reconstrucción](docs/GENERATION_RECOVERY.md).

**Reconstrucción 030.** Habitaciones ACT_ONE ensambladas como grids de 80×40
tiles con sus cinco capas y ubicaciones de entidades. Borde cerrado y predicado
de candidatos de entrada recuperados. [Pruebas, comandos y pendientes](docs/GENERATION_RECOVERY.md).
La iteración 026 añade sellado de secretos, simulación geométrica de agua y
extremos provisionales al análisis privado; el VPK 026 empaqueta el exportador
de mundo ACT_ONE sin recursos comerciales. Sigue pendiente la paridad exacta,
las entidades funcionales y validar el recorrido en Vita3K o hardware.

**Entrega vigente: 065.** El log ACT_ONE distingue entidades con soporte de
BreakableWall/Boulder aún pendientes; el smoke requiere cero tipos pendientes
en los tres pisos actuales. VPK compilado, pero Vita3K no llega a abrir
interfaz ni log. [Iteración 065](docs/ITERATION65.md).

**Entrega vigente: 064.** El paquete de campaña valida la tabla VLT1 completa
antes de aceptar loot; VPK recompilado con VitaSDK. Vita3K aislado sigue sin
iniciar y las rutas/mapas de la campaña son provisionales. [Iteración 064](docs/ITERATION64.md).
