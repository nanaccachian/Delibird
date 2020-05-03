#include "protocolo.h"
#include <stdlib.h>

//TODO cuando ger termine las colas hay que hacer la de suscriptor

// FUNCIONES INDIVIDUALES PARA CADA SERIALIZAR
//2
void* Serializar_NEW_POKEMON(DATOS_NEW_POKEMON* datos, int* tamanioBuffer)
{
	*tamanioBuffer = datos->largoPokemon + sizeof(uint32_t)*4;
	void* buffer = malloc(*tamanioBuffer);

	int desplazamiento = 0;
	memcpy(buffer, &(datos->largoPokemon), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, datos->pokemon, datos->largoPokemon);
	desplazamiento += datos->largoPokemon;
	memcpy(buffer + desplazamiento, &((datos->posicion).posX), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, &((datos->posicion).posY), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, &(datos->cantidad), sizeof(uint32_t));

	return buffer;
}

//3
void* Serializar_APPEARED_POKEMON(DATOS_APPEARED_POKEMON* datos, int* tamanioBuffer)
{
	*tamanioBuffer = datos->largoPokemon + sizeof(uint32_t)*4;
	void* buffer = malloc(*tamanioBuffer);

	int desplazamiento = 0;
	memcpy(buffer, &(datos->largoPokemon), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, datos->pokemon, datos->largoPokemon);
	desplazamiento += datos->largoPokemon;
	memcpy(buffer + desplazamiento, &((datos->posicion).posX), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, &((datos->posicion).posY), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t); //BORRE EL ID

	return buffer;
}

