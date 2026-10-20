# Iteración 120: separar el crash histórico del bloqueo de Vita3K actual

Se inspeccionaron los eventos de Windows después de tres timeouts del smoke de
campaña 1.12. No apareció un evento WER nuevo atribuible a las corridas del 17
de octubre. El registro de Aplicación sí contiene crashes históricos de la
instalación compartida `G:\Vita3k\Vita3K.exe`: 0.2.1.4182 y 0.2.1.4183,
`0xc0000005`, ambos fechados el 14 de octubre. Esos errores no demuestran la
causa de los timeouts de las copias privadas usadas en la iteración 119 y no se
modificó ni cerró la instancia compartida.

La enumeración de ventanas de escritorio detectó una ventana titulada
`Vita3K.exe - Error de la aplicación`, pero la captura de esa ventana expiró
dos veces. No se pudo leer el texto de la excepción y no se intentó cerrarla.
El dato queda como pista sin atribución; puede corresponder a un proceso viejo.

El punto demostrado sigue siendo anterior a la sesión nativa: cada corrida
aislada (build reciente OpenGL/SDL, build 0.2.1.4170 OpenGL/SDL y build
0.2.1.4170 consola) no generó `session-*.log`. El preflight del VPK y de los
tres pisos ACT_ONE pasó. La limitación no permite inferir si el gameplay, las
transiciones o el paquete de campaña funcionan.

## Siguiente verificación

Revisar el binario local de Vita3K contra un arranque de control independiente
del título y capturar stdout/stderr desde el inicio. Si la GUI o el proceso de
control tampoco arrancan, investigar esa distribución/entorno; si sí arrancan,
reproducir con VAGV00006 y capturar el primer error de carga. No reutilizar
`G:\Vita3k` ni su perfil compartido. Después, retomar la integración de los
jefes ACT_ONE: el runtime actual completa rutas por los tres pisos, pero no
ofrece los encuentros de jefe y conserva `campaign_complete=false`.
