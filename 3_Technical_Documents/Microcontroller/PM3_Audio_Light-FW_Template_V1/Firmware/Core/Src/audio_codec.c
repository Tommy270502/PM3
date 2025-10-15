/**
 * @file    audio_codec.c
 * @brief   API for audio codec initialization, SAI/DMA handling, and audio buffer management.
 * @author  Patrick Rennhard (renn@zhaw.ch)
 * @date    2025-09-03
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "audio_codec.h"
#include "calc.h"

#include <stdio.h>

#include "stm32f4xx.h"

#include "stm32f429i_discovery.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ts.h"

/******************************************************************************
 * Defines
 *****************************************************************************/

/******************************************************************************
 * Variables
 *****************************************************************************/

static int32_t audio_in_buffer_ping[AUDIO_FRAME_SIZE];
static int32_t audio_in_buffer_pong[AUDIO_FRAME_SIZE];

static int32_t audio_out_buffer_ping[AUDIO_FRAME_SIZE] = TEST_AUDIO_FRAME_2048;
static int32_t audio_out_buffer_pong[AUDIO_FRAME_SIZE] = TEST_AUDIO_FRAME_2048;
static int32_t *next_audio_out_buffer_pointer = audio_out_buffer_ping;

static float32_t *left_channel_buffer_pointer = 0;
static float32_t *right_channel_buffer_pointer = 0;

static uint8_t audio_codec_data_ready = 0;

/******************************************************************************
 * Functions
 *****************************************************************************/

/**
 * @brief Initializes the SAI interface DMA for audio streaming.
 *
 * This static helper function configures the DMA channels for both
 * transmission and reception in ping-pong (double-buffer) mode and
 * enables the necessary interrupts. It is used internally by the codec
 * driver and should not be called directly from application code.
 *
 * @note This function is called by `codec_start()` after the codec
 *       and GPIOs have been initialized.
 */
static void sai_dma_init(void);

/**
 * @brief Splits and converts interleaved I²S data into separate float buffers.
 *
 * This static helper function takes an interleaved I²S input buffer
 * containing left and right channel samples in 32-bit format (24-bit
 * data left aligned), splits the channels, and stores them into
 * separate floating-point output buffers.
 *
 * Each 32-bit I²S sample is shifted right by 8 bits to align the 24-bit
 * audio data, and then cast to `float32_t` for further processing.
 *
 * @param i2s_buffer Pointer to the interleaved I²S input buffer (32-bit samples).
 * @param left_out Pointer to the output buffer for left channel samples.
 * @param right_out Pointer to the output buffer for right channel samples.
 * @param out_buffer_size size of each output buffer.
 */
static void split_and_cast_i2s_buffer(int32_t *i2s_buffer, float32_t *left_out,
		float32_t *right_out, uint32_t out_buffer_size);

/**
 * @brief DMA interrupt handler for SAI/I²S receive events.
 *
 * This ISR handles DMA transfer complete events on DMA2 Stream1, which
 * is used for receiving audio data via the SAI/I²S interface. It clears
 * the transfer complete flag, determines which buffer (ping or pong) has
 * been filled, and copies the received samples into the corresponding
 * output buffer.
 *
 * After copying, the function splits the interleaved I²S data into
 * separate left and right floating-point channel buffers using
 * `split_and_cast_i2s_buffer()`. It also updates the pointer to the next
 * audio output buffer and sets the `audio_codec_data_ready` flag, which
 * signals that new audio data is available for processing.
 *
 * @note This function should not be called directly. It is automatically
 *       invoked by the NVIC when the DMA transfer complete interrupt
 *       for Stream1 occurs.
 */
void DMA2_Stream1_IRQHandler(void);

/**
 * @brief DMA interrupt handler for SAI/I²S transmit events.
 *
 * This ISR handles DMA transfer complete events on DMA2 Stream5, which
 * is used for transmitting audio data via the SAI/I²S interface. It
 * clears the transfer complete flag to acknowledge the interrupt.
 *
 * @note Unlike the receive handler, this function does not process data
 *       buffers, but only clears the interrupt flag. Buffer management
 *       for transmission is handled elsewhere in the codec driver.
 */
void DMA2_Stream5_IRQHandler(void);

void codec_reset(void) {
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
	HAL_Delay(1000);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
}

