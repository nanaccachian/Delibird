#include "mensajes.h"
#include "gameCard.h"

void SuscribirseColas(Cliente* cliente) {

	Eventos_AgregarOperacion(cliente->eventos, BROKER_CONECTADO, (EventoOperacion) &ConexionColas);
	Eventos_AgregarOperacion(cliente->eventos, NEW_POKEMON, (EventoOperacion)&Recibir_NEW_POKEMON);
	Eventos_AgregarOperacion(cliente->eventos, CATCH_POKEMON, (EventoOperacion)&Recibir_CATCH_POKEMON);
	Eventos_AgregarOperacion(cliente->eventos, GET_POKEMON, (EventoOperacion)&Recibir_GET_POKEMON);
	Eventos_AgregarOperacion(cliente->eventos, BROKER_ID_MENSAJE, (EventoOperacion)&Recibir_ID);

	if (Socket_Enviar(BROKER_CONECTAR, NULL, 0, cliente->socket) < 0) {
		free(cliente->info);
		TerminarProgramaConError("ERROR EN CONEXION CON EL BROKER");
	}
}

void SocketEscucha(char* ip, int puerto) {

	Eventos* eventos = Eventos_Crear0();

	Eventos_AgregarOperacion(eventos, NEW_POKEMON, (EventoOperacion) &Recibir_NEW_POKEMON);
	Eventos_AgregarOperacion(eventos, CATCH_POKEMON, (EventoOperacion) &Recibir_CATCH_POKEMON);
	Eventos_AgregarOperacion(eventos, GET_POKEMON, (EventoOperacion) &Recibir_GET_POKEMON);

	servidor = CrearServidor(ip,puerto,eventos);
}

void ConexionColas(Cliente* cliente, Paquete* paquete) {
	BROKER_DATOS_SUSCRIBIRSE datosNEW;
	BROKER_DATOS_SUSCRIBIRSE datosCATCH;
	BROKER_DATOS_SUSCRIBIRSE datosGET;

	datosNEW.cola = COLA_NEW_POKEMON;
	datosCATCH.cola = COLA_CATCH_POKEMON;
	datosGET.cola = COLA_GET_POKEMON;

	EnviarMensajeSinFree(cliente, BROKER_SUSCRIBIRSE, &datosNEW, (void*) &SerializarM_BROKER_SUSCRIBIRSE);
	EnviarMensajeSinFree(cliente, BROKER_SUSCRIBIRSE, &datosCATCH, (void*) &SerializarM_BROKER_SUSCRIBIRSE);
	EnviarMensajeSinFree(cliente, BROKER_SUSCRIBIRSE, &datosGET, (void*) &SerializarM_BROKER_SUSCRIBIRSE);

	free(cliente->info);
}

void Recibir_NEW_POKEMON(Cliente* cliente, Paquete* paqueteRecibido) {
	Stream* stream = Stream_CrearLecturaPaquete(paqueteRecibido);
	uint32_t id = Deserializar_uint32(stream);
	DATOS_NEW_POKEMON_ID* datos = malloc(sizeof(DATOS_NEW_POKEMON_ID));
	datos->datos = Deserializar_NEW_POKEMON(stream);
	datos->id = id;

	if (stream->error)
		log_error(logger,"Error al deserializar NEW_POKEMON");
	else {
		log_info(logger,"Tu mensaje llego correctamente");
		EnviarID(cliente,id);
		pthread_t thread;
		pthread_create(&thread, NULL, (void*) Operacion_NEW_POKEMON,datos);
		pthread_detach(thread);
	}
}

