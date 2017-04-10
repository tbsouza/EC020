/*****************************************************************************
 *   Value read from BNC is written to the OLED display (nothing graphical
 *   yet only value).
 *
 *   Copyright(C) 2010, Embedded Artists AB
 *   All rights reserved.
 *
 ******************************************************************************/

// *************************** INCLUDES ***************************
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
// *****************************************************************


// ******** Definições *************
#define UART_DEV LPC_UART3
//**********************************************


static uint8_t buf[10];

// ************ Usado na temperatura ************
uint32_t getMsTicks(void);
static uint32_t msTicks = 0;
//**********************************************

// ************* Inicializa UART *************
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
//**********************************************


//*************************************************************************************
static void intToString(int value, uint8_t* pBuf, uint32_t len, uint32_t base)
{
    static const char* pAscii = "0123456789abcdefghijklmnopqrstuvwxyz";
    int pos = 0;
    int tmpValue = value;

    // the buffer must not be null and at least have a length of 2 to handle one
    // digit and null-terminator
    if (pBuf == NULL || len < 2)
    {
        return;
    }

    // a valid base cannot be less than 2 or larger than 36
    // a base value of 2 means binary representation. A value of 1 would mean only zeros
    // a base larger than 36 can only be used if a larger alphabet were used.
    if (base < 2 || base > 36)
    {
        return;
    }

    // negative value
    if (value < 0)
    {
        tmpValue = -tmpValue;
        value    = -value;
        pBuf[pos++] = '-';
    }

    // calculate the required length of the buffer
    do {
        pos++;
        tmpValue /= base;
    } while(tmpValue > 0);


    if (pos > len)
    {
        // the len parameter is invalid.
        return;
    }

    pBuf[pos] = '\0';

    do {
        pBuf[--pos] = pAscii[value % base];
        value /= base;
    } while(value > 0);

    return;
}
//*************************************************************************************


//*************************** INITs ***********************************
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
// *************************************************************************


// *************************** MAIN ***************************
int main (void) {

    uint32_t val  = 0;

    // Variaveis UART
    uint8_t uart1Read = 1;
    uint32_t recvd = 0;
    uint32_t len = 0;
    uint8_t data = 0;

    // Modos:
    //       1 - vaalvula automatica (inicial)
    //		 2 - Abre manualmente
    // 		 3 - Fecha manualmente
    //		 4 - Mostra temperatura
    uint8_t mode = 1;

    // Inicialização
    init_i2c();
    init_ssp();
    init_adc();
    oled_init();
    init_uart();
    uart2_init(115200, CHANNEL_A);
    led7seg_init();

    // Init sensor temperatura
    SysTick_Config(SystemCoreClock / 1000);
    temp_init (&getMsTicks);

    // Deixa oled fundo preto
    oled_clearScreen(OLED_COLOR_BLACK);

    // Escreve no oled
    oled_putString(1,1,  "Temp: ", OLED_COLOR_WHITE, OLED_COLOR_BLACK);

    // Seta led 7 segmentos para 1 inicialmente
    led7seg_setChar( '1', 0 );

    while(1) {

        /* analog input connected to BNC */
    	ADC_StartCmd(LPC_ADC,ADC_START_NOW);
    	//Wait conversion complete
    	while (!(ADC_ChannelGetStatus(LPC_ADC,ADC_CHANNEL_5,ADC_DATA_DONE)));
    	val = ADC_ChannelGetData(LPC_ADC,ADC_CHANNEL_5);


    	// Le temperatura e passa para celsius
    	int32_t celsius = temp_read()/10;

    	// Converte int32_t para char* para exibir no oled
    	uint8_t *c[3];
    	sprintf(c, "%d", celsius);

        //oled_fillRect((1+6*6),1, 80, 8, OLED_COLOR_BLACK); // Desenha retangulo no oled
        oled_putString((1+6*6),1, c, OLED_COLOR_WHITE, OLED_COLOR_BLACK);

        // Leitura do UART
        len = UART_Receive(UART_DEV, &data, 1, NONE_BLOCKING);

        // Mudança dos modos
        // len>0 significa alguma informação chegando no uart
        if( (len>0) && (data)=='1' ){ mode = 1; }

        if( (len>0) && (data)=='2' ){ mode = 2; }

        if( (len>0) && (data)=='3' ){ mode = 3; }

        if( (len>0) && (data)=='4' ){ mode = 4; }

        // Ações de cada modo


        if( mode == 1 ){ // Modo 1

        	// Led & segmentos valor 1
        	led7seg_setChar( '1', 0 );

			if( celsius  >= 25 ){
				oled_putString(1,12,  "Valvula  aberta", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
			}else if( celsius  < 22 ){
				oled_putString(1,12,  "Valvula fechada", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
			}
        }


        else if( mode == 2 ){ // Modo 2

        	led7seg_setChar( '2', 0 );
        	oled_putString(1,12,  "Valvula  aberta", OLED_COLOR_WHITE, OLED_COLOR_BLACK);

        }else if( mode == 3 ){ // Modo 3

        	led7seg_setChar( 'E', 0 );
        	oled_putString(1,12,  "Valvula fechada", OLED_COLOR_WHITE, OLED_COLOR_BLACK);

        }else if( mode == 4 ){ // Modo 4

        	led7seg_setChar( '-', 0 );

        	UART_SendString(UART_DEV, (uint8_t*)c );	  // mostra temp no uart
        	UART_SendString(UART_DEV, (uint8_t*)"\r\n" ); // Pula linha

        	Timer0_Wait(350);

        	// Mostra a temperatura no uart e volta para modo 1
        	mode = 1;
        }

        /* delay */
        Timer0_Wait(1000);
    }
}
// *********************************************************************************


// *************************** Temperatura ***************************
void SysTick_Handler(void) {
    msTicks++;
}

uint32_t getMsTicks(void)
{
    return msTicks;
}
// *******************************************************************

// *******************************************************************
void check_failed(uint8_t *file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while(1);
}
// *******************************************************************

