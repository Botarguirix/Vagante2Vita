# Iteración 043: sprite del bloqueador de roca

El renderer ACT_ONE ahora carga opcionalmente `entities/boulder_blocker.png` y
lo dibuja para las entidades VWE2 de tipo 23. Verifica las dimensiones de
16×16 y aplica el reflejo según `facingLeft`; la orientación corresponde a
`BoulderBlocker::draw` en el binario de Vagante. El paquete privado de campaña
049 ya contiene el recurso y el manifiesto valida su hash. El juego no falla si
el recurso no está disponible.

Esto es integración visual del obstáculo. No activa aún su colisión ni la
trampa de Boulder completa; ambos comportamientos dependen de las condiciones
de contacto y del ciclo de vida del proyectil/roca originales, que todavía no
están reconstruidos con suficiente fidelidad.

## Verificación

- VPK 00.43 compilado con VitaSDK Windows. Contiene las siete entradas estándar
de LiveArea, `param.sfo` y `eboot.bin`; tamaño 808,652 bytes.
- SHA-256: `489B0F028ABE9C0BA8E56DB1521D4AB8482E048982AF301BD718E0C613DB48B2`.
- Campaña privada 049: manifiesto válido, 68 archivos.
- El smoke aislado con Vita3K 0.2.1.4183 para 00.43 agotó 60 segundos y
  terminó con código -1; no produjo log de Vita3K ni sesión nativa. La
  transición de salida no pudo verificarse en el emulador.
