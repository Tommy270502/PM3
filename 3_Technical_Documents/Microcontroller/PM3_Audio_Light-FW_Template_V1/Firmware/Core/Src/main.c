/** ***************************************************************************
 * @file
 * @brief Sets up the microcontroller, the clock system and the peripherals.
 *
 * Initialization is done for the system, the blue user button, the user LEDs,
 * and the LCD display with the touchscreen.
 * @n All the peripherals needed for measuring are initialized.
 * And also those for the DMXoutputs.
 * @n Then the code enters an infinite while-loop, where it checks for
 * user input or newly available data and refreshes the display accordingly.
 *
 * @author  Hanspeter Hochreutener, hhrt@zhaw.ch
 * @date	19.11.2024
 * @modified_by Patrick Rennhard, renn@zhaw.ch
 * @modified_date 25.08.2025
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "stm32f4xx.h"
#include "stm32f429i_discovery.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ts.h"
#include "core_cm4.h"
#include <stdio.h>

#include "main.h"
#include "pushbutton.h"
#include "menu.h"

#include "calc.h"
#include "display.h"
#include "DMX.h"
#include "audio_codec.h"

/******************************************************************************
 * Defines
 *****************************************************************************/
#define DISP_LOOP_M0 	4	// Prevent flicker, delay refreshing display menu 0
#define DISP_LOOP_M1 	4	// Prevent flicker, delay refreshing display menu 1
#define DISP_LOOP_M2 	10  // Time signal
#define DISP_LOOP_M3 	4
#define DISP_LOOP_M4 	4

// Define the maximum number of points for the time signal (Display 240 x 320 pixels)
#define MAX_TIME_SIGNAL_POINTS 240

// Define time_signal_points based on AUDIO_CHANNEL_SIZE, ensuring it's capped at MAX_TIME_SIGNAL_POINTS
#define TIME_SIGNAL_POINTS (AUDIO_CHANNEL_SIZE > MAX_TIME_SIGNAL_POINTS ? MAX_TIME_SIGNAL_POINTS : AUDIO_CHANNEL_SIZE)

/******************************************************************************
 * Variables
 *****************************************************************************/

static float32_t left_channel_samples[AUDIO_CHANNEL_SIZE];
static float32_t right_channel_samples[AUDIO_CHANNEL_SIZE];

static float32_t light_avgs[NUMBER_OF_COLORS] = { 0.10, 0.17, 0.5, 0.8 };
static float32_t light_peaks[NUMBER_OF_COLORS] = { 0.2, 0.23, 0.75, 0.9 };

static uint32_t disp_loop_count_m0; // Loop counter for refreshing display menu 0
static uint32_t disp_loop_count_m1; // Loop counter for refreshing display menu 1
static uint32_t disp_loop_count_m2;
static uint32_t disp_loop_count_m3;
static uint32_t disp_loop_count_m4;
static bool disp_refresh;			///< Display should be refreshed

static uint8_t efect_active = 0;

/******************************************************************************
 * Functions
 *****************************************************************************/
static void SystemClock_Config(void);	///< System Clock Configuration
static void gyro_disable(void);			///< Disable the onboard gyroscope
static void error_handling(HAL_StatusTypeDef error);

/** ***************************************************************************
 * @brief  Main function
 * @return not used because main ends in an infinite loop
 *
 * Initialization and infinite while loop
 *****************************************************************************/
