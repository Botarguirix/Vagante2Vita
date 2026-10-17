# Iteración 046: reentrada requerida en BoulderTrap

Se amplió la regresión C de BoulderTrap para cubrir la secuencia de colisión
que impone `prevDown`: un actor que permanece encima durante los 60 ticks de
armado mantiene presionada la placa, pero no activa la trampa. Al salir, la
placa sube; al volver a entrar registra una sola activación. Salir y volver a
pisar después no genera otra activación.

Esto valida el ciclo de la placa en el modelo portable. La comprobación aún
usa rectángulos del jugador y no reproduce el despacho genérico de colisiones.
La entidad Boulder asociada, su liberación tras seis ticks, física y daño
siguen pendientes de recuperar con evidencia suficiente de los argumentos de
`BoulderTrap::setLevel` y del comportamiento de `Boulder::step`.

## Verificación

- VPK `00.46` compilado con VitaSDK Windows; Title ID `VAGV00006`.
- Python `unittest`: 119 pruebas OK.
- Compilación C estricta Vita para runtime y el test de entidades: correcta.
- No hay compilador C host disponible para ejecutar el test C en Windows.
- No se ejecutó el nuevo VPK en Vita3K; los intentos anteriores expiraron
  antes de crear una sesión de juego.
