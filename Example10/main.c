// EXEMPLO 10

/*
 FreeRTOS V6.1.1 - Copyright (C) 2011 Real Time Engineers Ltd.

 This file is part of the FreeRTOS distribution.

 FreeRTOS is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License (version 2) as published by the
 Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
 ***NOTE*** The exception to the GPL is included to allow you to distribute
 a combined work that includes FreeRTOS without being obliged to provide the
 source code for proprietary components outside of the FreeRTOS kernel.
 FreeRTOS is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 more details. You should have received a copy of the GNU General Public
 License and the FreeRTOS license exception along with FreeRTOS; if not it
 can be viewed here: http://www.freertos.org/a00114.html and also obtained
 by writing to Richard Barry, contact details for whom are available on the
 FreeRTOS WEB site.

 1 tab == 4 spaces!

 http://www.FreeRTOS.org - Documentation, latest information, license and
 contact details.

 http://www.SafeRTOS.com - A version that is certified for use in safety
 critical systems.

 http://www.OpenRTOS.com - Commercial support, development, porting,
 licensing and training services.
 */

/****************************************************************************************/
// LIBRAIES
/* FreeRTOS.org includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Demo includes. */
#include "basic_io.h"

#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_ssp.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_uart.h"

#include "oled.h"
#include "temp.h"
#include "uart2.h"
#include "led7seg.h"
/****************************************************************************************/


/****************************************************************************************/
// TASKS
void taskSensor(void *pvParameters); // Tarefa para leitura e tratamento do sensor
void taskValve(void *pvParameters);  // Tarefa para tratamento da válvula
void taskUART(void *pvParameters);   // Tarefa para ler a UART
void taskMode(void *pvParameters);   // Tarefa para tratamento dos dados recebidos da UART
/****************************************************************************************/


/****************************************************************************************/
//QUEUES
xQueueHandle queueSensor; // Fila para valores lidos do Sensor
xQueueHandle queueUART;   // Fila para valores lidos da UART
/****************************************************************************************/

// Modos:
    //       1 - Válvula automática (inicial//padrão)
    //		 2 - Abre manualmente
    // 		 3 - Fecha manualmente
    //		 4 - Mostra temperatura no UART
int mode = 1; // modo de funcionamento do sistema

uint8_t *temp[3];

//static uint8_t buf[10];

#define UART_DEV LPC_UART3
// Taxa: 115200

/************************ Usado na Temperatura *******************************************/
uint32_t getMsTicks(void);
static uint32_t msTicks = 0;

void SysTick_Handler(void) {
    msTicks++;
}

uint32_t getMsTicks(void)
{
    return msTicks;
}
/****************************************************************************************/


/************************************ INITs *********************************************/
static void init_uart(void)
{
	PINSEL_CFG_Type PinCfg;
	UART_CFG_Type uartCfg;

	/* Initialize UART3 pin connect */
	PinCfg.Funcnum = 2;
	PinCfg.Pinnum = 0;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 1;
	PINSEL_ConfigPin(&PinCfg);

	uartCfg.Baud_rate = 115200;
	uartCfg.Databits = UART_DATABIT_8;
	uartCfg.Parity = UART_PARITY_NONE;
	uartCfg.Stopbits = UART_STOPBIT_1;

	UART_Init(UART_DEV, &uartCfg);

	UART_TxCmd(UART_DEV, ENABLE);
}

static void init_ssp(void)
{
	SSP_CFG_Type SSP_ConfigStruct;
	PINSEL_CFG_Type PinCfg;

	/*
	 * Initialize SPI pin connect
	 * P0.7 - SCK;
	 * P0.8 - MISO
	 * P0.9 - MOSI
	 * P2.2 - SSEL - used as GPIO
	 */
	PinCfg.Funcnum = 2;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 7;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 8;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 9;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Funcnum = 0;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 2;
	PINSEL_ConfigPin(&PinCfg);

	SSP_ConfigStructInit(&SSP_ConfigStruct);

	// Initialize SSP peripheral with parameter given in structure above
	SSP_Init(LPC_SSP1, &SSP_ConfigStruct);

	// Enable SSP peripheral
	SSP_Cmd(LPC_SSP1, ENABLE);
}

