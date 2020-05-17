#pragma once

typedef enum
{
	// CLIENTE -> BROKER
	BROKER_CONECTAR,
	BROKER_RECONECTAR,
	BROKER_SUSCRIBIRSE,
	BROKER_ACK,
	// BROKER -> CLIENTE
	BROKER_CONECTADO,
	BROKER_SUSCRITO,
	BROKER_ID_MENSAJE,
	// CUALQUIERA -> CUALQUIERA
	NEW_POKEMON,
	APPEARED_POKEMON,
	CATCH_POKEMON,
	CAUGHT_POKEMON,
	GET_POKEMON,
	LOCALIZED_POKEMON
} CodigoDeOperacion;

extern char* CodigoDeOperacionAString(CodigoDeOperacion codigoDeOperacion);