void Operacion_NEW_POKEMON(DATOS_NEW_POKEMON_ID* datos) {

	log_info(logger,"Buscando pokemon...");

	NodoArbol* nodoPokemon = encontrarPokemon(datos->datos.pokemon); //NO ESTA ENCONTRANDO POKEMONS QUE YA EXISTEN

	if(nodoPokemon == NULL) {
		nodoPokemon = crearPokemon(datos->datos.pokemon);
		agregarNodo(directorioFiles(),nodoPokemon);
	}

	char* path = pathPokemon(datos->datos.pokemon);

	FILE* filePokemon = fopen(path,"ab+");

	if (filePokemon != NULL) {

		if(estaAbierto(pathPokemon(datos->datos.pokemon))) {
			sleep(config_get_int_value(config,"TIEMPO_REINTENTO_OPERACION"));
			Operacion_NEW_POKEMON(datos);
		} else {
			abrir(path);

			int cantBloques =  0;

			t_list* numerosBloques = leerBlocks(path, &cantBloques); //DEVUELVE LA LISTA DE INTS DE LOS NROS DE BLOQUE

			t_list* datosBloques = convertirBloques(numerosBloques,cantBloques); //DEVUELVE LA LISTA DE DATOSBLOQUES

			DatosBloques posYCant;

			posYCant.cantidad = datos->datos.cantidad;
			posYCant.pos.posX = datos->datos.posicion.posX;
			posYCant.pos.posY = datos->datos.posicion.posY;

			int size = config_get_int_value(config,"BLOCK_SIZE"); // TODO PUEDE NO VENIR DEL CONFIG

			int bytes = agregarCantidadEnPosicion(datosBloques,posYCant,numerosBloques,size);

			fclose(filePokemon);

			cambiarMetadataPokemon(path,numerosBloques,bytes);

			sleep(config_get_int_value(config,"TIEMPO_RETARDO_OPERACION")); //TODO PUEDE NO VENIR DEL CONFIG
			Enviar_APPEARED_POKEMON(datos);

			free(path);

			list_clean(numerosBloques);
			list_destroy(numerosBloques);
			list_clean(datosBloques);
			list_destroy(datosBloques);
		}
	} else {
		log_error(logger,"Error al leer archivo de pokemon");
	}
}

void eliminarElemento(void* datos) {
	free(datos);
}

void Recibir_CATCH_POKEMON(Cliente* cliente, Paquete* paqueteRecibido) {
	Stream* stream = Stream_CrearLecturaPaquete(paqueteRecibido);
	uint32_t id = Deserializar_uint32(stream);
	DATOS_CATCH_POKEMON_ID* datos = malloc(sizeof(DATOS_NEW_POKEMON_ID));
	datos->datos = Deserializar_CATCH_POKEMON(stream);
	datos->id = id;

	if (stream->error)
		log_error(logger, "Error al deserializar CATCH_POKEMON");
	else
		EnviarID(cliente,id);

	pthread_t thread;
	pthread_create(&thread, NULL, (void*) Operacion_CATCH_POKEMON,&datos);
	pthread_detach(thread);
}

void Operacion_CATCH_POKEMON(DATOS_CATCH_POKEMON_ID* datos) {
	log_info(logger,"Buscando pokemon...");

	NodoArbol* nodoPokemon = encontrarPokemon(datos->datos.pokemon); //NO ESTA ENCONTRANDO POKEMONS QUE YA EXISTEN

	if(nodoPokemon == NULL) {
		log_error(logger, "No existe el pokemon");
	} else {
		char* path = pathPokemon(datos->datos.pokemon);

		FILE* filePokemon = fopen(path,"ab+");

		if (filePokemon != NULL) {

			if(estaAbierto(pathPokemon(datos->datos.pokemon))) {
				sleep(config_get_int_value(config,"TIEMPO_REINTENTO_OPERACION"));
				Operacion_CATCH_POKEMON(datos);
			} else {
				abrir(path);

				int cantBloques =  0;

				t_list* numerosBloques = leerBlocks(path, &cantBloques); //DEVUELVE LA LISTA DE INTS DE LOS NROS DE BLOQUE

				t_list* datosBloques = convertirBloques(numerosBloques,cantBloques); //DEVUELVE LA LISTA DE DATOSBLOQUES

				Posicion posicion;

				posicion.posX = datos->datos.posicion.posX;
				posicion.posY = datos->datos.posicion.posY;

				int size = config_get_int_value(config,"BLOCK_SIZE"); // TODO PUEDE NO VENIR DEL CONFIG

				int* bytes = malloc(sizeof(int));

				bool caught = atraparPokemon(datosBloques,posicion,numerosBloques,size,bytes);

				fclose(filePokemon);

				cambiarMetadataPokemon(path,numerosBloques,*bytes);

				sleep(config_get_int_value(config,"TIEMPO_RETARDO_OPERACION")); //TODO PUEDE NO VENIR DEL CONFIG
				Enviar_CAUGHT_POKEMON(datos,caught);

				free(path);

				list_clean(numerosBloques);
				list_destroy(numerosBloques);
				list_clean(datosBloques);
				list_destroy(datosBloques);
			}
		} else log_error(logger,"Error al leer archivo de pokemon");
	}
}

void Recibir_GET_POKEMON(Cliente* cliente, Paquete* paqueteRecibido) {
	Stream* stream = Stream_CrearLecturaPaquete(paqueteRecibido);
	uint32_t id = Deserializar_uint32(stream);
	DATOS_GET_POKEMON_ID* datos = malloc(sizeof(DATOS_GET_POKEMON_ID));
	datos->datos = Deserializar_GET_POKEMON(stream);
	datos->id = id;

	if (stream->error)
		log_error(logger, "Error al deserializar GET_POKEMON");
	else
		EnviarID(cliente,id);

	pthread_t thread;
	pthread_create(&thread, NULL, (void*) Operacion_GET_POKEMON,&datos);
	pthread_detach(thread);
}

