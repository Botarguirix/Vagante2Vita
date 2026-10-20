# Iteración 119: VPK de campaña 1.12 y diagnóstico de Vita3K

Se avanzó a la versión de aplicación `01.12` y se compiló el código Vita actual
con el VitaSDK Windows instalado (`arm-vita-eabi-gcc 15.2.0`). El empaquetado
terminó y produjo `_local/build-act-one-112-smoke/Vagante2Vita-iteration112.vpk`
(835,751 bytes; SHA-256
`fea6ea3b67351dc7b94ce9123bab627f9010e9f45fa08664b3f21095c9f6af2e`). La SFO
contiene `TITLE_ID=VAGV00006` y `APP_VER=01.12`; el VPK abre como ZIP íntegro,
contiene `eboot.bin` (188,909 bytes) y siete entradas esperadas. La cifra de
tamaño se deja como dato del archivo generado y no implica una prueba de
ejecución.

El preflight del bundle `_local/delivery-act-one-campaign112` pasó: manifiesto
con 113 archivos, tres pisos, cinco familias enemigas y 66 imágenes. El
harness construyó una replay externa de 545 segmentos / 3,184 ticks, con las
tres pruebas de ruta `player_step` ligadas al paquete de campaña.

## Resultado de Vita3K

Se hicieron tres pruebas aisladas con filesystem nuevo, sin modificar
instancias compartidas:

- Vita3K local más reciente, OpenGL/SDL: timeout a 180 s antes de una sesión.
- Vita3K 0.2.1.4170, OpenGL/SDL: timeout a 120 s antes de una sesión.
- Vita3K 0.2.1.4170, consola cargando directamente el VPK: timeout a 45 s antes
  de una sesión.

En las tres, el VPK quedó instalado/cargado y el preflight del paquete pasó,
pero no se creó `session-*.log`; `vita3k.log`, stdout/stderr y debug stdout/stderr
quedaron vacíos. El resultado automático reporta
`failure_stage=before_native_session`. Esto no demuestra un defecto del mapa o
de las transiciones; tampoco prueba que el juego funcione. La prueba de
campaña en runtime sigue sin evidencia y debe resolverse el arranque del
emulador o capturarse un smoke exitoso en un Vita3K que inicie sesiones.

## Toolchain y validación

- WSL sigue devolviendo `Wsl/EnumerateDistros/Service/E_ACCESSDENIED`; para
  producir el VPK se usó VitaSDK Windows, que ya está instalado y disponible.
- `python -m unittest discover -s tests -v`: 187/187 pasan después de actualizar
  la versión de aplicación.
- VPK: ZIP válido, SFO y título verificados; SHA-256 registrado arriba.
- No se validó gameplay en Vita3K ni en hardware. ACT_ONE conserva tres rutas
  automatizadas de `player_step`, pero el paquete sigue declarando
  `campaign_complete=false` porque no integra los jefes nativos.