//4
void* Serializar_CATCH_POKEMON(DATOS_CATCH_POKEMON* datos, int* tamanioBuffer)
{
	*tamanioBuffer = datos->largoPokemon + sizeof(uint32_t)*3;
	void* buffer = malloc(*tamanioBuffer);

	int desplazamiento = 0;
	memcpy(buffer, &(datos->largoPokemon), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer+desplazamiento, datos->pokemon, datos->largoPokemon);
	desplazamiento += datos->largoPokemon;
	memcpy(buffer + desplazamiento, &((datos->posicion).posX), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, &((datos->posicion).posY), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	return buffer;
}

//5
void* Serializar_CAUGHT_POKEMON(DATOS_CAUGHT_POKEMON* datos, int* tamanioBuffer)
{
	*tamanioBuffer = sizeof(uint32_t)*2;
	void* buffer = malloc(*tamanioBuffer);

	int desplazamiento = 0; //BORRE EL ID
	memcpy(buffer + desplazamiento, &(datos->capturado), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	return buffer;
}

//6
void* Serializar_GET_POKEMON(DATOS_GET_POKEMON* datos, int* tamanioBuffer)
{
	*tamanioBuffer = datos->largoPokemon + sizeof(uint32_t);
	void* buffer = malloc(*tamanioBuffer);

	int desplazamiento = 0;
	memcpy(buffer, &(datos->largoPokemon), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer+desplazamiento, datos->pokemon, datos->largoPokemon);

	return buffer;
}

//7 TODO serializar localized pokemon hacer
void* Serializar_LOCALIZED_POKEMON(DATOS_LOCALIZED_POKEMON* datos, int* tamanioBuffer){

	*tamanioBuffer = datos->largoPokemon + sizeof(uint32_t) + (datos->cantidad)*(sizeof(uint32_t)*2);
	void* buffer = malloc(*tamanioBuffer);
/*
	int desplazamiento = 0;
	memcpy(paqueteSerializado, &(datos->ID_MENSAJE), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(paqueteSerializado, &(datos->largoPokemon), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(paqueteSerializado+desplazamiento, &(datos->pokemon), datos->largoPokemon);
	desplazamiento += datos->largoPokemon;
	memcpy(paqueteSerializado + desplazamiento, &(datos->cantidad), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	for (int i = 0; i < datos->cantidad; i++){
		memcpy(paqueteSerializado + desplazamiento, &(datos->posiciones->posX), sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(paqueteSerializado + desplazamiento, &(datos->posiciones->posY), sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
	}
*/
	return buffer;
}

void* Serializar_ID_MENSAJE(uint32_t* ID, void* buffer, int* tamanioBuffer) {

	int desplazamiento = sizeof(&tamanioBuffer);

	*tamanioBuffer += sizeof(uint32_t);

	memcpy(buffer + desplazamiento, &ID,sizeof(uint32_t));

	return buffer;
}

// FUNCIONES INDIVIDUALES PARA CADA DESSERIALIZAR


bool Deserializar_ID_MENSAJE(Paquete* paquete, DATOS_MENSAJE* datos)
{
	if (!Paquete_Deserializar(paquete, &(datos->ID), sizeof(uint32_t))) return false;
	return true;
}

//2
bool Deserializar_NEW_POKEMON(Paquete* paquete, DATOS_NEW_POKEMON* datos)
{
	if (!Paquete_Deserializar(paquete, &(datos->largoPokemon), sizeof(uint32_t))) return false;
	if (!Paquete_DeserializarString(paquete, &(datos->pokemon), datos->largoPokemon)) return false; //todo comentar a ger esto
	if (!Paquete_Deserializar(paquete, &((datos->posicion).posX), sizeof(uint32_t))) return false;
	if (!Paquete_Deserializar(paquete, &((datos->posicion).posY), sizeof(uint32_t))) return false;
	if (!Paquete_Deserializar(paquete, &(datos->cantidad), sizeof(uint32_t))) return false;
	return true;
}

//3
bool Deserializar_APPEARED_POKEMON(Paquete* paquete, DATOS_APPEARED_POKEMON* datos)
{
	if (!Paquete_Deserializar(paquete, &(datos->largoPokemon), sizeof(uint32_t))) return false;
	if (!Paquete_DeserializarString(paquete, &(datos->pokemon), datos->largoPokemon)) return false;
	if (!Paquete_Deserializar(paquete, &((datos->posicion).posX), sizeof(uint32_t))) return false;
	if (!Paquete_Deserializar(paquete, &((datos->posicion).posY), sizeof(uint32_t))) return false; //BORRE EL ID, FUNCION APARTE
	return true;
}

//4
bool Deserializar_CATCH_POKEMON(Paquete* paquete, DATOS_CATCH_POKEMON* datos)
{
	if (!Paquete_Deserializar(paquete, &(datos->largoPokemon), sizeof(uint32_t))) return false;
	if (!Paquete_DeserializarString(paquete, &(datos->pokemon), datos->largoPokemon)) return false;
	if (!Paquete_Deserializar(paquete, &((datos->posicion).posX), sizeof(uint32_t))) return false;
	if (!Paquete_Deserializar(paquete, &((datos->posicion).posY), sizeof(uint32_t))) return false;
	return true;
}

//5
bool Deserializar_CAUGHT_POKEMON(Paquete* paquete, DATOS_CAUGHT_POKEMON* datos)
{
	if (!Paquete_Deserializar(paquete, &(datos->capturado), sizeof(uint32_t))) return false; //BORRE EL ID FUNCION APARTE
	return true;
}

//6
bool Deserializar_GET_POKEMON(Paquete* paquete, DATOS_GET_POKEMON* datos)
{
	if (!Paquete_Deserializar(paquete, &(datos->largoPokemon), sizeof(uint32_t))) return false;
	if (!Paquete_DeserializarString(paquete, &(datos->pokemon), datos->largoPokemon)) return false;
	return true;
}

//7
bool Deserializar_LOCALIZED_POKEMON(Paquete* paquete, DATOS_LOCALIZED_POKEMON* datos) {
	//TODO deserializar localized
	return true;
}

void* Serializar_BROKER_RECONECTAR(Broker_DATOS_RECONECTAR* datos, int* tamanioBuffer)
{
	*tamanioBuffer = sizeof(Broker_DATOS_RECONECTAR);
	void* stream = malloc(*tamanioBuffer);

	int desplazamiento = 0;
	memcpy(stream, &(datos->id), sizeof(datos->id));
	desplazamiento += sizeof(datos->id);

	return stream;
}
bool Deserializar_BROKER_RECONECTAR(int socket, Broker_DATOS_RECONECTAR* datos)
{
	datos = malloc(sizeof(Broker_DATOS_RECONECTAR));

	uint32_t verificador = 0;

	verificador += recv(socket, &(datos->id), sizeof(datos->id), 0);

	return verificador == sizeof(Broker_DATOS_RECONECTAR);
}

void* Serializar_BROKER_CONECTADO(Broker_DATOS_RECONECTAR* datos, int* tamanioBuffer)
{
	return Serializar_BROKER_RECONECTAR(datos, tamanioBuffer);
}
bool Deserializar_BROKER_CONECTADO(int socket, Broker_DATOS_RECONECTAR* datos)
{
	return Deserializar_BROKER_RECONECTAR(socket, datos);
}
