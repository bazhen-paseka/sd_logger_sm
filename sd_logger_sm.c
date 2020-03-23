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
		FRESULT		fres;
		uint8_t		button_download_pressed = 0;
		uint8_t		time_count_update_flag = 0;
		uint32_t	second_count_u32 = 0;
		uint32_t 	counter_u32 = 0;

		RTC_TimeTypeDef TimeSt;
		RTC_DateTypeDef DateSt;

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
	soft_version_arr_int[0] = ((SOFT_VERSION) / 100) %10 ;
	soft_version_arr_int[1] = ((SOFT_VERSION) /  10) %10 ;
	soft_version_arr_int[2] = ((SOFT_VERSION)      ) %10 ;

	char DataChar[100];
	sprintf(DataChar,"\r\n\tSD-card logger 2020-march-22 v%d.%d.%d \r\n\tUART1 for debug on speed 115200/8-N-1\r\n\r\n",
			soft_version_arr_int[0], soft_version_arr_int[1], soft_version_arr_int[2]);
	HAL_UART_Transmit(&huart1, (uint8_t *)DataChar, strlen(DataChar), 100);

	I2Cdev_init(&hi2c1);
	I2C_ScanBusFlow(&hi2c1, &huart1);

#if (SET_RTC_TIM_AND_DATE == 1)
	Set_Date_and_Time_to_DS3231(0x20, 0x03, 0x23, 0x15, 0x10, 0x00);
#endif

	ds3231_GetTime(ADR_I2C_DS3231, &TimeSt);
	ds3231_GetDate(ADR_I2C_DS3231, &DateSt);
	ds3231_PrintTime(&TimeSt, &huart1);
	ds3231_PrintDate(&DateSt, &huart1);

	FATFS_SPI_Init(&hspi1);	/* Initialize SD Card low level SPI driver */

	HAL_IWDG_Refresh(&hiwdg);

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

	//ds3231_Alarm1_SetSeconds(ADR_I2C_DS3231, 0x00);
	ds3231_Alarm1_SetEverySeconds(ADR_I2C_DS3231);
	ds3231_Alarm1_ClearStatusBit(ADR_I2C_DS3231);

	//HAL_TIM_Base_Start(&htim4);
	//HAL_TIM_Base_Start_IT(&htim4);
	HAL_IWDG_Refresh(&hiwdg);
}
//************************************************************************

void SD_Logger_Main(void) {
	char DataChar[100];
	if (time_count_update_flag == 1) {
		HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
		HAL_IWDG_Refresh(&hiwdg);

		time_count_update_flag  = 0;
		ds3231_Alarm1_ClearStatusBit(ADR_I2C_DS3231);
		second_count_u32++;

		if (second_count_u32 >= SECOND) {
			second_count_u32  = 0;
			//HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, RESET);

			ds3231_GetTime(ADR_I2C_DS3231, &TimeSt);
			ds3231_GetDate(ADR_I2C_DS3231, &DateSt);

			//ds3231_PrintTime(&TimeSt, &huart1);
			//ds3231_PrintDate(&DateSt, &huart1);

			snprintf(DataChar, 30,"%04d\t%02d\t%02d\t%02d\t%02d\t%02d\t%04d\r\n",
					(int)(DateSt.Year + 2000), (int)DateSt.Month, (int)DateSt.Date,
					(int)TimeSt.Hours, (int)TimeSt.Minutes, (int)TimeSt.Seconds,
					(int)(100000*DateSt.Year + 10000*DateSt.Month + 10000*DateSt.Date + 10000*TimeSt.Hours + 100*TimeSt.Minutes + TimeSt.Seconds));
			HAL_UART_Transmit(&huart1, (uint8_t *)DataChar, strlen(DataChar), 100);

			fres = f_open(&USERFile, "sd_log.txt", FA_OPEN_ALWAYS | FA_WRITE);
			fres += f_lseek(&USERFile, f_size(&USERFile));

			if (fres == FR_OK) 	{
				f_printf(&USERFile, "%s", DataChar);	/* Write to file */
				uint32_t file_size_u32 = f_size(&USERFile);
				f_close(&USERFile);	/* Close file */
				sprintf(DataChar,"write_SD - Ok; file_size=%d; ", (int)file_size_u32);
				HAL_UART_Transmit(&huart1, (uint8_t *)DataChar, strlen(DataChar), 100);
			} else {
				sprintf(DataChar,"write_SD - Error; ");
				HAL_UART_Transmit(&huart1, (uint8_t *)DataChar, strlen(DataChar), 100);
			}
			//HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, SET);
		}
	}

	if (button_download_pressed == 1) {
		button_download_pressed =  0;
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
	}
}
//************************************************************************

void Set_button_download_pressed(uint8_t _new_button_status_u8) {
	button_download_pressed = _new_button_status_u8;
}
//************************************************************************

void Set_time_count_update_flag(uint8_t _new_flag_u8) {
	time_count_update_flag = _new_flag_u8;
}
//************************************************************************

/*
**************************************************************************
*                           LOCAL FUNCTIONS
**************************************************************************
*/

//************************************************************************
