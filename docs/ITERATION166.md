# Iteración 01.66 — estadísticas del Worm y replay de campaña

La reconstrucción del Worm usaba 80 HP y daño fijo de contacto. La evidencia Ghidra de `BossWorm::initializeStats` (`0x00530df0`) indica 100 HP y daño nominal 3 con factores por defecto de 1.0; `BossWorm::doCollision` (`0x00531d00`) aplica daño nominal menos uno más un entero uniforme entre 0 y 2 y establece 30 frames de cooldown por jugador. El runtime queda en 100 HP, daño de contacto 2–4 y cooldown de 30 frames. El test C confirma el rango y que el contacto no se repite antes de vencer el cooldown.

El VPK `Vagante2Vita-iteration166.vpk` se compiló para `VAGV00006`, versión `01.66`; su SHA-256 es `B3904EF004AEC00FA1D0B9FD3012078E91AAD149516A09CC81830C06E47EBCEF`. Se ejecutaron las 13 suites C host y las tres rutas de ACT_ONE con los fuentes actuales: todas alcanzaron la salida; en la ruta del primer piso el Worm fue derrotado y el jugador terminó con 36 HP. Los proofs v3 y sus trazas de replay se regeneraron para la campaña privada.

Se intentó el smoke de campaña en Vita3K 0.2.1.4194 usando `C:\Vita3k` como origen del emulador y un filesystem aislado del proyecto. El preflight validó los 144 recursos de entrega (134 archivos de juego), y la replay generada cubre los tres pisos (1364 pasos). El proceso expiró a los 45 segundos antes de crear el log de sesión nativa; stdout, stderr y vita3k.log quedaron vacíos. Por eso este intento no verifica entrada, transición ni final del mundo dentro del emulador. Las instancias y el perfil compartido de `C:\Vita3k` no se modificaron.

La reconstrucción de combate sigue incompleta: el ataque ácido del Worm, movimiento y selección del jefe todavía necesitan recuperación/validación; las rutas de piso 1–2 siguen usando geometría provisional y no se verifica la transición nativa. Véanse [BOSS_DECOMP_LEDGER.md](BOSS_DECOMP_LEDGER.md) y [WORM_BUILD_DIAGNOSTIC.md](WORM_BUILD_DIAGNOSTIC.md).

El reintento adicional con Qt/OpenGL visible confirmó la causa anterior al
juego: Windows reportó una lectura nula y el depurador capturó `ucrtbase.dll
+0x25F31`, llamado desde `Vita3K.exe+0x683965`. Sucede también con `--skip-title`
y sin las opciones de configuración del harness, de modo que el VPK y la
replay quedan fuera de esa ruta de fallo. Se documentó en
[WORM_BUILD_DIAGNOSTIC.md](WORM_BUILD_DIAGNOSTIC.md).