static void init_i2c(void)
{
	PINSEL_CFG_Type PinCfg;

	/* Initialize I2C2 pin connect */
	PinCfg.Funcnum = 2;
	PinCfg.Pinnum = 10;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 11;
	PINSEL_ConfigPin(&PinCfg);

	// Initialize I2C peripheral
	I2C_Init(LPC_I2C2, 100000);

	/* Enable I2C1 operation */
	I2C_Cmd(LPC_I2C2, ENABLE);
}

static void init_adc(void)
{
	PINSEL_CFG_Type PinCfg;

	/*
	 * Init ADC pin connect
	 * AD0.5 on P1.31
	 */
	PinCfg.Funcnum = 3;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Pinnum = 31;
	PinCfg.Portnum = 1;
	PINSEL_ConfigPin(&PinCfg);

	/* Configuration for ADC :
	 * 	Frequency at 1Mhz
	 *  ADC channel 5, no Interrupt
	 */
	ADC_Init(LPC_ADC, 1000000);
	ADC_IntConfig(LPC_ADC,ADC_CHANNEL_5,DISABLE);
	ADC_ChannelCmd(LPC_ADC,ADC_CHANNEL_5,ENABLE);

}
/****************************************************************************************/


/****************************************************************************************/
// Função para inicialização do sensor de temperatura
void initSensor(){

		SysTick_Config(SystemCoreClock / 1000);

		// Utilizar o MsTicks do RTOS
		temp_init (&xTaskGetTickCount);
}

