/*
 * comunicacion.cpp
 *
 *  Created on: Sep 4, 2020
 *      Author: xx
 */

// ############################################################################
// INCLUDE
#include "comunicacion.h"


// ############################################################################
// DEFINE
#ifdef SERVER
#define MAC				0xE4, 0xAB, 0x89, 0x1B, 0xF1, 0xA1
#define IP_PROPIA 		192, 168, 88, 30
#define IP_SERVIDOR		192, 168, 88, 70
#elif CLIENT
#define MAC				0xE4, 0xAB, 0x89, 0x1B, 0xF1, 0xA0
#define IP_PROPIA 		192, 168, 88, 40
#define IP_SERVIDOR		192, 168, 88, 30
#endif
#define PUERTO			3030

#define MAX_BUFFER	20

#define INICIALIZADOR	':'
#define FINALIZADOR	';'

// ############################################################################
// VARIABLES INTERNAS

extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;

// Estado y subestado de la maquina de estados de la comunicacion
uint16_t estado = 0;
uint16_t sub_estado = 0;

// Mac del dispositivo
uint8_t mac[6] = { MAC };

// IP del servidor al que se conectara el cliente
IPAddress ip_server(IP_SERVIDOR);

// IP propia fija
IPAddress ip(IP_PROPIA);
IPAddress myDns(192, 168, 0, 1);

// Objeto cliente
EthernetClient client;

// Objeto servidor
EthernetServer server(PUERTO);

// Buffer de lectura para comunicacion
static uint8_t buffer_read[20];

// Indica que recibio el inicializador de msj
bool isMsjInicializado;

// Indica que debe responder/Indica que esta esperando respuesta
bool mustResponder;

// ############################################################################
// CABECERA DE FUNCIONES INTERNAS
bool recibir_conexion();
bool debe_responder(void);
bool no_debe_responder(void);
void responder_consulta(void);
// ############################################################################
// CABECERA DE FUNCIONES INTERNAS

// Funcion para recibir conexion de un cliente al servidor
bool recibir_conexion() {
	client = server.available();
	if (!client)
		return false;
	return true;
}

// Funcion para indicar cuando debe responder
bool debe_responder() {
	mustResponder = true;
	return mustResponder;
}

bool no_debe_responder() {
	mustResponder = false;
	return mustResponder;
}

void responder_consulta() {
	char _buf[3];
	_buf[0] = INICIALIZADOR;
	_buf[1] = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) ? '1' : '0';
	_buf[2] = FINALIZADOR;
	client.print(_buf);
}

// ############################################################################
// FUNCIONES PUBLICAS

// ---------- Cliente -------------

// Funcion para inicializar el cliente
void inicializar_cliente(SPI_HandleTypeDef &_spi, UART_HandleTypeDef &_uart) {
	HAL_InitTick(0);	// Inicia interrupcion de SysTick cada 1ms para millis()
	Ethernet.init(hspi1, 1);
	HAL_UART_Transmit(&huart1, (uint8_t*) "Inicializa Ethernet con DHCP\n", 29,
			100);
	if (!Ethernet.begin(mac)) {
		HAL_UART_Transmit(&huart1, (uint8_t*) "Fallo usando DHCP\n", 18, 100);
		Ethernet.begin(mac, ip, myDns);
	} else {
		HAL_UART_Transmit(&huart1, (uint8_t*) "Conecto usando DHCP\n", 20, 100);
	}
}

//---------- Servidor --------------

// Funcion para inicializar el servidor
void inicializar_servidor(SPI_HandleTypeDef &_spi, UART_HandleTypeDef &_uart) {
	HAL_InitTick(0);
	Ethernet.init(hspi1, 1);
	HAL_UART_Transmit(&huart1, (uint8_t*) "Inicializa Ethernet con IP fija\n", 32,
			100);
	Ethernet.begin(mac, ip);
	server.begin();
	HAL_UART_Transmit(&huart1, (uint8_t*) "Hecho\n", 6, 100);
}

// Maquina de estados del servidor para comunicacion
void me_servidor(SPI_HandleTypeDef &_spi, UART_HandleTypeDef &_uart) {
	static int idx_buffer;
	int16_t length;
	switch(estado) {
	case 0:	// Obtiene direccion IP
		inicializar_servidor(_spi, _uart);
		estado++;
		break;
	case 1:	// Espera conexion de cliente
		if (recibir_conexion()) {
			// Entra si se conecto un cliente al servidor
			idx_buffer = 0;
			isMsjInicializado = false;
			estado++;
			sub_estado = 1;
		}
		break;
	case 2:	// Lee msj desde cliente
		switch (sub_estado) {
		case 1:	// Lee la respuesta
			length = client.available();
			if (length > 20) {
				length = 20;
			}
			client.read(buffer_read, length);
			idx_buffer = 0;
			sub_estado++;
			break;
		case 2:	// Busca los terminadores
			if (buffer_read[0] == INICIALIZADOR) {
				for (int i = 0; i < MAX_BUFFER; i++) {
					if (buffer_read[i] == FINALIZADOR) {
						idx_buffer = i;
						break;
					}
				}
			}
			sub_estado++;
		case 3:	// Verifica que esten ambos
			if (buffer_read[0]
					== INICIALIZADOR && buffer_read[idx_buffer] == FINALIZADOR) {
				estado++;
			} else {
				estado = 5;
			}
		}
		break;
	case 3:	// Interpreta msj recibido
		switch (buffer_read[1]) {
		case 'C':
			debe_responder();
			break;
		case '0':
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
			no_debe_responder();
			break;
		case '1':
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
			no_debe_responder();
			break;
		}
		estado++;
		break;
	case 4:	// Contesta a cliente
		if (mustResponder) {
			// Entra si debe responder al cliente
			switch (buffer_read[1]) {
			case 'C':
				responder_consulta();
				break;
			}
		}
		estado++;
		break;
	case 5: // Desconecta del cliente
		client.stop();
		estado = 1;
		break;
	}
}

// ############################################################################
