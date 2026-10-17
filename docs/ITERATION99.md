# Iteración 099: comparación de runtimes Vita3K y referencias externas

Se repitió la replay automatizada de campaña del VPK 00.97 en un sandbox nuevo,
esta vez usando el ejecutable Vita3K con SHA-256
`6C1254342C0D5549F16F5FEF8019CE6E6034A6956C328F95E1E3D4FA7C38E4B7`. Este
hash coincide con el binario registrado por los smoke nativos que pasaron en
la iteración 025. La entrega 097 volvió a validar 113 archivos, tres pisos y
66 imágenes, y generó el replay de 545 segmentos/3,184 ticks. Vita3K terminó
con `0xC0000005` antes de crear sesión o log nativo. El resultado completo
está en
`_local/vita3k-vagante-test097-campaign-4170/runs/iteration097/run-000001/result.json`.

El binario anterior, por sí solo, no explica el smoke histórico exitoso; la
ejecución 025 y esta ejecución difieren también en perfil, configuración,
versión del paquete y entorno de Windows. Todavía no existe evidencia de que
el proceso llegue al runtime de Vagante. `campaign_complete` permanece falso.

Se revisaron repositorios públicos para buscar motor o runtime reutilizable.
La herramienta de extracción de Vagante confirma la utilidad del formato VRA,
ya implementado localmente. Repentogxm y D2Vita aportan patrones de build,
separación de capas y diagnóstico, pero son runtimes específicos de Isaac y
Diablo II, con licencias GPL-2.0 y GPL-3.0 frente a la licencia MIT del port.
No se importó código. El análisis está resumido en
[`REFERENCE_REPOSITORIES.md`](REFERENCE_REPOSITORIES.md).
