# Iteración 051: contrato flotante reproducible

Los dos flujos de build para Vita ahora pasan `-ffp-contract=off`: el script
directo de VitaSDK y el target CMake del runtime. La simulación de movimiento,
colisiones y los replays ya no permiten que el compilador contraiga
multiplicación y suma en FMA. Esto reduce diferencias de redondeo entre
compilaciones y mantiene rutas de jugador reproducibles. El estado de
generación del ACT_ONE no cambia con este ajuste.

## Verificación

- VitaSDK `2026.08.1-1` / GCC ARM `15.2.0`; VPK `00.51` compilado, con siete
  entradas, `TITLE_ID=VAGV00006` y `APP_VER=00.51`.
- SHA-256: `E4E1FAA6AA3B768DBBEAF8C5253FE990E718EBF14D77173CE63E9B5D51AA4A93`.
- Las 119 pruebas Python pasan y el paquete privado ACT_ONE 050 valida sus 69
  archivos. La compilación ARM estricta con `-ffp-contract=off` acepta runtime
  y fuentes de pruebas C; las pruebas C host siguen sin ejecutarse porque no
  hay compilador C nativo disponible.
- No se repitió el smoke Vita3K: el binario/perfil disponible sigue siendo el
  mismo que mostró una excepción nativa antes de iniciar el juego en la
  iteración 049. La ejecución Vita3K de esta versión permanece pendiente.
