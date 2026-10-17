# Iteración 054: armas de la tabla melee

El paquete VWA y su cargador ahora admiten 12 armas con secuencia `player-stab`
de 24×24: dagas 45–47, espadas 49–53, vara mágica 79 y CQC 85–87. El generador
lee los nombres de armas, frames, geometría de golpe, daño y knockback de
`attacks.json`, y mantiene los IDs asignados por `Item::getWeapon` en Ghidra.
El cargador valida IDs duplicados, límites y geometría. El ID 62 está
confirmado como `FLAIL`, aunque no se incluye porque requiere `player-swing`.

`player-swing` y `player-heavy-swing` se dejaron fuera de VWA por ahora: sus
secuencias usan 24×24 y 32×32 respectivamente, y el renderer actual solo tiene
el atlas de puñalada. Su integración requiere representar la geometría de cada
animación y cargar el atlas correcto, para que el sprite y el hitbox mantengan
la misma secuencia.

## Verificación

- Suite Python: 121/121; incluye pruebas de codificación de la tabla y de
  rechazo de geometría incompatible.
- `tools/build_vpk_windows.ps1 -Iteration 054`: compilación VitaSDK aprobada;
  VPK en `_local/build-act-one-054/`.
- Bundle privado ACT_ONE de 67 archivos: manifiesto validado, VWA actualizado
  a 12 armas y generado desde el `attacks.json` local.
- Replay de salida ACT_ONE en Vita3K 0.2.1.4170 con perfil aislado: el proceso
  terminó con código -1 tras 90 segundos. `stdout.log`, `stderr.log` y
  `vita3k.log` quedaron vacíos y no se generó log nativo de la aplicación; por
  eso esta corrida no confirma gameplay ni el cargador VWA.

## Límites

Las armas añadidas están disponibles para la base de ataque, pero todavía no
hay entidad de botín, selección del objeto generado por el cofre, recogida ni
equipo desde el inventario. La generación de recompensas de ACT_ONE y la
validación de gameplay en Vita3K siguen pendientes.
