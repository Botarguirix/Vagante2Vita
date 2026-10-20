# Iteración 112: selección de la sala para `addBoss`

Ghidra muestra que `generateLevel` (`0x0067c4c0`) guarda `bossRoomX/Y` antes
de inicializar habitaciones, y que `addRandomEnemies` llama a `addBoss`
(`0x0068a770`) al recorrer esa sala. Se implementó la recuperación del
candidato en `choose_boss_room`: salas x-major excluyendo shop, tesoro, secret
y bright-sky; distancia Manhattan mayor que dos desde start y goal; gana la
sala con más tiles cuyo tipo no sea 1 y el primer candidato conserva empates.
El informe registra los candidatos y la coordenada seleccionada. Esto aún no
instancia ni simula al jefe, y el orden RNG de `addBoss` sigue pendiente.

Con seed 12345, el cálculo reporta floor 0 como fallback a goal `[3,3]` porque
no encontró un candidato con tiles abiertos; floor 1 `[3,1]` con 44 abiertos;
floor 2 `[4,2]` con 46 abiertos. Son resultados del modelo privado, no una
prueba de spawn nativo. `0x4000` sigue identificado como `bookPedestalRoom`, no
como flag de arena.

## Validación

- `python -m unittest discover -s tests -v`: 173/173. El barrido de 20 seeds
  por cada uno de los tres pisos verifica además que la coordenada boss-room
  reportada siempre cae dentro del grid ACT_ONE.
- Los tres pisos se exportaron de nuevo. VWI y VWE de entidades coinciden con
  los paquetes base previamente validados; las pruebas de ruta siguen ligadas
  a esos mismos mapas y entidades.
- Se preparó `_local/delivery-act-one-campaign112`; el preflight de runtime
  valida tres pisos, cinco familias de enemigos y 66 imágenes. La replay
  regenerada tiene 545 segmentos/3,184 ticks y tres proofs `player_step`.
- No cambió el ejecutable Vita, por lo que el VPK sigue siendo 01.11. No se
  volvió a ejecutar Vita3K: aún no crea sesión nativa, y esta iteración solo
  modifica el generador/anotación privada. No hay evidencia nueva de gameplay
  del jefe en Vita3K.

## Pendiente

Resolver la invocación RNG/selección de boss concreta y el spawn dentro de la
sala elegida; luego exportar el jefe y sus atlas, implementar sus acciones,
daño/derrota y verificar la continuación del nivel. ACT_ONE sigue incompleto.

## Revisión Ghidra posterior

La lectura enfocada de `addBoss` corrige el alcance de la nota inicial: ACT_ONE
tiene tres slots (Goblin King, Dragon y Worm; entity types 49–51). Si el slot
sorteado ya figura en `spawnedBosses`, la función incrementa el índice módulo
tres y reintenta. `GameEngine` conserva el mismo `LevelGenerator` al llamar
`generateFloor` en `loadNextLevel`, por lo que el conjunto de jefes usados
persiste entre pisos. Se deben integrar los jefes en cada piso según ese estado,
no reservar un único jefe para después del tercer paquete. Aún falta ejecutar
la función con semillas comparables, identificar spawn points y recuperar
ataques, daño, muerte y salida del nivel; esta revisión no cambia el VPK.