void Operacion_GET_POKEMON(DATOS_GET_POKEMON_ID* datos) {

	log_info(logger,"llego");
	log_info(logger,"Buscando pokemon...");

	NodoArbol* nodoPokemon = encontrarPokemon(datos->datos.pokemon); //NO ESTA ENCONTRANDO POKEMONS QUE YA EXISTEN

	if(nodoPokemon == NULL) {
		Enviar_LOCALIZED_POKEMON(datos,NULL);
	} else {
		char* path = pathPokemon(datos->datos.pokemon);

		FILE* filePokemon = fopen(path,"ab+");

		if (filePokemon != NULL) {

			if(estaAbierto(pathPokemon(datos->datos.pokemon))) {
				sleep(config_get_int_value(config,"TIEMPO_REINTENTO_OPERACION"));
				Operacion_GET_POKEMON(datos);
			} else {
				abrir(path);

				int cantBloques =  0;

				t_list* numerosBloques = leerBlocks(path, &cantBloques); //DEVUELVE LA LISTA DE INTS DE LOS NROS DE BLOQUE

				t_list* datosBloques = convertirBloques(numerosBloques,cantBloques); //DEVUELVE LA LISTA DE DATOSBLOQUES

				sleep(config_get_int_value(config,"TIEMPO_RETARDO_OPERACION")); //TODO PUEDE NO VENIR DEL CONFIG

				Enviar_LOCALIZED_POKEMON(datos,datosBloques);

				free(path);

				list_clean(numerosBloques);
				list_destroy(numerosBloques);
				list_clean(datosBloques);
				list_destroy(datosBloques);
			}
		} else {
			log_error(logger,"Error al leer archivo de pokemon");
		}
	}
}

void EnviarID(Cliente* cliente, uint32_t identificador)
{
	DATOS_ID_MENSAJE* id_mensaje = malloc(sizeof(DATOS_ID_MENSAJE));

	id_mensaje->id = identificador;
	EnviarMensaje(cliente, BROKER_ACK, id_mensaje, (void*) &SerializarM_ID_MENSAJE);

	free(id_mensaje);
}


void Enviar_APPEARED_POKEMON(DATOS_NEW_POKEMON_ID* datos) {

	DATOS_APPEARED_POKEMON_ID* datosEnviar = malloc(sizeof(DATOS_APPEARED_POKEMON_ID));

	datosEnviar->id = datos->id;
	datosEnviar->datos.pokemon = datos->datos.pokemon;
	datosEnviar->datos.posicion = datos->datos.posicion;

	//HAY QUE VER COMO HACER PARA QUE NO LE ENVIE MENSAJE AL BROKER SI NO SE PUDO CONECTAR
	//HABRIA QUE GUARDAR LOS MENSAJES EN ALGUN LUGAR Y CUANDO SE RECONECTA MANDARSELOS
	if(clienteBroker != NULL) {
		EnviarMensaje(clienteBroker, APPEARED_POKEMON, datosEnviar, (void*) &SerializarM_APPEARED_POKEMON_ID);
		log_info(logger,"Se envio correctamente el appeared pokemon");
	} else {
		log_info(logger, "Se guardo el mensaje para cuando se pueda volver a conectarse");
	}

	free(datos);
}

void Enviar_CAUGHT_POKEMON(DATOS_CATCH_POKEMON_ID* datos, bool caught) {

	DATOS_CAUGHT_POKEMON_ID* datosEnviar = malloc(sizeof(DATOS_CAUGHT_POKEMON_ID));

	datosEnviar->idCorrelativa = datos->id;
	datosEnviar->datos.capturado = caught;

	//HABRIA QUE GUARDAR LOS MENSAJES EN ALGUN LUGAR Y CUANDO SE RECONECTA MANDARSELOS
	if(clienteBroker != NULL) {
		EnviarMensaje(clienteBroker, CAUGHT_POKEMON, datosEnviar, (void*) &SerializarM_CAUGHT_POKEMON_ID);
		log_info(logger,"Se envio correctamente el caught pokemon");
	} else {
		log_info(logger, "Se guardo el mensaje para cuando se pueda volver a conectarse");
	}

	free(datos);
}

void Enviar_LOCALIZED_POKEMON(DATOS_GET_POKEMON_ID* datos,t_list* datosArchivo) {

}

void Recibir_ID(Cliente* cliente, Paquete* paqueteRecibido) {
	Stream* stream = Stream_CrearLecturaPaquete(paqueteRecibido);
	Deserializar_uint32(stream);
}
