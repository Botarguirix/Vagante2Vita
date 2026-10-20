# Iteración 202 — cobertura completa a 960×540

El framebuffer de Vita mide 960×544; el área visible de 960×540 ya estaba al
límite físico. La revisión del escalador encontró que la conversión anterior
640×360→960×540 pisaba algunas filas y dejaba huecos verticales. Ahora el
perfil alto hace mapeo nearest-neighbour exacto 3:2 en ambos ejes y llena todas
las filas visibles. El modo ligero 320×180 conserva el escalado exacto 3×. Los
márgenes superior e inferior de dos filas y el padding de pitch siguen negros.
No se agregó otro framebuffer; el escalador usa una fila temporal de 960
píxeles.

La prueba C valida el mapeo completo, los márgenes, el padding del pitch y ambos
perfiles. Pasan 14 suites C host y 226 pruebas Python. El VPK 02.02 conserva el
ID `VAGV00006` y siete entradas permitidas. Se generó un demo local con los
recursos de campaña fuera del VPK y hashes en `demo-manifest.json`.

VitaSDK de Windows compiló el paquete. WSL rechazó el inicio de su instancia
con `E_ACCESSDENIED`, así que esta compilación no se atribuye a WSL. Vita3K
0.2.1.4194 no se pudo usar para comprobar la pantalla: el diagnóstico más
reciente reproduce `0xC0000005` antes del título, en los frontends SDL y Qt y
con `--skip-title`. No se declara validación en emulador ni en hardware.

El artefacto local está en `_local/demo-act-one-202/` y su manifiesto cubre
189 archivos. SHA-256 del VPK:
`201de4a857c48b52d23904c6ce766208f53509b19bd9d639cc5f2bf949913ece`.

La ruta de campaña sigue con el límite registrado en la iteración 201: las
rutas separadas por piso no equivalen a una cadena con salud persistente, que
muere en el piso 1. No se alteraron combate ni reglas de salud en esta
iteración.