HAL_StatusTypeDef codec_init(float32_t *left_channel_buffer,
		float32_t *right_channel_buffer, uint32_t size) {
	HAL_StatusTypeDef ret_val;
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	if (size >= AUDIO_FRAME_SIZE / 2) {
		left_channel_buffer_pointer = left_channel_buffer;
		right_channel_buffer_pointer = right_channel_buffer;

		__HAL_RCC_GPIOA_CLK_ENABLE();       // Enable Clock for GPIO port B
		__HAL_RCC_GPIOB_CLK_ENABLE();       // Enable Clock for GPIO port B
		__HAL_RCC_GPIOC_CLK_ENABLE();       // Enable Clock for GPIO port C
		__HAL_RCC_GPIOE_CLK_ENABLE();       // Enable Clock for GPIO port E

		// Configure input pins ---------------------
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;   // Set as input
		GPIO_InitStruct.Pull = GPIO_NOPULL;       // No pull-up or pull-down
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

		// --- GPIOE: PE2 as input ---
		GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_6;
		HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

		// Configure output pins ---------------------
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;   // Push-Pull Output
		GPIO_InitStruct.Pull = GPIO_NOPULL;       // No pull-up or pull-down
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;   // Low frequency

		// --- GPIOA: PA8 as Output, initial HighGPIO_PIN_8 ---
		GPIO_InitStruct.Pin = GPIO_PIN_8;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);

		// --- GPIOA: PB4 as Output, initial Low ---
		GPIO_InitStruct.Pin = GPIO_PIN_4;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);

		// --- GPIOA: PC9 as Output, initial Low ---
		GPIO_InitStruct.Pin = GPIO_PIN_9;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);

		GPIO_InitTypeDef GPIO_InitStructSAI;

		/* --- PE3 (SDIN RX), PE4 (LRCK), PE5 (SCK) --- */
		GPIO_InitStructSAI.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
		GPIO_InitStructSAI.Mode = GPIO_MODE_AF_PP; // Alternate Function Push-Pull
		GPIO_InitStructSAI.Pull = GPIO_NOPULL;
		GPIO_InitStructSAI.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStructSAI.Alternate = GPIO_AF6_SAI1;
		HAL_GPIO_Init(GPIOE, &GPIO_InitStructSAI);

		/* --- PE6 (SDOUT TX) --- */
		GPIO_InitStructSAI.Pin = GPIO_PIN_6;
		HAL_GPIO_Init(GPIOE, &GPIO_InitStructSAI);

		ret_val = HAL_OK;
	} else {
		ret_val = HAL_ERROR;
	}

	return ret_val;

}

