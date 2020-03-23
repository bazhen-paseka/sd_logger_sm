/**
* \file
* \version 1.0
* \author bazhen.levkovets
** \date 2018
*
*************************************************************************************
* \copyright	Bazhen Levkovets
* \copyright	Brovary, Kyiv region
* \copyright	Ukraine
*
*************************************************************************************
*
* \brief
*
*/

/*
**************************************************************************
*							INCLUDE FILES
**************************************************************************
*/
	#include "sd_logger_sm.h"
/*
**************************************************************************
*							LOCAL DEFINES
**************************************************************************
*/
/*
**************************************************************************
*							LOCAL CONSTANTS
**************************************************************************
*/
/*
**************************************************************************
*						    LOCAL DATA TYPES
**************************************************************************
*/
/*
**************************************************************************
*							  LOCAL TABLES
**************************************************************************
*/
/*
**************************************************************************
*								 MACRO'S
**************************************************************************
*/

/*
**************************************************************************
*						    GLOBAL VARIABLES
**************************************************************************
*/
  	  uint32_t	logger_u32 = 3000;
  	  FRESULT	fres;
  	  uint8_t	button_download_status = 0;
  	  uint8_t	time_write_to_sd_flag = 0;
/*
**************************************************************************
*                        LOCAL FUNCTION PROTOTYPES
**************************************************************************
*/
	
/*
**************************************************************************
*                           GLOBAL FUNCTIONS
**************************************************************************
*/

void SD_Logger_Init(void) {
	int soft_version_arr_int[3];
	soft_version_arr_int[0] = ((SOFT_VERSION) / 100)     ;
	soft_version_arr_int[1] = ((SOFT_VERSION) /  10) %10 ;
	soft_version_arr_int[2] = ((SOFT_VERSION)      ) %10 ;

	char DataChar[100];
	sprintf(DataChar,"\r\n SD-card logger 2020-march-22 v%d.%d.%d \r\n\tUART1 for debug on speed 115200/8-N-1\r\n\r\n",
			soft_version_arr_int[0], soft_version_arr_int[1], soft_version_arr_int[2]);
	HAL_UART_Transmit(&huart1, (uint8_t *)DataChar, strlen(DataChar), 100);


	FATFS_SPI_Init(&hspi1);	/* Initialize SD Card low level SPI driver */

	static uint8_t try_u8;
	do {
		fres = f_mount(&USERFatFS, "", 1);	/* try to mount SDCARD */
		if (fres == FR_OK) {
			sprintf(DataChar,"\r\nSDcard mount - Ok \r\n");
			HAL_UART_Transmit(&huart1, (uint8_t *)DataChar, strlen(DataChar), 100);
		}
		else {
			f_mount(NULL, "", 0);			/* Unmount SDCARD */
			Error_Handler();
			try_u8++;
			sprintf(DataChar,"%d)SDcard mount: Failed  Error: %d\r\n", try_u8, fres);
			HAL_UART_Transmit(&huart1, (uint8_t *)DataChar, strlen(DataChar), 100);
			HAL_Delay(1000);
		}
	} while ((fres !=0) && (try_u8 < 3));

	HAL_TIM_Base_Start(&htim4);
	HAL_TIM_Base_Start_IT(&htim4);
}
//************************************************************************

void SD_Logger_Main(void) {
	char DataChar[100];
	if (time_write_to_sd_flag == 1) {
		HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
		logger_u32++;
		sprintf(DataChar,"log# %04d; ", (int)logger_u32 );
		HAL_UART_Transmit(&huart1, (uint8_t *)DataChar, strlen(DataChar), 100);

		snprintf(DataChar, 7,"%04d\r\n", (int)logger_u32);
		fres = f_open(&USERFile, "sd_log.txt", FA_OPEN_ALWAYS | FA_WRITE);
		fres += f_lseek(&USERFile, f_size(&USERFile));

		if (fres == FR_OK) 	{
			f_printf(&USERFile, "%s", DataChar);	/* Write to file */
	//		_sd->file_size = f_size(&USERFile);
			f_close(&USERFile);	/* Close file */
			sprintf(DataChar,"write_SD - Ok;\r\n");
					HAL_UART_Transmit(&huart1, (uint8_t *)DataChar, strlen(DataChar), 100);
		} else {
			sprintf(DataChar,"write_SD - Error;\r\n");
			HAL_UART_Transmit(&huart1, (uint8_t *)DataChar, strlen(DataChar), 100);
		}
		time_write_to_sd_flag = 0;
	}

	if (button_download_status == 1) {
		HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, RESET);
		sprintf(DataChar,"Download data to port...\r\n");
		HAL_UART_Transmit(&huart1, (uint8_t *)DataChar, strlen(DataChar), 100);

		fres = f_open(&USERFile, "sd_log.txt", FA_OPEN_EXISTING | FA_READ);
		if (fres == FR_OK) {
			sprintf(DataChar,"FR - Ok;\r\n");
			HAL_UART_Transmit(&huart1, (uint8_t *)DataChar, strlen(DataChar), 100);
			char buff[200];
			while (f_gets(buff, 200, &USERFile)) {
				sprintf(DataChar,"%s\r",buff);
				HAL_UART_Transmit(&huart1, (uint8_t *)DataChar, strlen(DataChar), 100);
			}
			f_close(&USERFile);
		} else {
			sprintf(DataChar,"FR - Fail;");
			HAL_UART_Transmit(&huart1, (uint8_t *)DataChar, strlen(DataChar), 100);
		}

		sprintf(DataChar,"\r\n\t...download finish.\r\n");
		HAL_UART_Transmit(&huart1, (uint8_t *)DataChar, strlen(DataChar), 100);
		HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, SET);

		button_download_status = 0;
	}
}
//************************************************************************

void Set_button_download_status(uint8_t _new_button_status_u8) {
	button_download_status = _new_button_status_u8;
}
//************************************************************************

void Set_time_write_to_sd_flag(uint8_t _new_flag_u8) {
	time_write_to_sd_flag = _new_flag_u8;
}
//************************************************************************

/*
**************************************************************************
*                           LOCAL FUNCTIONS
**************************************************************************
*/

//************************************************************************