void setup_oled(){

	// Deixa oled fundo preto
	oled_clearScreen(OLED_COLOR_BLACK);

	// Escreve "Temp: " no oled
	oled_putString(1,1, "Temp: ", OLED_COLOR_WHITE, OLED_COLOR_BLACK);

	oled_putString(3,40, " :D ", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
}

void setup_7seg(){

	// Led 7 segmentos valor 1 - valor inicial
	led7seg_setChar( '1', 0 );
}

/****************************************************************************************/


/****************************************************************************************/
// Função para inicialização
void initAll(){

		init_i2c();
	    init_ssp();
	    init_adc();
	    oled_init();
	    init_uart();
	    uart2_init(115200, CHANNEL_A);
	    led7seg_init();

	    setup_oled();
	    setup_7seg();
}
/****************************************************************************************/


/****************************************************************************************/
int main(void) {

	// Inicializa as interfaces de comunicação (IART, I@C, ADC, led7seg)
	initAll();

	// -------------------------------------------------------------------
	// DECLARA A QUEUE (10 posições)
	queueSensor = xQueueCreate(10, sizeof(long));
	queueUART = xQueueCreate(10, sizeof(char));
	// -------------------------------------------------------------------

	// queueUART
	if (queueUART != NULL ) {

		// Cria a tarefa que ira colocar o valor do sensor na queue
		xTaskCreate(taskUART, "UART", 240, NULL, 1, NULL);

		// Cria a tarefa que ira ler da queue
		xTaskCreate(taskMode, "Mode", 240, NULL, 1, NULL);

	} else {
		/* The queue could not be created. */
		vPrintString("The queueUART could not be created.\r\n");
	}

	// queueSensor
	if (queueSensor != NULL ) {

		// Cria a tarefa que ira ler da queue
		xTaskCreate(taskValve, "Valve", 240, NULL, 2, NULL);

		// Cria a tarefa que ira colocar o valor do sensor na queue
		xTaskCreate(taskSensor, "Sensor", 240, NULL, 2, NULL);

	} else {
		/* The queue could not be created. */
		vPrintString("The queueSensor could not be created.\r\n");
	}

	/* Start the scheduler so the created tasks start executing. */
	vTaskStartScheduler();

	for (;;);

	return 0;
}
/****************************************************************************************/


/****************************************************************************************/
// TAREFA PARA LEITURA DO SENSOR
void taskSensor(void *pvParameters) {

	//vPrintString("Entrou taskSensor\r\n");

	long lValueToSend;
	portBASE_TYPE xStatus;

	const portTickType xTicksToWait = 300 / portTICK_RATE_MS;

	uint32_t val  = 0;

	// Inicializa o sensor de temperatura
	initSensor();

	for (;;) {

		// Faz a leitura e tratamento do sensor

		/* analog input connected to BNC */
		ADC_StartCmd(LPC_ADC,ADC_START_NOW);
		//Wait conversion complete
		while (!(ADC_ChannelGetStatus(LPC_ADC,ADC_CHANNEL_5,ADC_DATA_DONE)));
		val = ADC_ChannelGetData(LPC_ADC,ADC_CHANNEL_5);

		// Le temperatura do sensor
		int32_t celsius = temp_read()/10;

		// valor da temperatura a ser colocado na fila
		lValueToSend = (long) celsius;

		// Inserir valores na fila
		xStatus = xQueueSendToBack( queueSensor, &lValueToSend, 0 );

		if (xStatus != pdPASS) {
			/* We could not write to the queue because it was full � this must
			 be an error as the queue should never contain more than one item! */
			vPrintString("Could not send to the queue.\r\n");
		}

		sprintf(temp, "%d", (int)celsius);

		//vPrintString("taskSensor\r\n"); // teste

		vTaskDelay( xTicksToWait );
	}
}
/****************************************************************************************/


/****************************************************************************************/
// TAREFA PARA EXIBIR NO DISPLAY (acionar válvula)
void taskValve(void *pvParameters) {

	//vPrintString("Entrou taskValve\r\n");

	long lReceivedValue;
	portBASE_TYPE xStatus;
	const portTickType xTicksToWait = 300 / portTICK_RATE_MS;

	int celsius = 0;

	/* This task is also defined within an infinite loop. */
	for (;;) {

		// Verifica se a queue não está vazia
		if (uxQueueMessagesWaiting(queueSensor) != 0) {

			//vPrintString("Queue should have been empty!\r\n");

			// (Fila, valor recebido, tempo de espera)
			xStatus = xQueueReceive( queueSensor, &lReceivedValue, xTicksToWait );

			// Conseguiu ler da queue com sucesso
			if (xStatus == pdPASS) {

				// Converte  para char* para exibir no oled
				celsius = (int)lReceivedValue;
				uint8_t *c[3];
				sprintf(c, "%d", celsius); // para exibir no display

				// Exibe a temperatura no display
				//oled_fillRect((1+6*6),1, 80, 8, OLED_COLOR_BLACK); // Desenha retangulo no oled
				oled_putString((1+6*6),1, c, OLED_COLOR_WHITE, OLED_COLOR_BLACK);

				// Verifica o modo que está para exiber o status da válvula
				if( mode == 1 ){ // Modo 1 - automático

					if( celsius  >= 22 ){
						oled_putString(1,12,  "Valvula  aberta", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
					}else if( celsius  < 18 ){
						oled_putString(1,12,  "Valvula fechada", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
					}
				 }
				 else if( mode == 2 ){ // Modo 2 - sempre aberta

				     oled_putString(1,12,  "Valvula  aberta", OLED_COLOR_WHITE, OLED_COLOR_BLACK);

				 }else if( mode == 3 ){ // Modo 3 - sempre fechada

					 oled_putString(1,12,  "Valvula fechada", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
				 }

			} else {
				// Não conseguiu ler da queue
				vPrintString("Could not receive from the queue.\r\n");
			}
		}

		//vPrintString("taskValve\r\n");

		vTaskDelay( xTicksToWait );
	}
}
/****************************************************************************************/


/****************************************************************************************/
// TAREFA PARA RECEBER OS DADOS DO UART ENVIADOS PELO USUÁRIO
void taskUART(void *pvParameters) {

	//vPrintString("Entrou taskUART\r\n");

	char lValueToSend;
	portBASE_TYPE xStatus;

	const portTickType xTicksToWait = 300 / portTICK_RATE_MS;

	// Variaveis UART
	uint8_t uart1Read = 1;
	uint32_t recvd = 0;
	uint32_t len = 0;
	uint8_t data = 0;

	uint32_t val  = 0;

	for (;;) {

		// Faz a leitura do UART

		/* analog input connected to BNC */
		ADC_StartCmd(LPC_ADC,ADC_START_NOW);
		//Wait conversion complete
		while (!(ADC_ChannelGetStatus(LPC_ADC,ADC_CHANNEL_5,ADC_DATA_DONE)));
		val = ADC_ChannelGetData(LPC_ADC,ADC_CHANNEL_5);

		// Leitura da UART
		len = UART_Receive(UART_DEV, &data, 1, NONE_BLOCKING);

		lValueToSend = '0';

		// verifica se recebeu algo da UART para colocar na fila
		if( len > 0 ){
			// valor recebido pelo UART a ser colocado na fila
			lValueToSend = (char) data;
		}

		// insere na fila o valor recebido
		xStatus = xQueueSendToBack( queueUART, &lValueToSend, 0 );

		if (xStatus != pdPASS) {
			/* We could not write to the queue because it was full � this must
			 be an error as the queue should never contain more than one item! */
			vPrintString("Could not send to the queue.\r\n");
		}

		//vPrintString("taskUART\r\n");

		vTaskDelay( xTicksToWait );
	}
}
/****************************************************************************************/


/****************************************************************************************/
// TAREFA PARA TRATAR OS DADOS RECEBIS PELO UART
void taskMode(void *pvParameters) {

	//vPrintString("Entrou taskMode\r\n");

	char lReceivedValue = '0';

	portBASE_TYPE xStatus;
	const portTickType xTicksToWait = 300 / portTICK_RATE_MS;

	for (;;) {

		// Verifica se a queue não está vazia
		if (uxQueueMessagesWaiting(queueUART) != 0) {

			// (Fila, valor recebido, tempo de espera)
			xStatus = xQueueReceive( queueUART, &lReceivedValue, xTicksToWait );

			// Conseguiu ler da queue com sucesso
			if (xStatus == pdPASS) {

				// Trata o valor recebido pela UART e altera os modos do sistema
				if( lReceivedValue == '1' ){
					// Led 7 segmentos valor 1
					led7seg_setChar( '1', 0 );
					mode = 1;
				}

				if( lReceivedValue == '2' ){
					// Led 7 segmentos valor 2
					led7seg_setChar( '2', 0 );
					mode = 2;
				}

				if( lReceivedValue == '3' ){
					// Led 7 segmentos valor 3
					//O valor é setado para E, pois é igual a 3 invertido no led7seg
					led7seg_setChar( 'E', 0 );
					mode = 3;
				}

				if( lReceivedValue == '4' ){
					// Led 7 segmentos valor -
					led7seg_setChar( '-', 0 );

					//Envia o valor da temperatura para o UART
					UART_SendString(UART_DEV, (uint8_t*)temp );	  // mostra temp no uart
					UART_SendString(UART_DEV, (uint8_t*)"\r\n" ); // Pula linha

					// Apos enviar os dados, volta para o modo 1 (modo padrão)
					mode = 1;

					// Led 7 segmentos valor -
					led7seg_setChar( '1', 0 );
				}
			}
		}

		//vPrintString("taskMode\r\n");

		vTaskDelay( xTicksToWait );
	}
}
/****************************************************************************************/


/****************************************************************************************/
void vApplicationMallocFailedHook(void) {
	/* This function will only be called if an API call to create a task, queue
	 or semaphore fails because there is too little heap RAM remaining. */
	for (;;)
		;
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName) {
	/* This function will only be called if a task overflows its stack.  Note
	 that stack overflow checking does slow down the context switch
	 implementation. */
	for (;;)
		;
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook(void) {
	/* This example does not use the idle hook to perform any processing. */
}
/*-----------------------------------------------------------*/

void vApplicationTickHook(void) {
	/* This example does not use the tick hook to perform any processing. */
}
/*-----------------------------------------------------------*/