void codec_start(void) {
// Enable peripheral clock for SAI1
	RCC->APB2ENR |= RCC_APB2ENR_SAI1EN;

// Disable Block A before configuration
	SAI1_Block_A->CR1 &= ~SAI_xCR1_SAIEN;

	/* --- CR1 --- */
	SAI1_Block_A->CR1 = (3U << SAI_xCR1_MODE_Pos) | // Mode = Slave Receiver
			(7U << SAI_xCR1_DS_Pos) | // Data size = 32 bits
			(0U << SAI_xCR1_PRTCFG_Pos) | // Free protocol
			(0U << SAI_xCR1_LSBFIRST_Pos) | // MSB first
			(1U << SAI_xCR1_CKSTR_Pos) | // Clock strobing on xx edge
			(0U << SAI_xCR1_SYNCEN_Pos) | // Asynchronous (independent)
			(0U << SAI_xCR1_OUTDRIV_Pos) | // Output drive disabled (receiver)
			(0U << SAI_xCR1_MONO_Pos) | // Stereo mode
			(0U << SAI_xCR1_DMAEN_Pos) | // DMA disabled (enable later)
			(0U << SAI_xCR1_NODIV_Pos) | // MCLK divider enabled
			(0U << SAI_xCR1_MCKDIV_Pos); // MCLK divide by 1 (ignored in slave)

	/* --- CR2 --- */
	SAI1_Block_A->CR2 = (1U << SAI_xCR2_FTH_Pos) | // FIFO threshold = 1/4
			(0U << SAI_xCR2_TRIS_Pos) | // Tristate not used
			(0U << SAI_xCR2_COMP_Pos);     // No companding

	/* --- FRCR --- */
	SAI1_Block_A->FRCR = (63U << SAI_xFRCR_FRL_Pos) | // Frame length = 64 bits (2 slots × 32)
			(31U << SAI_xFRCR_FSALL_Pos) | // Active frame length = 32 bits
			(0U << SAI_xFRCR_FSDEF_Pos) | // FS asserted at start of frame
			(0U << SAI_xFRCR_FSPOL_Pos) | // FS active low
			(1U << SAI_xFRCR_FSOFF_Pos); // FS asserted one bit before first data bit

	/* --- SLOTR --- */
	SAI1_Block_A->SLOTR = (0x3 << SAI_xSLOTR_SLOTEN_Pos) | // Enable slots 0 and 1 (stereo)
			(1U << SAI_xSLOTR_NBSLOT_Pos) | // Number of slots = 2
			(2U << SAI_xSLOTR_SLOTSZ_Pos) | // Slot size = 32 bits
			(0U << SAI_xSLOTR_FBOFF_Pos);   // First bit offset = 0

// Disable Block B before configuration
	SAI1_Block_B->CR1 &= ~SAI_xCR1_SAIEN;

	/* --- CR1 --- */
	SAI1_Block_B->CR1 = (2U << SAI_xCR1_MODE_Pos) | // Mode = Slave Transmitter
			(7U << SAI_xCR1_DS_Pos) | // Data size = 32 bits
			(0U << SAI_xCR1_PRTCFG_Pos) | // Free protocol
			(0U << SAI_xCR1_LSBFIRST_Pos) | // MSB first
			(1U << SAI_xCR1_CKSTR_Pos) | // Clock strobing on xx edge
			(1U << SAI_xCR1_SYNCEN_Pos) | // Synchronous with Block A
			(1U << SAI_xCR1_OUTDRIV_Pos) | // Output drive enabled
			(0U << SAI_xCR1_MONO_Pos) | // Stereo mode
			(0U << SAI_xCR1_DMAEN_Pos) | // DMA disabled (enable later)
			(0U << SAI_xCR1_NODIV_Pos) | // MCLK divider enabled
			(0U << SAI_xCR1_MCKDIV_Pos); // MCLK divide by 1 (ignored in slave)

	/* --- CR2 --- */
	SAI1_Block_B->CR2 = (1U << SAI_xCR2_FTH_Pos) | // // FIFO threshold = 1/4
			(0U << SAI_xCR2_TRIS_Pos) | // Tristate not used
			(0U << SAI_xCR2_COMP_Pos);     // No companding

	/* --- FRCR --- */
	SAI1_Block_B->FRCR = (63U << SAI_xFRCR_FRL_Pos) | // Frame length = 64 bits
			(31U << SAI_xFRCR_FSALL_Pos) | // Active frame length = 32 bits
			(0U << SAI_xFRCR_FSDEF_Pos) | // FS asserted at start of frame
			(0U << SAI_xFRCR_FSPOL_Pos) | // FS active low
			(1U << SAI_xFRCR_FSOFF_Pos); // FS asserted one bit before first data bit

	/* --- SLOTR --- */
	SAI1_Block_B->SLOTR = (0x3 << SAI_xSLOTR_SLOTEN_Pos) | // Enable slots 0 and 1
			(1U << SAI_xSLOTR_NBSLOT_Pos) | // Number of slots = 2
			(2U << SAI_xSLOTR_SLOTSZ_Pos) | // Slot size = 32 bits
			(0U << SAI_xSLOTR_FBOFF_Pos);   // First bit offset = 0

// DMA init
	sai_dma_init();

// enable sai dma
	SAI1_Block_A->CR1 |= SAI_xCR1_DMAEN; // enable RX DMA
	SAI1_Block_B->CR1 |= SAI_xCR1_DMAEN; // enable TX DMA

// Enable SAI blocks
	SAI1_Block_A->CR1 |= SAI_xCR1_SAIEN;
	SAI1_Block_B->CR1 |= SAI_xCR1_SAIEN;

}

uint8_t codec_data_ready(void) {
	return audio_codec_data_ready;
}

void codec_clear_data_ready(void) {
	audio_codec_data_ready = 0;
}

void codec_mirror_left_channel(void) {
	// Copy left channel samples to right channel in the output buffer
	// This creates a mono output (left channel duplicated to both L and R)
	
	// Determine which output buffer is currently being prepared for next DMA transfer
	int32_t *current_out_buffer = next_audio_out_buffer_pointer;
	
	// Iterate through the interleaved I²S buffer
	// Format: [L0, R0, L1, R1, L2, R2, ...]
	for (uint32_t i = 0; i < AUDIO_FRAME_SIZE; i += 2) {
		// Copy left channel sample (even index) to right channel (odd index)
		current_out_buffer[i + 1] = current_out_buffer[i];
	}
}

