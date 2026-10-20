# Iteración 01.34: soporte visual inicial para Dragon

El paquete VWE3 y la auditoría ACT_ONE ahora reconocen los subtipos nativos de
jefe 49/50/51, identificándolos como Goblin King, Dragon y Worm. El runtime Vita
acepta por ahora el Goblin King y Dragon; Dragon carga y dibuja el atlas de
vuelo `baby_dragon_flying.png` usando cuatro celdas de 48×48 confirmadas por
Ghidra y las dimensiones del PNG. Ghidra confirma en `BossDragon` cuerpo de
13×13, 65 HP y daño 5 con los factores normales por defecto. El Dragon todavía
es un actor de vista no interactivo: sus acciones, ataques, colisiones letales,
recompensa y continuación no están implementados. El Worm se identifica en el
auditor, pero el runtime lo rechaza hasta que se porte su estructura cabeza +
seis segmentos enlazados.

Validación: suite Python `unittest` 198/198; pruebas host C 12/12, incluida la
carga del subtipo Dragon y sus dimensiones de colisión. El auditor también
acepta una copia temporal de la campaña con un registro Dragon y verifica que
los atlas `flying`, `dive`, `fall` y `launch` tengan cuatro cuadros cada uno;
el paquete de entrega real conserva el Goblin King. Se regeneraron los
proofs de ruta de los tres pisos contra las fuentes actuales; la campaña valida
142 archivos y el replay se prepara con 250 pasos y 2,206 ticks nominales.

El VPK 01.34 se compiló con VitaSDK de Windows porque `wsl.exe --list --quiet`
volvió a fallar con `Wsl/EnumerateDistros/Service/E_ACCESSDENIED`. Artefacto:
`_local/build-act-one-134/Vagante2Vita-iteration134.vpk`; contiene 7 entradas,
`eboot.bin` de 191,574 bytes y versión SFO 01.34. SHA-256:
`EEB577761EF7AFE30741A7240E3425B0F50F2AFB60B9CF5245131471D3CFD85C`.

La prueba automatizada visible con Vita3K 0.2.1.4190 volvió a expirar a los 30 s
antes de crear una sesión nativa (`failure_stage=before_native_session`);
stdout, stderr y `vita3k.log` están vacíos y no aparece ventana principal.
Reporte: `_local/vita3k-act-one-134-4190/runs/iteration134/run-000001/`. La
ejecución sigue sin validación en emulador. Este soporte de atlas no completa la
campaña ni afirma paridad nativa.
