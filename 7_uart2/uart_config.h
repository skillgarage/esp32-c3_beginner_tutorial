#pragma once

#include "driver/uart.h"

#define UART_PORT UART_NUM_1
#define UART_BAUD_RATE 115200
#define UART_BUF_SIZE 256

#define TX_PIN 4
#define RX_PIN 5

void uart_init(void);