int main(void) {
	HAL_StatusTypeDef ret_val;

	HAL_Init();							// Initialize the system

	SystemClock_Config();				// Configure system clocks

#ifdef FLIPPED_LCD
	BSP_LCD_Init_Flipped();				// Initialize the LCD for flipped orientation
#else
	BSP_LCD_Init();						// Initialize the LCD display
#endif
	BSP_LCD_LayerDefaultInit(LCD_FOREGROUND_LAYER, LCD_FRAME_BUFFER);
	BSP_LCD_SelectLayer(LCD_FOREGROUND_LAYER);
	BSP_LCD_DisplayOn();
	BSP_LCD_Clear(LCD_COLOR_WHITE);

	BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());	// Touchscreen
	/* Uncomment next line to enable touchscreen interrupt */
	//BSP_TS_ITConfig();					// Enable Touchscreen interrupt
	PB_init();							// Initialize the user pushbutton
	PB_enableIRQ();					// Enable interrupt on user pushbutton

	BSP_LED_Init(LED3);					// Toggles in while loop
	BSP_LED_Init(LED4);					// Is toggled by user button

	MENU_draw();						// Draw the menu
	MENU_hint();						// Show hint at startup

	gyro_disable();					// Disable gyro, use those analog inputs

	ret_val = codec_init(left_channel_samples, right_channel_samples,
	AUDIO_CHANNEL_SIZE);                  // Audio Codec init
	error_handling(ret_val);

	codec_reset();

	codec_start();

	DMX_init();                     // Init DMX interf. to LED party panel

	ret_val = calc_init();
	error_handling(ret_val);

	/* Infinite while loop */
	while (1) {							// Infinitely loop in main function

		/* Comment next line if touchscreen interrupt is enabled */
		MENU_check_transition();
		switch (MENU_get_transition()) {	// Handle user menu transitions
		case MENU_NONE:	// No transition => do nothing
			break;
		case MENU_SCROLL_LEFT:	// Scroll menu left
			MENU_scroll_left();
			break;
		case MENU_SCROLL_RIGHT:	// Scroll menu right
			MENU_scroll_right();
			break;
		case MENU_ZERO:
		case MENU_ONE:
		case MENU_TWO:
		case MENU_THREE:
		case MENU_FOUR:
		case MENU_FIVE:
		case MENU_SIX:
		case MENU_SEVEN:
		case MENU_EIGHT:
		case MENU_NINE:
			disp_refresh = true;	// Switch to new menu item
			break;
		default:	// Should never occur
			break;
		}

		if (PB_pressed()) {				// Check if user pushbutton was pressed
			efect_active = !efect_active;
		}

		if (codec_data_ready()) {
			codec_clear_data_ready();
			BSP_LED_On(LED4);

			if (efect_active) {
				// Possibility of a variable effect, switchable on and off via a user button.
			}

			// ToDo Use the audio data in left_channel_samples and right_channel_samples for the different calculations.

			// ToDo Set new DMX values

			disp_refresh = true;      // Tell the display about the new data
		}

		DMX_transmit(); // Transmit (new) values to party panel. Regular execution is required; otherwise, the lamp will turn off.

		if (disp_refresh) {
			disp_refresh = false;

			switch (MENU_get_active()) {	// Show data for active user menu
			case MENU_NONE:	// Display help screen
				break;
			case MENU_ZERO:
				if (disp_loop_count_m0++ >= DISP_LOOP_M0) {
					disp_loop_count_m0 = 0;
					disp_clear_data();
					disp_level(-10, -5, -12, -6);
				}
				break;
			case MENU_ONE:
				if (disp_loop_count_m1++ >= DISP_LOOP_M1) {
					disp_loop_count_m1 = 0;
					disp_clear_data();
					disp_light_bars(light_avgs, light_peaks);
				}
				break;
			case MENU_TWO:
				if (disp_loop_count_m2++ >= DISP_LOOP_M2) {
					disp_loop_count_m2 = 0;
					disp_clear_data();
					disp_curves(left_channel_samples, TIME_SIGNAL_POINTS,
							-(1 << (CODEC_ADC_RES - 1)) / 2,
							(1 << (CODEC_ADC_RES - 1)) / 2,
							LCD_COLOR_RED);
					disp_curves(right_channel_samples, TIME_SIGNAL_POINTS,
							-(1 << (CODEC_ADC_RES - 1)) / 2,
							(1 << (CODEC_ADC_RES - 1)) / 2,
							LCD_COLOR_BLUE);
				}
				break;
			case MENU_THREE:
				if (disp_loop_count_m3++ >= DISP_LOOP_M3) {
					disp_loop_count_m3 = 0;
					disp_clear_data();
					//disp_curves(ToDo, ToDo, 0, 0.05, LCD_COLOR_RED);
				}
				break;
			case MENU_FOUR:
				if (disp_loop_count_m4++ >= DISP_LOOP_M4) {
					disp_loop_count_m4 = 0;
					disp_clear_data();
					//ToDo ....
				}
				break;
			case MENU_FIVE:
			case MENU_SIX:
			case MENU_SEVEN:
			case MENU_EIGHT:
			case MENU_NINE:
				// ToDo ....
				break;
			default:
				// Should never occur
				break;
			}
			BSP_LED_Off(LED4);

		}

	}
}

