/*
 * comunicacion.cpp
 *
 *  Created on: Sep 4, 2020
 *      Author: xx
 */

#include "comunicacion.h"

extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;

//uint8_t mac[6] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
uint8_t mac[6] = { 0xE4, 0xAB, 0x89, 0x1B, 0xF1, 0xA0};
//char server[] = "www.google.com";
IPAddress ip_server(172, 217, 162, 4);

IPAddress ip(192, 168, 1, 70);
IPAddress myDns(192, 168, 0, 1);
EthernetClient client;

#ifdef SERVER
EthernetServer server(80);
#endif

int8_t byteCount = -1;

static uint8_t buffer[1024];

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
	if (client.connect(ip_server, 80)) {
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
	// Crea conexion cliente
	client = server.available();
	if (!client) return;
	currentLineIsBlank = true;
	while (client.connected()) {
		if (client.available()) {
			c = client.read();
			msj = (uint8_t) c;
			HAL_UART_Transmit(&huart1, &msj, 1, 100);
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
			if (c == '\n') {
				// you're starting a new line
				currentLineIsBlank = true;
			} else if (c != '\r') {
				// you've gotten a character on the current line
				currentLineIsBlank = false;
			}
		}
	}
	HAL_UART_Transmit(&huart1, (uint8_t *)"\nFin\n", 4, 100);
	HAL_Delay(15);
	client.stop();
}

