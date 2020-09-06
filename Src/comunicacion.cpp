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

// ############################################################################
// VARIABLES INTERNAS

extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;

// Estado de la maquina de estados de la comunicacion
uint16_t estado = 0;

// Mac del dispositivo
uint8_t mac[6] = {MAC};

//IP del servidor al que se conectara el cliente
IPAddress ip_server(IP_SERVIDOR);

IPAddress ip(IP_PROPIA);
IPAddress myDns(192, 168, 0, 1);

// Objeto cliente
EthernetClient client;

// Objeto servidor
EthernetServer server(PUERTO);

int8_t byteCount = -1;

static uint8_t buffer[1024];

bool isMsjInicializado;

// ############################################################################
// CABECERA DE FUNCIONES INTERNAS
bool recibir_conexion();

// ############################################################################
// FUNCIONES EXTERNAS

// ---------- Cliente -------------

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
	byteCount = -1;
}

void connect_to_server() {
	if (byteCount != -1)
		return; // only send request after finished the reception
	HAL_UART_Transmit(&huart1, (uint8_t *)"*****\n", 6, 100);
	HAL_Delay(1000);
	HAL_UART_Transmit(&huart1, (uint8_t *)"Conectando\n", 11, 100);
	// if you get a connection, report back via serial:
	if (client.connect(ip_server, PUERTO)) {
		HAL_UART_Transmit(&huart1, (uint8_t *)"Conectado\n", 10, 100);
		// Make a HTTP request:
		//client.println("GET /search?q=arduino HTTP/1.1"); // for Google
		//client.println("GET /search?q=arduino HTTP/1.1"); // for Google
		client.println("GET / HTTP/1.1");
		client.print("Host: ");
		client.println(ip_server);
		client.println("Connection: close");
		client.println();
		byteCount = 0;
	} else {
		// if you didn't get a connection to the server:
		HAL_UART_Transmit(&huart1, (uint8_t *)"Fallo conexion\n", 15, 100);
	}
}

void check_client() {
	if (byteCount < 0)
		return;
	// if there are incoming bytes available
	// from the server, read them and print them:
	uint16_t len;
	while ((len = client.available()) > 0) {
		if (len > 1024)
			len = 1024;
		//Serial.print("-> received "); Serial.println(len); //Serial.print(" bytes in "); Serial.println((float)(endMicros-beginMicros) / 1000000.0);
		client.read(buffer, len);
		HAL_UART_Transmit(&huart1, (uint8_t *)buffer, len, 100);
		byteCount += len;
	}
	// if the server's disconnected, stop the client:
	if (byteCount > 0 && !client.connected()) {
		HAL_UART_Transmit(&huart1, (uint8_t *)"desconectando\n", 14, 100);
		client.stop();
		//Serial.print("\nReceived ");
		//Serial.print(byteCount);
		//Serial.print(" bytes in ");
		//Serial.print(seconds, 4);
		//Serial.print(", rate = ");
		//Serial.print(rate);
		//Serial.print(" kbytes/second");
		//Serial.println();
		byteCount = -1;
	}
}


//---------- Servidor --------------

void inicializar_servidor(SPI_HandleTypeDef &_spi, UART_HandleTypeDef &_uart) {
	HAL_InitTick(0);
	Ethernet.init(hspi1, 1);
	HAL_UART_Transmit(&huart1, (uint8_t*) "Inicializa Ethernet con IP fija\n", 32,
			100);
	Ethernet.begin(mac, ip);
	server.begin();
	HAL_UART_Transmit(&huart1, (uint8_t*) "Hecho\n", 6, 100);
}

void escuchar_cliente() {
	char c;
	bool currentLineIsBlank;;
	uint8_t msj;

	uint8_t tout = 10;

	// Crea conexion cliente
	client = server.available();
	if (!client) return;
	currentLineIsBlank = true;
	while (client.connected()) {
		if (client.available()) {
			/*if (tout == 0) {
				break;
			}*/
			c = client.read();
			msj = (uint8_t) c;
			HAL_UART_Transmit(&huart1, &msj, 1, 100);
			/*
			if (c == '\n' && currentLineIsBlank) {
				client.println("HTTP/1.1 200 OK"); //envia una nueva pagina en codigo HTML
				client.println("Content-Type: text/html");
				client.println();
				client.println("<HTML>");
				client.println("<HEAD>");
				client.println("<TITLE>Ethernet Arduino</TITLE>");
				client.println("</HEAD>");
				client.println("<BODY>");
				client.println("<h1>Lo logre</h1>");
				client.println("<BODY>");
				break;
			}
			*/
			if (c == '1') {
				HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
				break;
			} else {
				HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
				break;
			}
			if (c == '\n') {
				// you're starting a new line
				currentLineIsBlank = true;
			} else if (c != '\r') {
				// you've gotten a character on the current line
				currentLineIsBlank = false;
			}
			tout--;
		}
	}
	HAL_UART_Transmit(&huart1, (uint8_t *)"\nFin\n", 4, 100);
	HAL_Delay(15);
	client.stop();
}

bool recibir_conexion() {
	client = server.available();
	if (!client)
		return false;
	return true;
}
/*
char leer_mensaje() {
	char caracter_leido;
	caracter_leido = client.read();

	return caracter_leido;
}*/

void me_servidor(SPI_HandleTypeDef &_spi, UART_HandleTypeDef &_uart) {
	int i;
	char c;
	uint8_t msj;
	switch(estado) {
	case 0:	// Obtiene direccion IP
		inicializar_servidor(_spi, _uart);
		estado++;
		break;
	case 1:	// Espera conexion de cliente
		if (recibir_conexion()) {
			// Entra si se conecto un cliente al servidor
			i = 0;
			isMsjInicializado = false;
			estado++;
		}
		break;
	case 2:	// Lee msj desde cliente
		if (client.available()) {
			c = client.read();
			msj = (uint8_t) c;
			if (c == ':') {
				// Entra si recibio inicializados
				i = 0;
				isMsjInicializado = true;
			}
			if (isMsjInicializado) {
				// Entra si ya ha recibido el inicializador
				HAL_UART_Transmit(&huart1, &msj, 1, 100);
				buffer[i++] = msj;
			}
			if (c == ';') {
				// Entra si recibio terminador
				isMsjInicializado = false;
				buffer[i] = msj;
				estado++;
			}
		}
		break;
	case 3:	// Interpreta msj recibido
		if (buffer[1] == '1') {
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
		} else {
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
		}
		estado++;
		break;
	case 4:	// Contesta a cliente
		estado++;
		break;
	case 5: // Desconecta del cliente
		client.stop();
		estado = 1;
		break;
	}
}

// ############################################################################
