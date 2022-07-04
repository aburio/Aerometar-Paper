// Copyright 2013 Pervasive Displays, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
// express or implied.  See the License for the specific language
// governing permissions and limitations under the License.
//
// Embedded Artists has modified Pervasive Display Inc's demo application
// to run on the 2.7 inch E-paper Display module (EA-LCD-009)
//
//
// Arthur Burnichon has modified Embedded Artists's demo application
// to run with ESP32 and simplify the EPD driver from Pervasive Displays Inc
// to only handle 2.7 inch e-ink screen
//
#ifndef EPD_H_
#define EPD_H_

#include <Arduino.h>
#include <SPI.h>

typedef enum
{					// Image pixel -> Display pixel
	EPD_compensate, // B -> W, W -> B (Current Image)
	EPD_white,		// B -> N, W -> W (Current Image)
	EPD_inverse,	// B -> N, W -> B (New Image)
	EPD_normal		// B -> B, W -> W (New Image)
} stage;

typedef void reader(void *buffer, uint32_t address, uint16_t length);

class EPD
{
private:
	uint8_t panel_on_pin;
	uint8_t border_pin;
	uint8_t discharge_pin;
	uint8_t reset_pin;
	uint8_t busy_pin;
	uint8_t cs_pin;
	SPIClass &SPI;

	uint16_t stage_time;
	uint16_t factored_stage_time;
	uint16_t lines_per_display;
	uint16_t dots_per_line;
	uint16_t bytes_per_line;
	uint16_t bytes_per_scan;
	const uint8_t *gate_source;
	uint16_t gate_source_length;
	const uint8_t *channel_select;
	uint16_t channel_select_length;
	uint8_t *buffer;

	bool filler;

	// turn on/off display driver
	void power_on_cog();
	void power_off_cog();

	// single frame refresh
	void frame_fixed(uint8_t fixed_value, stage stage);
	void frame_data(const uint8_t *new_image, stage stage);
	void frame_cb(uint32_t address, reader *reader, stage stage);

	// stage_time frame refresh
	void frame_fixed_repeat(uint8_t fixed_value, stage stage);
	void frame_data_repeat(const uint8_t *new_image, stage stage);
	void frame_cb_repeat(uint32_t address, reader *reader, stage stage);

	// convert temperature to compensation factor
	uint8_t temperature_to_factor_10x(int16_t temperature);

	// single line display - very low-level
	void line(uint16_t line, const uint8_t *data, uint8_t fixed_value, stage stage);

public:
	// Constructor
	EPD(uint16_t width,
		uint16_t height,
		uint8_t panel_on_pin,
		uint8_t border_pin,
		uint8_t discharge_pin,
		uint8_t reset_pin,
		uint8_t busy_pin,
		uint8_t chip_select_pin,
		SPIClass &SPI_driver);

	~EPD(void);
	
	// initialize display pinout
	void begin();

	// set the display driver settings according to room temperature
	void setFactor(int16_t temperature = 25);

	// clear display (anything -> white)
	void clear();

	// update the screen content with the new image
	void update(const uint8_t *image);
};

#endif