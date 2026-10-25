# Rama A — seis unidades de menús

Traducción solicitada completada en orden, con un commit local por unidad:

| Unidad | Commit de traducción | Inventario por función, constantes y dudas |
|---|---|---|
| Tween | f821500 | [Informe](RAMA_A_TWEEN.md) |
| PaletteDatabase | e134e61 | [Informe](RAMA_A_PALETTEDATABASE.md) |
| MainMenu | a4a0531 | [Informe](RAMA_A_MAINMENU.md) |
| Credits | 8958cbb | [Informe](RAMA_A_CREDITS.md) |
| ProgressionGui | dd3f1eb | [Informe](RAMA_A_PROGRESSIONGUI.md) |
| ConfigGui | commit que contiene este informe | [Informe](RAMA_A_CONFIGGUI.md) |

Todas las entradas de --unit están contabilizadas: cuerpos del juego traducidos, operaciones implícitas de C++/STL y wrappers del compilador identificados; Steam excluido expresamente. No se modificaron cabeceras generadas.

La comprobación final integrada `pwsh tools/build_recon_pc.ps1 -Run` compila y ejecuta con **exit code: 0**. recon.log contiene58 funciones todavía sin traducir alcanzadas. Pasaron los seis tests de unidad con assertions activas. Evidencia en `_local/menus/ConfigGui_final_build.log`, `ConfigGui_test.log` y los cinco `<Unidad>_integrated_test.log`.

Esta comprobación integrada sustituye el bloqueo histórico descrito en los primeros informes: Client fue corregido por su autor durante el trabajo. No se editó su código aquí.

Queda provisional la interacción de opciones: GuiSlider/GuiSelect/GuiCheckbox/GuiLabel aún son stubs. También quedan helpers no indexados y dependencias de juego detallados por unidad. INTRO_CARAVAN_ENTRANCE sigue sin un inmediato inequívoco. No se presenta este trabajo como validación de gameplay ni de Vita. No se hizo push.
