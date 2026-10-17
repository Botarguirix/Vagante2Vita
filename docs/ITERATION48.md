# Iteración 048: Boulder visible vinculado a BoulderTrap

El cargador VWE2 ahora reproduce el anclaje que `BoulderTrap::setLevel` hace al
asignar nivel: comprueba el tile central de la placa, omite spikes/tiles sólidos
y busca hacia arriba el primer tile sólido. Si lo encuentra, guarda la roca
cuadrada (`Boulder::Boulder` tipo `SQUARE`) en el origen indicado por ese tile.
El renderer muestra el sprite original de 16×16 `traps/boulders.png` encima del
terreno y junto a la placa. `WORLD_ENTITIES` registra cuántas rocas pudo anclar
el paquete de entidades, para diagnóstico en los logs de sesión.

El bundle privado 050 añade ese recurso sin incluirlo en el VPK. El reloj de
liberación de seis frames de la iteración 047 sigue registrándose, pero la roca
aún no cae ni participa en colisiones con unidades; falta portar la física de
`Boulder::step`, colisión y daño. El primer mundo sigue incompleto.

## Verificación

- VPK `00.48` compilado con VitaSDK Windows; Title ID `VAGV00006`.
- SHA-256: `efde46b5870f48adf829660c3e0ed65a0d19f908da5ec3f3f3ce18ac99941273`.
- Paquete privado ACT_ONE 050: manifiesto válido, 69 archivos.
- `unittest`: 119 pruebas OK, incluida la preparación del sprite Boulder en el
  paquete ACT_ONE.
- Compilación ARM C estricta correcta para runtime y la regresión de entidades;
  el ejecutable de regresión C host no se pudo correr porque WSL devuelve
  `E_ACCESSDENIED` y no hay compilador host.
- Vita3K sigue sin validación de este VPK: la instancia aislada previa se quedó
  sin ventana ni log de sesión nativa.
