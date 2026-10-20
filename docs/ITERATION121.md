# Iteración 121: control sin título en la instancia privada de Vita3K

Se probó el runtime Vita3K 0.2.1.4190 copiado a
`_local/vita3k-vagante-test112/runtime`, usando su `config.yml` con la ruta de
configuración correctamente citada y filesystem privado. Esta verificación no
cargó VAGV00006 ni un VPK, para separar el arranque del frontend del contenido
del juego.

- El sondeo CLI `--version` terminó con código 0, sin texto de salida.
- El arranque normal con `--load-config --keep-config --config-location <config>`
  permaneció activo pero sin `MainWindowHandle`, sin título y sin actualizar
  `vita3k.log`.
- Repetir con `--load-app-list` produjo lo mismo.
- La enumeración de módulos mostró solo el ejecutable y módulos Windows; Qt no
  llegó a cargarse. No apareció evento WER nuevo.
- Se detuvieron únicamente los procesos del control privado.

La primera corrida manual de control no cuenta: `Start-Process -ArgumentList`
dividió una ruta con espacios. Las observaciones de arriba usan
`ProcessStartInfo` con argumentos separados y ruta citada.

Esto localiza el bloqueo antes de cargar un título y antes de inicializar Qt,
pero no explica por qué el frontend queda en ese estado. Las corridas del VPK
1.12 siguen sin sesión nativa, por lo que la campaña no está validada en
Vita3K. No se tocó la instalación compartida `G:\Vita3k`. La integración de
jefes ACT_ONE continúa pendiente.
