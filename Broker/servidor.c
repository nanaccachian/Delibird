#include "broker.h"
#include "clienteBroker.h"
#include "cola.h"
#include "memoria.h"
#include "mensaje.h"
#include "../Utils/socket.h"
#include "../Utils/serializacion.h"

Servidor* servidor;
pthread_mutex_t mutexRecepcion;

static void Operacion_CONECTAR(Cliente* cliente, Paquete* paqueteRecibido);
static void Operacion_RECONECTAR(Cliente* cliente, Paquete* paqueteRecibido);
static void Operacion_SUSCRIBIRSE(Cliente* cliente, Paquete* paqueteRecibido);
static void Operacion_ACK(Cliente* cliente, Paquete* paqueteRecibido);

static void Operacion_NEW_POKEMON(Cliente* cliente, Paquete* paqueteRecibido);
static void Operacion_APPEARED_POKEMON(Cliente* cliente, Paquete* paqueteRecibido);
static void Operacion_CATCH_POKEMON(Cliente* cliente, Paquete* paqueteRecibido);
static void Operacion_CAUGHT_POKEMON(Cliente* cliente, Paquete* paqueteRecibido);
static void Operacion_GET_POKEMON(Cliente* cliente, Paquete* paqueteRecibido);
static void Operacion_LOCALIZED_POKEMON(Cliente* cliente, Paquete* paqueteRecibido);

void IniciarServidorBroker(char* ip, int puerto)
{
	Eventos* eventos = Eventos_Crear0();
	Eventos_AgregarOperacion(eventos, BROKER_CONECTAR, (EventoOperacion)&Operacion_CONECTAR);
	Eventos_AgregarOperacion(eventos, BROKER_RECONECTAR, (EventoOperacion)&Operacion_RECONECTAR);
	Eventos_AgregarOperacion(eventos, BROKER_SUSCRIBIRSE, (EventoOperacion)&Operacion_SUSCRIBIRSE);
	Eventos_AgregarOperacion(eventos, BROKER_ACK, (EventoOperacion)&Operacion_ACK);

	Eventos_AgregarOperacion(eventos, NEW_POKEMON, (EventoOperacion)&Operacion_NEW_POKEMON);
	Eventos_AgregarOperacion(eventos, APPEARED_POKEMON, (EventoOperacion)&Operacion_APPEARED_POKEMON);
	Eventos_AgregarOperacion(eventos, CATCH_POKEMON, (EventoOperacion)&Operacion_CATCH_POKEMON);
	Eventos_AgregarOperacion(eventos, CAUGHT_POKEMON, (EventoOperacion)&Operacion_CAUGHT_POKEMON);
	Eventos_AgregarOperacion(eventos, GET_POKEMON, (EventoOperacion)&Operacion_GET_POKEMON);
	Eventos_AgregarOperacion(eventos, LOCALIZED_POKEMON, (EventoOperacion)&Operacion_LOCALIZED_POKEMON);

	pthread_mutex_init(&mutexRecepcion, NULL);

	servidor = CrearServidor(ip, puerto, eventos);
	if (servidor == NULL) log_error(logger, "Error al iniciar escucha (%s:%d)", ip, puerto);
}
void FinalizarServidorBroker()
{
	DestruirServidor(servidor);
}

static void Operacion_CONECTAR(Cliente* cliente, Paquete* paqueteRecibido)
{
	if (cliente->info != NULL)
	{
		//log_error(logger, "El cliente ya está conectado");
		return;
	}

	ClienteBroker* clienteBroker = CrearClienteBroker(cliente);
	cliente->info = clienteBroker;

	BROKER_DATOS_RECONECTAR datos;
	datos.id = clienteBroker->id;
	Stream* stream = SerializarM_BROKER_CONECTADO(&datos);
	int r = Socket_Enviar(BROKER_CONECTADO, stream->base, stream->tamanio, cliente->socket);
	Stream_DestruirConBuffer(stream);

	if (r > 0)
		log_info(logger, "Nuevo proceso conectado (id: %d)", clienteBroker->id);
	else
		log_error(logger, "Error al conectar cliente");
}

