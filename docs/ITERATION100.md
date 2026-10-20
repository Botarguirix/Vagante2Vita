# Iteración 100: persistencia de campaña y VPK 00.98

`player_carry_campaign_state` conserva ahora `items_collected` al cargar el
siguiente piso, junto con HP, atributos, inventario y cofres abiertos. El test
de jugador verifica explícitamente que el contador no se reinicie en la
transición. El diagnóstico de carga de armas cuerpo a cuerpo dejó de declarar
el daño completamente pendiente: registra que el daño de las formas de arma
ya se aplica y mantiene pendiente el escalado por estadísticas. La relación
entre el daño de formas y `Player::updateStats` requiere evidencia adicional de
Ghidra antes de integrarse.

La suite Python pasa 170/170 y CTest pasa 12/12. El build ARM en WSL compila
con VitaSDK. Se corrigió el empaquetado CMake para entregar los mappings con
`VERBATIM`, preservando las rutas que contienen espacios; la compilación
automática completa pasó desde cero. `_local/build-vita-098/Vagante2Vita-iteration098.vpk` pesa
831,570 bytes, SHA-256
`A59A03F04A8C38368CC8A23440CAAE4E6646EB84370A2E343BE0E14924617FF3`. La
identidad verificada es `VAGV00006`, `Vagante2Vita`, versión `00.98`; el VPK
contiene únicamente los siete archivos permitidos y no incluye los recursos
privados del juego.

Los proofs y trazas VTR1 de los tres pisos se regeneraron con la fuente actual;
el fingerprint común es
`61f76c519dda1ce5a0aea252dcccbab9bfde40998c9c3a65071ddb8691833715`. Las rutas
de 1,741, 736 y 375 ticks alcanzaron sus salidas. La entrega privada
`_local/delivery-act-one-campaign098` valida 113 archivos y prepara un replay
de 545 segmentos/3,184 ticks con `player_step` en los pisos 0–2.

Vita3K 0.2.1.4170, SDL/OpenGL, recibió el VPK 00.98 en un perfil aislado, pero
terminó con `0xC0000005` antes de crear un log nativo. El resultado está en
`_local/vita3k-vagante-test098-campaign/runs/iteration098/run-000002/result.json`.
Esto no valida el runtime ni las transiciones; el bloqueo continúa en el
arranque del emulador. La cobertura de combate sigue parcial, las rutas usan
geometría provisional y `campaign_complete=false`.
