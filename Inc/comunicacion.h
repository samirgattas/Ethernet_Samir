/*
 * comunicacion.h
 *
 *  Created on: Sep 4, 2020
 *      Author: xx
 */

#ifndef INC_COMUNICACION_H_
#define INC_COMUNICACION_H_

// ############################################################################
// INCLUDE
#include "main.h"
#include "Ethernet_STM32.h"

// ############################################################################
// DEFINE
#define SERVER

// ############################################################################
// FUNCIONES PUBLICAS

// ------ Cliente ------

void inicializar_cliente(SPI_HandleTypeDef &_spi, UART_HandleTypeDef &_uart);
void connect_to_server(void);
void check_client(void);

// ------ Servidor ------

void inicializar_servidor(SPI_HandleTypeDef &_spi, UART_HandleTypeDef &_uart);
void escuchar_cliente(void);
void me_servidor(SPI_HandleTypeDef &_spi, UART_HandleTypeDef &_uart);


#endif /* INC_COMUNICACION_H_ */