void Operacion_RECONECTAR(Cliente* cliente, Paquete* paqueteRecibido)
{
	if (cliente->info != NULL)
	{
		//log_error(logger, "El cliente ya está conectado");
		return;
	}

	BROKER_DATOS_RECONECTAR datos;
	if (!DeserializarM_BROKER_RECONECTAR(paqueteRecibido, &datos))
		log_error(logger, "Error Deserializar_BROKER_RECONECTAR");
	else
	{
		ClienteBroker* clienteBroker = ObtenerClienteBroker(datos.id);
		if (clienteBroker == NULL)
		{
			AvanzarIDCliente(datos.id);
			ClienteBroker* clienteBroker = CrearClienteBroker(cliente);
			cliente->info = clienteBroker;
			BROKER_DATOS_RECONECTAR datos;
			datos.id = clienteBroker->id;
			Stream* stream = SerializarM_BROKER_CONECTADO(&datos);
			Socket_Enviar(BROKER_CONECTADO, stream->base, stream->tamanio, cliente->socket);
			Stream_DestruirConBuffer(stream);

			log_info(logger, "Un cliente no registrado se reconecto (ID previa: %d)", datos.id);
			return;
		}
		else
		{
			Stream* stream = SerializarM_BROKER_CONECTADO(&(datos));
			int r = Socket_Enviar(BROKER_CONECTADO, stream->base, stream->tamanio, cliente->socket);
			Stream_DestruirConBuffer(stream);

			if (r > 0)
				log_info(logger, "Proceso reconectado (id: %d)", clienteBroker->id);
			else
				log_error(logger, "Error al conectar cliente");

			Colas_EnviarMensajesRestantesSiCorrespondeA(clienteBroker);
		}
	}
}

static void EnviarIDMensaje(uint32_t idMensaje, Cliente* cliente)
{
	DATOS_ID_MENSAJE respuesta;
	respuesta.id = idMensaje;
	EnviarMensaje(cliente, BROKER_ID_MENSAJE, &respuesta, (Serializador)&SerializarM_ID_MENSAJE);
}
static void RecibirMensaje(Cliente* cliente, CodigoDeCola tipoDeMensaje, uint32_t idMensaje, uint32_t idCorrelativo, Stream* contenido)
{
	//if (!CorresponderRecibirRespuesta(tipoDeMensaje, idCorrelativo))
	//	return;

	pthread_mutex_lock(&mutexRecepcion);
	Mensaje* nuevoMensaje = CrearMensaje(tipoDeMensaje, idMensaje, idCorrelativo, contenido->tamanio);
	GuardarMensaje(nuevoMensaje, tipoDeMensaje, contenido);
	EnviarIDMensaje(idMensaje, cliente);
	Cola_ProcesarNuevoMensaje(tipoDeMensaje, nuevoMensaje);
	pthread_mutex_unlock(&mutexRecepcion);
}

// Mensajes administrativos

static void Operacion_SUSCRIBIRSE(Cliente* cliente, Paquete* paqueteRecibido)
{
	if (cliente->info == NULL)
	{
		log_error(logger, "El cliente no está conectado! (SUSCRIPCION)");
		return;
	}

	BROKER_DATOS_SUSCRIBIRSE datosSuscripcion;
	DeserializarM_BROKER_SUSCRIBIRSE(paqueteRecibido, &datosSuscripcion);
	Cola* cola = ObtenerCola(datosSuscripcion.cola);
	AgregarSuscriptor(cola, (ClienteBroker*)cliente->info);
	log_info(logger, "Nueva suscripción (cliente: %d, cola: %s)", ((ClienteBroker*)cliente->info)->id, CodigoDeColaAString(datosSuscripcion.cola));

	pthread_mutex_lock(&mutexRecepcion);
	Cola_EnviarMensajesRestantesSiCorrespondeA(cola, datosSuscripcion.cola, cliente->info);
	pthread_mutex_unlock(&mutexRecepcion);
}
static void Operacion_ACK(Cliente* cliente, Paquete* paqueteRecibido)
{
	if (cliente->info == NULL)
	{
		log_error(logger, "El cliente no está conectado! (ACK)");
		return;
	}

	DATOS_ID_MENSAJE datosACK;
	DeserializarM_ID_MENSAJE(paqueteRecibido, &datosACK);
	if(RegistrarACK(datosACK.id, (ClienteBroker*)cliente->info))
		log_info(logger, "Recibido ACK (cliente: %d, mensaje: %d)", ((ClienteBroker*)cliente->info)->id, datosACK.id);
}

// Mensajes de colas