static void sai_dma_init(void) {

	__HAL_RCC_DMA2_CLK_ENABLE();

	DMA2_Stream1->CR &= ~DMA_SxCR_EN;   // Disable the DMA stream 1
	while (DMA2_Stream1->CR & DMA_SxCR_EN) {

	}   // Wait for DMA to finish

	// Clear transfer complete interrupt flag
	DMA2->HIFCR |= DMA_LISR_TCIF1;

	DMA2_Stream5->CR &= ~DMA_SxCR_EN;   // Disable the DMA stream 5
	while (DMA2_Stream5->CR & DMA_SxCR_EN) {

	}   // Wait for DMA to finish

	// Clear transfer complete interrupt flag
	DMA2->HIFCR |= DMA_HISR_TCIF5;

	// --- SAI1_Block_A RX (Stream1, Channel0) ---
	DMA2_Stream1->CR = 0;                       // Reset CR

	// Select channel 0
	DMA2_Stream1->CR |= (0UL << DMA_SxCR_CHSEL_Pos);

	// Set Priority high
	DMA2_Stream1->CR |= DMA_SxCR_PL_1;

	// Memory data size = 32 bit
	DMA2_Stream1->CR |= DMA_SxCR_MSIZE_1;

	// Peripheral data size = 32 bit
	DMA2_Stream1->CR |= DMA_SxCR_PSIZE_1;

	// Increment memory address pointer
	DMA2_Stream1->CR |= DMA_SxCR_MINC;

	// Enable Circular Mode
	DMA2_Stream1->CR |= DMA_SxCR_CIRC;

	// Enable Double Buffer Mode
	DMA2_Stream1->CR |= DMA_SxCR_DBM;

	// Transfer complete interrupt enable
	DMA2_Stream1->CR |= DMA_SxCR_TCIE;

	// Number of data items to transfer
	DMA2_Stream1->NDTR = AUDIO_FRAME_SIZE;

	// Peripheral register address
	DMA2_Stream1->PAR = (uint32_t) &(SAI1_Block_A->DR); // Peripheral address

	// Ping Buffer memory address
	DMA2_Stream1->M0AR = (uint32_t) audio_in_buffer_ping;

	// Pong Buffer memory address
	DMA2_Stream1->M1AR = (uint32_t) audio_in_buffer_pong;

	// --- SAI1_Block_B TX (Stream5, Channel0) ---
	DMA2_Stream5->CR = 0;                       // Reset CR

	// Select channel 0
	DMA2_Stream5->CR |= (0UL << DMA_SxCR_CHSEL_Pos);

	// Set Priority high
	DMA2_Stream5->CR |= DMA_SxCR_PL_1;

	// Memory data size = 32 bit
	DMA2_Stream5->CR |= DMA_SxCR_MSIZE_1;

	// Peripheral data size = 32 bit
	DMA2_Stream5->CR |= DMA_SxCR_PSIZE_1;

	// Increment memory address pointer
	DMA2_Stream5->CR |= DMA_SxCR_MINC;

	// Enable Circular Mode
	DMA2_Stream5->CR |= DMA_SxCR_CIRC;

	// Enable Double Buffer Mode
	DMA2_Stream5->CR |= DMA_SxCR_DBM;

	// Transfer complete interrupt enable
	DMA2_Stream5->CR |= DMA_SxCR_TCIE;

	// Direction
	DMA2_Stream5->CR |= DMA_SxCR_DIR_0;

	// Number of data items to transfer
	DMA2_Stream5->NDTR = AUDIO_FRAME_SIZE;

	// Peripheral register address
	DMA2_Stream5->PAR = (uint32_t) &(SAI1_Block_B->DR); // Peripheral address

	// Ping Buffer memory address
	DMA2_Stream5->M0AR = (uint32_t) audio_out_buffer_ping;

	// Pong Buffer memory address
	DMA2_Stream5->M1AR = (uint32_t) audio_out_buffer_pong;

	DMA2_Stream1->CR |= DMA_SxCR_EN;   // Enable Stream
	DMA2_Stream5->CR |= DMA_SxCR_EN;   // Enable Stream

	// --- Optional: NVIC activate interrupts ---

	NVIC_ClearPendingIRQ(DMA2_Stream1_IRQn);
	NVIC_ClearPendingIRQ(DMA2_Stream5_IRQn);

	NVIC_EnableIRQ(DMA2_Stream1_IRQn); // RX complete/error
	//NVIC_EnableIRQ(DMA2_Stream5_IRQn); // TX complete/error
}

