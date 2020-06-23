#include "../Utils/net.h"
#include "../Utils/eventos.h"
#include "../Utils/protocolo.h"

extern t_list* id_mensajes_esperados;

typedef struct
{
	Cliente* cliente;
	void* operacion;
	CodigoDeOperacion codigo_operacion;
	CodigoDeCola cola;
}Datos_Suscripcion;

Cliente* crear_cliente_de_broker();
void conectarse_y_suscribirse_a_colas();
void solicitar_pokemons_para_objetivo_global();

void operacion_APPEARED_POKEMON(Cliente* cliente, Paquete* paquete);