static void Operacion_NEW_POKEMON(Cliente* cliente, Paquete* paqueteRecibido)
{
	Stream* streamLectura = Stream_CrearLecturaPaquete(paqueteRecibido);
	DATOS_NEW_POKEMON datos = Deserializar_NEW_POKEMON(streamLectura);
	if (streamLectura->error)
		log_error(logger, "Error al deserializar NEW_POKEMON");

	uint32_t id = GenerarIDMensaje();

	char* textoDatos = DatosAString_NEW_POKEMON(&datos);
	log_info(logger, "Recibido: %s", textoDatos);
	free(textoDatos);

	Stream* stream = SerializarM_NEW_POKEMON(&datos);
	RecibirMensaje(cliente, COLA_NEW_POKEMON, id, -1, stream);
	Stream_Destruir(streamLectura);
	Stream_DestruirConBuffer(stream);

	free(datos.pokemon);
}
static void Operacion_APPEARED_POKEMON(Cliente* cliente, Paquete* paqueteRecibido)
{
	Stream* streamLectura = Stream_CrearLecturaPaquete(paqueteRecibido);
	uint32_t idCorrelativa = Deserializar_uint32(streamLectura);
	DATOS_APPEARED_POKEMON datos = Deserializar_APPEARED_POKEMON(streamLectura);
	if (streamLectura->error)
		log_error(logger, "Error al deserializar APPEARED_POKEMON");

	uint32_t id = GenerarIDMensaje();

	char* textoDatos = DatosAString_APPEARED_POKEMON(&datos);
	log_info(logger, "Recibido: %s (correlativo: %d)", textoDatos, idCorrelativa);
	free(textoDatos);

	Stream* stream = SerializarM_APPEARED_POKEMON(&datos);
	RecibirMensaje(cliente, COLA_APPEARED_POKEMON, id, idCorrelativa, stream);
	Stream_Destruir(streamLectura);
	Stream_DestruirConBuffer(stream);

	free(datos.pokemon);
}
static void Operacion_CATCH_POKEMON(Cliente* cliente, Paquete* paqueteRecibido)
{
	Stream* streamLectura = Stream_CrearLecturaPaquete(paqueteRecibido);
	DATOS_CATCH_POKEMON datos = Deserializar_CATCH_POKEMON(streamLectura);
	if (streamLectura->error)
		log_error(logger, "Error al deserializar CATCH_POKEMON");

	uint32_t id = GenerarIDMensaje();

	char* textoDatos = DatosAString_CATCH_POKEMON(&datos);
	log_info(logger, "Recibido: %s", textoDatos);
	free(textoDatos);

	Stream* stream = SerializarM_CATCH_POKEMON(&datos);
	RecibirMensaje(cliente, COLA_CATCH_POKEMON, id, -1, stream);
	Stream_Destruir(streamLectura);
	Stream_DestruirConBuffer(stream);

	free(datos.pokemon);
}
static void Operacion_CAUGHT_POKEMON(Cliente* cliente, Paquete* paqueteRecibido)
{
	Stream* streamLectura = Stream_CrearLecturaPaquete(paqueteRecibido);
	uint32_t idCorrelativa = Deserializar_uint32(streamLectura);
	DATOS_CAUGHT_POKEMON datos = Deserializar_CAUGHT_POKEMON(streamLectura);
	if (streamLectura->error)
		log_error(logger, "Error al deserializar CAUGHT_POKEMON");

	uint32_t id = GenerarIDMensaje();

	char* textoDatos = DatosAString_CAUGHT_POKEMON(&datos);
	log_info(logger, "Recibido: %s (correlativo: %d)", textoDatos, idCorrelativa);
	free(textoDatos);

	Stream* stream = SerializarM_CAUGHT_POKEMON(&datos);
	RecibirMensaje(cliente, COLA_CAUGHT_POKEMON, id, idCorrelativa, stream);
	Stream_Destruir(streamLectura);
	Stream_DestruirConBuffer(stream);
}
static void Operacion_GET_POKEMON(Cliente* cliente, Paquete* paqueteRecibido)
{
	Stream* streamLectura = Stream_CrearLecturaPaquete(paqueteRecibido);
	DATOS_GET_POKEMON datos = Deserializar_GET_POKEMON(streamLectura);
	if (streamLectura->error)
		log_error(logger, "Error al deserializar GET_POKEMON");

	uint32_t id = GenerarIDMensaje();

	char* textoDatos = DatosAString_GET_POKEMON(&datos);
	log_info(logger, "Recibido: %s", textoDatos);
	free(textoDatos);

	Stream* stream = SerializarM_GET_POKEMON(&(datos));
	RecibirMensaje(cliente, COLA_GET_POKEMON, id, -1, stream);
	Stream_Destruir(streamLectura);
	Stream_DestruirConBuffer(stream);

	free(datos.pokemon);
}
static void Operacion_LOCALIZED_POKEMON(Cliente* cliente, Paquete* paqueteRecibido)
{
	Stream* streamLectura = Stream_CrearLecturaPaquete(paqueteRecibido);
	uint32_t idCorrelativa = Deserializar_uint32(streamLectura);
	DATOS_LOCALIZED_POKEMON datos = Deserializar_LOCALIZED_POKEMON(streamLectura);
	if (streamLectura->error)
		log_error(logger, "Error al deserializar LOCALIZED_POKEMON");

	uint32_t id = GenerarIDMensaje();

	char* textoDatos = DatosAString_LOCALIZED_POKEMON(&datos);
	log_info(logger, "Recibido: %s", textoDatos);
	free(textoDatos);

	Stream* stream = SerializarM_LOCALIZED_POKEMON(&datos);
	RecibirMensaje(cliente, COLA_LOCALIZED_POKEMON, id, idCorrelativa, stream);
	Stream_Destruir(streamLectura);
	Stream_DestruirConBuffer(stream);

	free(datos.pokemon);
	if (datos.cantidad > 0)
		free(datos.posiciones);
}
