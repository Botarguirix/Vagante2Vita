# Rama A — RoomDatabase

Inventario `pseudo_index.py ... --unit RoomDatabase`; contrato PDB sin editar. Carga, selección y compatibilidad traducidas desde 0075d200–007612c4. Evidencia local en `_local/gui-db/`.

| Función indexada | Estado |
|---|---|
| RoomDatabase::RoomDatabase | traducida; 12 actos con dimensiones12×8, metadatos y carga |
| RoomDatabase::~RoomDatabase | traducida; propiedad de Room* por roomUidLookup |
| getRandomRoom | traducida; búsqueda de menor frecuencia y un sorteo, copia de cuatro flags y contador debug |
| randomizeRooms | traducida; recorrido x/y, conteos por intento, reintentos, compatibilidad y alias de salas múltiples |
| roomsFitVertically | traducida; condiciones originales de los tiles1/3/6 y vecinos |
| getRoom | sobrecarga UID traducida; sobrecarga de índice es de editor y queda vacía con comentario |
| getFirstRoom | traducida |
| getRoomCountOfAct | traducida |
| getAllRoomUidAndNames | traducida |
| getAllTypesOfActWithBitSet | traducida; devuelve tipos con los bits solicitados retirados |
| getNextType | traducida; siguiente/anterior con vuelta, dirección0/error devuelve−1 |
| getFolderPath | traducida |
| getDatabaseDetails | traducida; metadatos locales JSON, copia y caché solo de lecturas válidas |
| loadRooms | traducida; archivo/roomData de data.vra, actos/tipos, Room::load, resize y UID únicos |
| debugPrintRoomFrequencies | traducida mediante VLOG |
| createNewRoomOfType | excluida: editor; comentario y nullptr |
| deleteRoom | excluida: editor; comentario y false |
| resizeAct | excluida: editor; comentario y false. Las especificaciones resize del loader sí se aplican |
| saveRoom | excluida: editor; comentario y false |
| saveRooms | excluida: editor; comentario y false |
| DatabaseDetails::DatabaseDetails | marcada−; construcción/copia STL implícita |
| DatabaseDetails::~DatabaseDetails | marcada−; destrucción STL implícita |

roomsFitHorizontally se recupera del inline de randomizeRooms: busca un par abierto en el borde. Se añadieron a inline_accessors.cpp, sin duplicar existentes, Room::getWidth/getHeight/getTiles/getWidthInRooms/getHeightInRooms, con comentario de evidencia roomsFitVertically @0075eb90 y randomizeRooms @0075ed40. No se tocaron Client, NetMenu, GuiCursor ni ConfigGui.

field_writes.py confirma que el movw1 del constructor escribe worldAllowBackgrounds=true (+0xB0) y debugCountRoomFrequency=false (+0xB1); Ghidra lo denomina erróneamente debugCountRoomFrequency. El flag JSON world_allow_backgrounds también escribe0xB0. Se contrastaron argumentos Room::Room/resizeRoomTo y la espera10ms al no encontrar salas con call_args.py. offset_name.py resuelve correctamente DC/E0/E4/E8; con D0 produce parent.links+4 porque no reconoce Tile** en su parser: se usó el offsetD0 de la cabecera y las instrucciones, sin modificar la herramienta ni el contrato.

Constantes: 12 actos; dimensiones iniciales12×8; 20 rerolls (21 elecciones como máximo por celda); una extracción MT19937 enmascarada0x7fffffff y módulo del número de candidatos, incluso si solo hay uno. El mapa used no se modifica en getRandomRoom: randomizeRooms lo incrementa antes de comprobar el encaje, conservando también los intentos fallidos. Las banderas copiadas son shop/treasure/brightSky/bookPedestal y act. No se sustituye la selección por una distribución STL.

Carga: metadatos personalizados, act_specifications, acts[].rooms[tipo][]; presencia de resize activa resize_width/height aunque resize sea false. Conserva el orden JSON, llamadas Room::load/resizeRoomTo/generateRandomUniqueId, inserción en UID y vectores. El getter existente ResourceManager::getConfig es privado en la cabecera; se usa la misma recuperación por instanciación explícita que PaletteDatabase, sin redefinirlo. Los IDs de Workshop son solo metadatos, sin APIs de Steam.

Límites explícitos: Room::Room, Room::load, Room::generateRandomUniqueId y la copia Room(Room*) siguen como dependencias stub. Por ello no se acredita la carga de salas comerciales ni el camino de clones/reintentos de randomizeRooms. El loader rechaza un UID vacío tras llamar al generador, en vez de entrar en un bucle/índice inválido debido al stub. JSON inválido, actos fuera de rango e índices UID rotos tienen retorno seguro con diagnóstico; no se fabrican tiles ni se consume RNG de reemplazo. Otros métodos sin entrada de pseudocódigo en --unit permanecen como stubs.

Verificación: `pwsh tools/build_recon_pc.ps1` terminó en OK; `pwsh tools/run_recon_tests.ps1 -Unit RoomDatabase` pasó. Casos: archivo/archive, metadatos, resize por presencia, caché, consultas y máscaras, frecuencias/orden RNG exacto, flags, bordes de tiles, celdas ya pobladas sin sorteos, propiedad/limpieza y exclusión de escrituras del editor. Room/Tile fixtures fijan sus campos deliberadamente; no sustituyen el loader del juego. Logs `_local/gui-db/RoomDatabase_build.log` y `RoomDatabase_test.log`. Sin recursos comerciales en Git, sin push y sin validación de gameplay Vita.
