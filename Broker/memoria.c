#include "broker.h"
#include "mensaje.h"
#include "clienteBroker.h"
#include "particion.h"
#include "dynamicPartitioning.h"
#include "buddySystem.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

void* memoria;
pthread_mutex_t mutexMemoria;
int tamanioMinimo;

void (*compactar)();
Particion* (*seleccionar)(int);
void (*eliminarParticion)();
void (*dump)();
void (*destuir)();
int frecuenciaCompactacion;

int _tamanioMemoria;
void IniciarMemoria(int tamanioMemoria, int _tamanioMinimo, char* algoritmoMemoria, char* algoritmoReemplazo, char* algoritmoSeleccion, int _frecuenciaCompactacion)
{
	_tamanioMemoria = tamanioMemoria;
	tamanioMinimo = _tamanioMinimo;

	// algoritmoMemoria
	if (strcmp(algoritmoMemoria, "PARTICIONES") == 0) // Particiones dinámicas
	{
		compactar = &DP_Compactar;
		dump = &DP_Dump;
		destuir = &DP_Destruir;

		if (strcmp(algoritmoSeleccion, "FF") == 0)
			seleccionar = &DP_Seleccionar_FF;
		else if (strcmp(algoritmoSeleccion, "BF") == 0)
			seleccionar = &DP_Seleccionar_BF;
		else
		{
			log_error(logger, "Algorimo de selección inválido");
			exit(-1);
		}

		if (strcmp(algoritmoReemplazo, "FIFO") == 0)
			eliminarParticion = &DP_Eliminar_FIFO;
		else if (strcmp(algoritmoReemplazo, "LRU") == 0)
			eliminarParticion = &DP_Eliminar_LRU;
		else
		{
			log_error(logger, "Algorimo de reemplazo inválido");
			exit(-1);
		}

		DP_Inicializar(tamanioMemoria);
	}
	else if (strcmp(algoritmoMemoria, "BS") == 0) // BuddySystem
	{
		compactar = NULL;
		seleccionar = &BS_Seleccionar;
		dump = &BS_Dump;
		destuir = &BS_Destruir;

		if (strcmp(algoritmoReemplazo, "FIFO") == 0)
			eliminarParticion = &BS_Eliminar_FIFO;
		else if (strcmp(algoritmoReemplazo, "LRU") == 0)
			eliminarParticion = &BS_Eliminar_LRU;
		else
		{
			log_error(logger, "Algorimo de reemplazo inválido");
			exit(-1);
		}

		BS_Inicializar(tamanioMemoria);
	}
	else
	{
		log_error(logger, "Algorimo de memoria inválido");
		exit(-1);
	}

	frecuenciaCompactacion = _frecuenciaCompactacion;

	memoria = malloc(tamanioMemoria);
	pthread_mutex_init(&mutexMemoria, NULL);
}

static Particion* SeleccionDeParticion(int tamanio)
{
	if (tamanio > _tamanioMemoria)
	{
		log_error(logger, "Se intento crear una particion de tamanio mayor que la cache");
		return NULL;
	}

	Particion* particion = NULL;
	int intentos = 0;

	particion = seleccionar(tamanio);
	if (particion != NULL)
		return particion;
	intentos++;

	while(true)
	{
		if (compactar != NULL && intentos == frecuenciaCompactacion/* || (frecuenciaCompactacion == -1 && particiones->elements_count == 1 && !((Particion*)(list_get(particiones, 0)))->ocupado)*/)
		{
			log_info(logger, "Compactando");
			compactar();
			intentos = 0;

			particion = seleccionar(tamanio);
			if (particion != NULL)
				return particion;
		}

		eliminarParticion();

		particion = seleccionar(tamanio);
		if (particion != NULL)
			return particion;
		intentos++;
	}
	return NULL;
}
void GuardarMensaje(Mensaje* mensaje, CodigoDeCola tipoDeMensaje, Stream* contenido)
{
	pthread_mutex_lock(&mutexMemoria);
	Particion* particion = SeleccionDeParticion(contenido->tamanio);

	particion->id = mensaje->id;
	particion->ocupado = true;
	particion->cola = tipoDeMensaje;
	particion->tiempoAsignado = clock();
	particion->tiempoUpdated = clock();
	memcpy(memoria + particion->base, contenido->base, contenido->tamanio);
	pthread_mutex_unlock(&mutexMemoria);

	mensaje->particion = particion;
}

void* ObtenerContenidoMensaje(Mensaje* mensaje)
{
	void* contenido = malloc(mensaje->tamanio);
	pthread_mutex_lock(&mutexMemoria);
	memcpy(contenido, memoria + mensaje->particion->base, mensaje->tamanio);
	mensaje->particion->tiempoUpdated = clock();
	pthread_mutex_unlock(&mutexMemoria);
	return contenido;
}

void Dump()
{
	FILE* archivo = fopen("dump.txt", "a");
	if(archivo == NULL) {
		log_error(logger, "No se pudo crear Dump (dump.txt)");
		return;
	}

	fprintf(archivo, "-----------------------------------------------------------------------------------------------------------------------------\n");

	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	fprintf(archivo, "Dump: %d/%d/%d %d:%d:%d\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);

	pthread_mutex_lock(&mutexMemoria);
	dump(archivo);
	pthread_mutex_unlock(&mutexMemoria);

	fclose(archivo);
	log_info(logger, "Dump! (dump.txt)");
}

void DestruirMemoria()
{
	free(memoria);
	destuir();
}