static void split_and_cast_i2s_buffer(int32_t *i2s_buffer, float32_t *left_out,
		float32_t *right_out, uint32_t out_buffer_size) {
	// I²S format: interleaved stereo data (left, right, left, right...)
	// Each 32-bit sample contains 24-bit audio data left-aligned
	// Need to shift right by 8 bits to get the actual 24-bit value
	
	for (uint32_t i = 0; i < out_buffer_size; i++) {
		// Left channel is at even indices (0, 2, 4, ...)
		// Shift right by 8 bits to align 24-bit data, then cast to float
		left_out[i] = (float32_t)(i2s_buffer[i * 2] >> 8);
		
		// Right channel is at odd indices (1, 3, 5, ...)
		// Shift right by 8 bits to align 24-bit data, then cast to float
		right_out[i] = (float32_t)(i2s_buffer[i * 2 + 1] >> 8);
	}
}

void codec_update_output_buffer(uint8_t channel, float32_t *data, uint32_t size) {
	// Update the audio output buffer for a specific channel
	// channel: 0 = left, 1 = right
	// data: pointer to float32 audio samples
	// size: number of samples to copy
	
	// Ensure size doesn't exceed half the frame size (one channel)
	if (size > AUDIO_CHANNEL_SIZE) {
		size = AUDIO_CHANNEL_SIZE;
	}
	
	// Determine which output buffer to update
	int32_t *current_out_buffer = next_audio_out_buffer_pointer;
	
	// Convert float32 samples to 32-bit I²S format and write to interleaved buffer
	for (uint32_t i = 0; i < size; i++) {
		// Convert float to int32 and shift left by 8 bits (24-bit data left-aligned)
		int32_t sample = ((int32_t)data[i]) << 8;
		
		if (channel == 0) {
			// Left channel: write to even indices (0, 2, 4, ...)
			current_out_buffer[i * 2] = sample;
		} else {
			// Right channel: write to odd indices (1, 3, 5, ...)
			current_out_buffer[i * 2 + 1] = sample;
		}
	}
}

void DMA2_Stream1_IRQHandler(void) {
	if (DMA2->LISR & DMA_LISR_TCIF1) {
		DMA2->LIFCR |= DMA_LIFCR_CTCIF1; // Clear Transfer Complete Flag

		if ((DMA2_Stream1->CR & DMA_SxCR_CT) == 0) {
			// data is ready in the "audio_in_buffer_pong"

			// Copy the data to the corresponding out_buffer for the audio loop
			for (uint32_t i = 0; i < AUDIO_FRAME_SIZE; i++) {
				next_audio_out_buffer_pointer[i] = audio_in_buffer_pong[i];
			}

			if ((left_channel_buffer_pointer != 0)
					&& (right_channel_buffer_pointer != 0)) {
				// Copy and cast the data to the right_channel_buffer_pointer and left_channel_buffer_pointer
				split_and_cast_i2s_buffer(audio_in_buffer_pong, 
					left_channel_buffer_pointer, 
					right_channel_buffer_pointer, 
					AUDIO_CHANNEL_SIZE);
			}
		} else {

			// data is ready in the "audio_in_buffer_ping"

			// Copy the data to the corresponding out_buffer for the audio loop
			for (uint32_t i = 0; i < AUDIO_FRAME_SIZE; i++) {
				next_audio_out_buffer_pointer[i] = audio_in_buffer_ping[i];
			}

			if ((left_channel_buffer_pointer != 0)
					&& (right_channel_buffer_pointer != 0)) {
				// Copy and cast the data to the right_channel_buffer_pointer and left_channel_buffer_pointer
				split_and_cast_i2s_buffer(audio_in_buffer_ping, 
					left_channel_buffer_pointer, 
					right_channel_buffer_pointer, 
					AUDIO_CHANNEL_SIZE);
			}
		}
		
		// Toggle output buffer pointer for next DMA cycle
		if (next_audio_out_buffer_pointer == audio_out_buffer_ping) {
			next_audio_out_buffer_pointer = audio_out_buffer_pong;
		} else {
			next_audio_out_buffer_pointer = audio_out_buffer_ping;
		}
		
		audio_codec_data_ready = 1;

	}

}

void DMA2_Stream5_IRQHandler(void) {
	if (DMA2->LISR & DMA_HISR_TCIF5) {
		DMA2->LIFCR |= DMA_HIFCR_CTCIF5; // Clear Transfer Complete Flag
	}
}

