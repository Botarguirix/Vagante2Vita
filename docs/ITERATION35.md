# Iteración 035: combate a distancia y continuidad entre pisos

Las flechas ahora recorren su trayectoria contra el terreno y los enemigos
activos. Un impacto aplica el daño entregado por la definición del arco,
desactiva el proyectil y registra impactos y muertes a distancia. La interpolación
de daño usa los valores mínimo/máximo del arco y la fracción de carga por cuadros;
esa curva todavía es provisional y requiere comparación con el juego original.

La transición entre pisos conserva la salud actual, los atributos derivados,
equipo, habilidades, munición y cofres abiertos. El nuevo piso reinicia los
estados transitorios del jugador y vuelve a enlazar el arma cuerpo a cuerpo y el
arco con las definiciones cargadas para ese piso. Una prueba nativa cubre
continuidad, munición y enlaces.

Validación: CTest host 11/11 y `python -m unittest discover -s tests` 117/117.
El runtime ARM estricto y el VPK se compilaron con VitaSDK en WSL. El artefacto
`_local/build-act-one-035-wsl/Vagante2Vita-iteration035.vpk` usa identidad
`VAGV00006`, versión `00.35`, siete entradas permitidas y SHA-256
`1e371f9d92ae932bb10b3ee60a55deff245b9e27d5ad300e62d57c8f70976b7b`.

Los dos intentos de humo con Vita3K 0.2.1.4183 usaron un perfil privado, el VPK
y la entrega 047. Ambos procesos terminaron con `0xC0000005` antes de producir
log del emulador o sesión nativa; por eso no confirman arranque, carga del mundo
ni transición. Los tres pisos sí tienen una ruta automatizada bajo
`player_step`, pero los pisos 1–2 usan geometría provisional.

Verificación adicional del 2026-10-16: `tools/build_vpk_windows.ps1` compiló
la misma fuente ARM con VitaSDK 15.2.0 en
`_local/build-check-035-windows/Vagante2Vita-iteration035.vpk`. El verificador
`check_vpk.py` reportó siete entradas, título `VAGV00006`, versión `00.35` y
cero errores/avisos. SHA-256:
`b2e0780df646b4ddb4dcf711ec36985d0c3dcc192dfc5de63d592c67f5499cc6`.
Esto confirma una segunda ruta de compilación independiente de WSL, cuyo
servicio en esta sesión responde `E_ACCESSDENIED`.

El smoke de esa compilación corrió con OpenGL, frontend SDL y filesystem
privado en `_local/vita3k-vagante-035/runs/iteration035/run-000003`. El proceso
alcanzó el timeout de 60 s y el harness lo detuvo (`timedOut=true`); no generó
`vita3k.log`, stdout/stderr ni sesión nativa. Por tanto el resultado es fallo
de smoke sin diagnóstico interno, no evidencia de que el título haya arrancado
ni de un crash atribuible al juego. La inspección visual de la ventana no pudo
completarse porque la herramienta de escritorio agotó su aprobación.

Investigación 2026-10-16: el perfil aislado omitía los temas inmutables
`data/gui-configs/custom-themes`; `tools/test_vita3k.ps1` ahora copia solo esa
carpeta, no preferencias de usuario. Una repetición fresca con Vulkan/SDL en
`_local/vita3k-vagante-035-themefix/runs/iteration035/run-000001` terminó en
`0xC0000005` antes de crear sesión y sin logs. El tema faltante no explica el
fallo; el diagnóstico de Vita3K queda abierto.

ACT_ONE sigue incompleto: enemigos y daño tienen parámetros provisionales,
abrir cofres aún no entrega botín, trampas/objetos activos y jefe faltan, y no se
ha validado la campaña en Vita3K ni en hardware. No se afirma paridad exacta de
generación.
