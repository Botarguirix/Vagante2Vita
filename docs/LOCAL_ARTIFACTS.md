# Inventario de artefactos locales

Este archivo separa entregables y evidencia de las salidas que se pueden
regenerar. No borra perfiles del emulador ni datos del juego.

## Vigente

- Demo local actual: `_local/demo-act-one-203/` (VPK 02.03 y recursos para `ux0:data/Vagante2Vita/`); inventario SHA-256 en `demo-manifest.json`.
- VPK compilado: `_local/build-act-one-203/Vagante2Vita-iteration203.vpk`; comprobado con la skill Vita-port (7 entradas, 0 errores).
- Feedback de daño y límites de prueba: [ITERATION203.md](ITERATION203.md) y [DEMO_ACT_ONE_203.md](DEMO_ACT_ONE_203.md).
- Auditoría de campaña persistente 02.03: `_local/route-act-one-203-rogue-chain-candidate/` y `_local/host-chain-act-one-203-rogue/`; proofs individuales no corresponden al estado cargado, gate detecta el desajuste y la cadena muere en floor 1.
- Demo anterior: `_local/demo-act-one-199/` (VPK 01.99).
- Campaña con proofs V4 válidos para el simulador actual: `_local/campaign-act-one-197-current-routes-20261019/`.
- Rutas host independientes: `_local/route-worm-first-current-20261019/`.
- Auditoría integrada de HP/transiciones: `tools/test_act_one_campaign_chain_windows.ps1`; ejecución reciente en `_local/host-chain-act-one-203-rogue/`.
- Smoke Vita3K del VPK 01.99: `_local/vita3k-c197-fullcopy-test/runs/iteration199/run-000001/`; timeout antes de sesión nativa.
- Registro de alcance y límites: [DEMO_ACT_ONE_199.md](DEMO_ACT_ONE_199.md), [DEMO_ACT_ONE_198.md](DEMO_ACT_ONE_198.md), [ITERATION197.md](ITERATION197.md) y [ROADMAP.md](ROADMAP.md).

El paquete Worm es un fixture provisional para probar la ruta. No representa
spawn nativo recuperado ni valida transiciones persistentes entre pisos.

## Históricos binarios retirados

- Se retiraron las carpetas regenerables `_local/build-act-one-194/`,
  `build-act-one-195-diag/` y `build-act-one-196-diag/` para quitar VPKs,
  ejecutables y objetos antiguos. Sus VPKs tenían estos hashes SHA-256, por si
  hace falta identificarlos en una copia externa:
  - `Vagante2Vita-iteration194.vpk` (848114 bytes):
    `AD92AD6AE5473B42FB5AA9ECAC89B778F440FD74990559D81A5F66827D749A81`.
  - `Vagante2Vita-iteration195.vpk` (849528 bytes):
    `CC06610A1EE685CB043F54F89B1A50248A4EC9A14955906A54D3654B0A1DCAF7`.
  - `Vagante2Vita-iteration196.vpk` (848657 bytes):
    `EB412329A7E076C6CD7DD129B2A43B1825B7991A12BA2B1C2E721C2015851B41`.
- Se retiraron las ejecuciones intermedias regenerables
  `_local/routes-act-one-197-lateworm-rogue/` y
  `_local/routes-act-one-197-lateworm-rogue-v4/`. Se conserva la prueba final
  `_local/routes-act-one-197-proved-final/`.

## Históricos que se conservan

- `_local/campaign-act-one-184-rogue/` y
  `campaign-act-one-194-rogue-current/`: bases anteriores útiles para comparar
  datos de piso y exportaciones.
- `_local/routes-act-one-197-rogue-current/`: ruta fallida que demuestra por qué
  el Worm en la entrada impedía la prueba del piso 1.
- Las carpetas `vita3k-*`: perfiles y resultados de pruebas. No son simples
  cachés; pueden contener configuración, datos instalados o evidencia del
  emulador y deben conservarse salvo que se revise cada una por separado.

## Salidas regenerables

La prueba final de rutas se ejecutó contra la campaña ya empaquetada y está en
`_local/routes-act-one-197-proved-final/`; sus proofs V4 incluyen hashes de
VWI2, VWE2 y VWE3.

Los directorios `build-host-*`, `routes-*` y `analysis*` restantes deben
evaluarse según referencias desde `docs/ITERATION*.md` antes de retirarlos:
varios conservan logs usados como evidencia histórica. No limpiar carpetas
fuera de `_local` con esta guía.