/** ***************************************************************************
 * @brief System Clock Configuration
 *
 *****************************************************************************/
static void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };
	/* Configure the main internal regulator output voltage */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
	/* Initialize High Speed External Oscillator and PLL circuits */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	HAL_RCC_OscConfig(&RCC_OscInitStruct);
	/* Initialize gates and clock dividers for CPU, AHB and APB busses */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
	HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
	/* Initialize PLL and clock divider for the LCD */
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
	PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
	PeriphClkInitStruct.PLLSAI.PLLSAIR = 4;
	PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_8;
	HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
	/* Set clock prescaler for ADCs */
	ADC->CCR |= ADC_CCR_ADCPRE_0;
}

/** ***************************************************************************
 * @brief Disable the GYRO on the microcontroller board.
 *
 * @note MISO of the GYRO is connected to PF8 and CS to PC1.
 * @n Some times the GYRO goes into an undefined mode at startup
 * and pulls the MISO low or high thus blocking the analog input on PF8.
 * @n The simplest solution is to pull the CS of the GYRO low for a short while
 * which is done with the code below.
 * @n PF8 is also reconfigured.
 * @n An other solution would be to remove the GYRO
 * from the microcontroller board by unsoldering it.
 *****************************************************************************/
static void gyro_disable(void) {
	__HAL_RCC_GPIOC_CLK_ENABLE();		// Enable Clock for GPIO port C
	/* Disable PC1 and PF8 first */
	GPIOC->MODER &= ~GPIO_MODER_MODER1_Msk;	// Reset mode for PC1
	GPIOC->MODER |= 1UL << GPIO_MODER_MODER1_Pos;	// Set PC1 as output
	GPIOC->BSRR |= GPIO_BSRR_BR1;		// Set GYRO (CS) to 0 for a short time
	HAL_Delay(10);						// Wait some time
	GPIOC->MODER |= 3UL << GPIO_MODER_MODER1_Pos;	// Analog PC1 = ADC123_IN11
	__HAL_RCC_GPIOF_CLK_ENABLE();		// Enable Clock for GPIO port F
	GPIOF->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED8_Msk;	// Reset speed of PF8
	GPIOF->AFR[1] &= ~GPIO_AFRH_AFSEL8_Msk;	// Reset alternate function of PF8
	GPIOF->PUPDR &= ~GPIO_PUPDR_PUPD8_Msk;	// Reset pulup/down of PF8
	HAL_Delay(10);						// Wait some time
	GPIOF->MODER |= 3UL << GPIO_MODER_MODER8_Pos; // Analog mode PF8 = ADC3_IN4
}

void error_handling(HAL_StatusTypeDef error) {
	if (error != HAL_OK) {
		while (1) {

		}
	}
}

// Write-function for debugging over console
int _write(int file, char *ptr, int len) {
	for (int i = 0; i < len; i++) {
		ITM_SendChar(*ptr++);
	}
	return len;
}

// Default function implementations required to prevent build errors.
__attribute__((weak)) void _close(void) {
}
__attribute__((weak)) void _lseek(void) {
}
__attribute__((weak)) void _read(void) {
}

__attribute__((weak)) void _fstat(void) {
}

__attribute__((weak)) void _isatty(void) {
}
/*__attribute__((weak)) void _write(void)
 {
 }*/

