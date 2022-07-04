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

#include <Arduino.h>
#include <limits.h>

#include <SPI.h>

#include "EPD.h"

// delays - more consistent naming
#define Delay_ms(ms) delay(ms)
#define Delay_us(us) delayMicroseconds(us)

// inline arrays
#define ARRAY(type, ...) ((type[]){__VA_ARGS__})
#define CU8(...) (ARRAY(const uint8_t, __VA_ARGS__))

static void SPI_put(uint8_t c);
static void SPI_put_wait(uint8_t c, int busy_pin);
static void SPI_send(uint8_t cs_pin, const uint8_t *buffer, uint16_t length);

EPD::EPD(uint16_t width,
		 uint16_t height,
		 uint8_t panel_on_pin,
		 uint8_t border_pin,
		 uint8_t discharge_pin,
		 uint8_t reset_pin,
		 uint8_t busy_pin,
		 uint8_t chip_select_pin,
		 SPIClass &SPI_driver) : panel_on_pin(panel_on_pin),
								 border_pin(border_pin),
								 discharge_pin(discharge_pin),
								 reset_pin(reset_pin),
								 busy_pin(busy_pin),
								 cs_pin(chip_select_pin),
								 SPI(SPI_driver)
{
	this->stage_time = 630; // milliseconds
	this->lines_per_display = height;
	this->dots_per_line = width;
	this->bytes_per_line = width / 8;
	this->bytes_per_scan = height / 4;
	this->filler = true;
	static uint8_t cs[] = {0x72, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xfe, 0x00, 0x00};
	static uint8_t gs[] = {0x72, 0x00};
	this->channel_select = cs;
	this->channel_select_length = sizeof(cs);
	this->gate_source = gs;
	this->gate_source_length = sizeof(gs);
	this->factored_stage_time = this->stage_time;

	uint16_t bytes = ((width + 7) / 8) * height;
	if ((buffer = (uint8_t *)malloc(bytes)))
	{
		memset(buffer, 0, bytes);
	}
}

EPD::~EPD(void)
{
	if (buffer)
	{
		free(buffer);
	}
}

void EPD::begin()
{
	pinMode(this->busy_pin, INPUT);
	pinMode(this->reset_pin, OUTPUT);
	pinMode(this->panel_on_pin, OUTPUT);
	pinMode(this->discharge_pin, OUTPUT);
	pinMode(this->border_pin, OUTPUT);
	pinMode(this->cs_pin, OUTPUT);

	digitalWrite(this->reset_pin, LOW);
	digitalWrite(this->panel_on_pin, LOW);
	digitalWrite(this->discharge_pin, LOW);
	digitalWrite(this->border_pin, LOW);
	digitalWrite(this->cs_pin, LOW);

	SPI.begin();
	SPI.setBitOrder(MSBFIRST);
	SPI.setDataMode(SPI_MODE0);
	SPI.setClockDivider(SPI_CLOCK_DIV4);
}

void EPD::setFactor(int16_t temperature)
{
	this->factored_stage_time = this->stage_time * this->temperature_to_factor_10x(temperature) / 10;
}

void EPD::clear()
{
	// clean buffer
	if (buffer)
	{
		uint16_t bytes = ((dots_per_line + 7) / 8) * lines_per_display;
		memset(buffer, 0, bytes);
	}

	// clean display
	this->power_on_cog();
	this->frame_fixed_repeat(0xff, EPD_compensate);
	this->frame_fixed_repeat(0xff, EPD_white);
	this->frame_fixed_repeat(0xaa, EPD_inverse);
	this->frame_fixed_repeat(0xaa, EPD_normal);
	this->power_off_cog();
}

void EPD::update(const uint8_t *image)
{
	uint16_t bytes = ((dots_per_line + 7) / 8) * lines_per_display;

	this->power_on_cog();
	this->frame_data_repeat(buffer, EPD_compensate);
	this->frame_data_repeat(buffer, EPD_white);
	this->frame_data_repeat(image, EPD_inverse);
	this->frame_data_repeat(image, EPD_normal);
	this->power_off_cog();

	memcpy(buffer, image, bytes);
}

// Private functions
void EPD::power_on_cog()
{
	SPI_put(0x00);

	// initial state
	digitalWrite(this->reset_pin, LOW);
	digitalWrite(this->panel_on_pin, LOW);
	digitalWrite(this->discharge_pin, LOW);
	digitalWrite(this->border_pin, LOW);
	digitalWrite(this->cs_pin, LOW);

	// power up sequence
	digitalWrite(this->panel_on_pin, HIGH);
	digitalWrite(this->cs_pin, HIGH);
	digitalWrite(this->border_pin, HIGH);
	Delay_ms(5);

	digitalWrite(this->reset_pin, LOW);
	Delay_ms(5);

	digitalWrite(this->reset_pin, HIGH);

	// wait for COG to become ready
	while (HIGH == digitalRead(this->busy_pin))
	{
		yield();
	}

	// channel select
	SPI_send(this->cs_pin, CU8(0x70, 0x01), 2);
	SPI_send(this->cs_pin, this->channel_select, this->channel_select_length);

	// DC/DC frequency
	SPI_send(this->cs_pin, CU8(0x70, 0x06), 2);
	SPI_send(this->cs_pin, CU8(0x72, 0xff), 2);

	// high power mode osc
	SPI_send(this->cs_pin, CU8(0x70, 0x07), 2);
	SPI_send(this->cs_pin, CU8(0x72, 0x9d), 2);

	// disable ADC
	SPI_send(this->cs_pin, CU8(0x70, 0x08), 2);
	SPI_send(this->cs_pin, CU8(0x72, 0x00), 2);

	// Vcom level

	SPI_send(this->cs_pin, CU8(0x70, 0x09), 2);
	SPI_send(this->cs_pin, CU8(0x72, 0xd0, 0x00), 3);

	// gate and source voltage levels
	SPI_send(this->cs_pin, CU8(0x70, 0x04), 2);
	SPI_send(this->cs_pin, this->gate_source, this->gate_source_length);

	Delay_ms(5); //???

	// driver latch on
	SPI_send(this->cs_pin, CU8(0x70, 0x03), 2);
	SPI_send(this->cs_pin, CU8(0x72, 0x01), 2);

	// driver latch off
	SPI_send(this->cs_pin, CU8(0x70, 0x03), 2);
	SPI_send(this->cs_pin, CU8(0x72, 0x00), 2);

	Delay_ms(5);

	// charge pump positive voltage on
	SPI_send(this->cs_pin, CU8(0x70, 0x05), 2);
	SPI_send(this->cs_pin, CU8(0x72, 0x01), 2);

	// charge pump negative voltage on
	SPI_send(this->cs_pin, CU8(0x70, 0x05), 2);
	SPI_send(this->cs_pin, CU8(0x72, 0x03), 2);

	Delay_ms(30);

	// Vcom driver on
	SPI_send(this->cs_pin, CU8(0x70, 0x05), 2);
	SPI_send(this->cs_pin, CU8(0x72, 0x0f), 2);

	Delay_ms(30);

	// output enable to disable
	SPI_send(this->cs_pin, CU8(0x70, 0x02), 2);
	SPI_send(this->cs_pin, CU8(0x72, 0x24), 2);
}

void EPD::power_off_cog()
{

	this->frame_fixed(0x55, EPD_normal);	  // dummy frame
	this->line(0x7fffu, 0, 0x55, EPD_normal); // dummy_line

	Delay_ms(25);

	digitalWrite(this->border_pin, LOW);
	Delay_ms(30);

	digitalWrite(this->border_pin, HIGH);

	// latch reset turn on
	SPI_send(this->cs_pin, CU8(0x70, 0x03), 2);
	SPI_send(this->cs_pin, CU8(0x72, 0x01), 2);

	// output enable off
	SPI_send(this->cs_pin, CU8(0x70, 0x02), 2);
	SPI_send(this->cs_pin, CU8(0x72, 0x05), 2);

	// Vcom power off
	SPI_send(this->cs_pin, CU8(0x70, 0x05), 2);
	SPI_send(this->cs_pin, CU8(0x72, 0x0e), 2);

	// power off negative charge pump
	SPI_send(this->cs_pin, CU8(0x70, 0x05), 2);
	SPI_send(this->cs_pin, CU8(0x72, 0x02), 2);

	// discharge
	SPI_send(this->cs_pin, CU8(0x70, 0x04), 2);
	SPI_send(this->cs_pin, CU8(0x72, 0x0c), 2);

	Delay_ms(120);

	// all charge pumps off
	SPI_send(this->cs_pin, CU8(0x70, 0x05), 2);
	SPI_send(this->cs_pin, CU8(0x72, 0x00), 2);

	// turn of osc
	SPI_send(this->cs_pin, CU8(0x70, 0x07), 2);
	SPI_send(this->cs_pin, CU8(0x72, 0x0d), 2);

	// discharge internal - 1
	SPI_send(this->cs_pin, CU8(0x70, 0x04), 2);
	SPI_send(this->cs_pin, CU8(0x72, 0x50), 2);

	Delay_ms(40);

	// discharge internal - 2
	SPI_send(this->cs_pin, CU8(0x70, 0x04), 2);
	SPI_send(this->cs_pin, CU8(0x72, 0xA0), 2);

	Delay_ms(40);

	// discharge internal - 3
	SPI_send(this->cs_pin, CU8(0x70, 0x04), 2);
	SPI_send(this->cs_pin, CU8(0x72, 0x00), 2);

	// turn of power and all signals
	digitalWrite(this->reset_pin, LOW);
	digitalWrite(this->panel_on_pin, LOW);
	digitalWrite(this->border_pin, LOW);
	digitalWrite(this->cs_pin, LOW);

	digitalWrite(this->discharge_pin, HIGH);

	SPI_put(0x00);

	Delay_ms(150);

	digitalWrite(this->discharge_pin, LOW);
}

uint8_t EPD::temperature_to_factor_10x(int16_t temperature)
{
	if (temperature <= -10)
	{
		return 170;
	}
	else if (temperature <= -5)
	{
		return 120;
	}
	else if (temperature <= 5)
	{
		return 80;
	}
	else if (temperature <= 10)
	{
		return 40;
	}
	else if (temperature <= 15)
	{
		return 30;
	}
	else if (temperature <= 20)
	{
		return 20;
	}
	else if (temperature <= 40)
	{
		return 10;
	}
	return 7;
}

void EPD::frame_fixed(uint8_t fixed_value, stage stage)
{
	for (uint8_t line = 0; line < this->lines_per_display; ++line)
	{
		this->line(line, 0, fixed_value, stage);
	}
}

void EPD::frame_data(const uint8_t *image, stage stage)
{
	for (uint8_t line = 0; line < this->lines_per_display; ++line)
	{
		this->line(line, &image[line * this->bytes_per_line], 0, stage);
	}
}

void EPD::frame_cb(uint32_t address, reader *reader, stage stage)
{
	static uint8_t buffer[264 / 8];
	for (uint8_t line = 0; line < this->lines_per_display; ++line)
	{
		reader(buffer, address + line * this->bytes_per_line, this->bytes_per_line);
		this->line(line, buffer, 0, stage);
	}
}

void EPD::frame_fixed_repeat(uint8_t fixed_value, stage stage)
{
	long stage_time = this->factored_stage_time;
	do
	{
		unsigned long t_start = millis();
		this->frame_fixed(fixed_value, stage);
		unsigned long t_end = millis();
		if (t_end > t_start)
		{
			stage_time -= t_end - t_start;
		}
		else
		{
			stage_time -= t_start - t_end + 1 + ULONG_MAX;
		}
	} while (stage_time > 0);
}

void EPD::frame_data_repeat(const uint8_t *image, stage stage)
{
	long stage_time = this->factored_stage_time;
	do
	{
		unsigned long t_start = millis();
		this->frame_data(image, stage);
		unsigned long t_end = millis();
		if (t_end > t_start)
		{
			stage_time -= t_end - t_start;
		}
		else
		{
			stage_time -= t_start - t_end + 1 + ULONG_MAX;
		}
	} while (stage_time > 0);
}

void EPD::frame_cb_repeat(uint32_t address, reader *reader, stage stage)
{
	long stage_time = this->factored_stage_time;
	do
	{
		unsigned long t_start = millis();
		this->frame_cb(address, reader, stage);
		unsigned long t_end = millis();
		if (t_end > t_start)
		{
			stage_time -= t_end - t_start;
		}
		else
		{
			stage_time -= t_start - t_end + 1 + ULONG_MAX;
		}
	} while (stage_time > 0);
}

void EPD::line(uint16_t line, const uint8_t *data, uint8_t fixed_value, stage stage)
{
	// charge pump voltage levels
	SPI_send(this->cs_pin, CU8(0x70, 0x04), 2);
	SPI_send(this->cs_pin, this->gate_source, this->gate_source_length);

	// send data
	SPI_send(this->cs_pin, CU8(0x70, 0x0a), 2);

	// CS low
	digitalWrite(this->cs_pin, LOW);
	SPI_put_wait(0x72, this->busy_pin);

	// even pixels
	for (uint16_t b = this->bytes_per_line; b > 0; --b)
	{
		if (0 != data)
		{
			uint8_t pixels;
			pixels = data[b - 1] & 0x55;

			switch (stage)
			{
			case EPD_compensate: // B -> W, W -> B (Current Image)
				pixels = 0xaa | (pixels ^ 0x55);
				break;
			case EPD_white: // B -> N, W -> W (Current Image)
				pixels = 0x55 + (pixels ^ 0x55);
				break;
			case EPD_inverse: // B -> N, W -> B (New Image)
				pixels = 0x55 | ((pixels ^ 0x55) << 1);
				break;
			case EPD_normal: // B -> B, W -> W (New Image)
				pixels = 0xaa | pixels;
				break;
			}
			uint8_t p1 = (pixels >> 0) & 0x03;
			uint8_t p2 = (pixels >> 2) & 0x03;
			uint8_t p3 = (pixels >> 4) & 0x03;
			uint8_t p4 = (pixels >> 6) & 0x03;
			pixels = (p1 << 6) | (p2 << 4) | (p3 << 2) | (p4 << 0);
			SPI_put_wait(pixels, this->busy_pin);
		}
		else
		{
			SPI_put_wait(fixed_value, this->busy_pin);
		}
	}

	// scan line
	for (uint16_t b = 0; b < this->bytes_per_scan; ++b)
	{
		if (line / 4 == b)
		{
			SPI_put_wait(0xc0 >> (2 * (line & 0x03)), this->busy_pin);
		}
		else
		{
			SPI_put_wait(0x00, this->busy_pin);
		}
	}

	// odd pixels
	for (uint16_t b = 0; b < this->bytes_per_line; ++b)
	{
		if (0 != data)
		{
			uint8_t pixels;
			pixels = data[b] & 0xaa;

			switch (stage)
			{
			case EPD_compensate: // B -> W, W -> B (Current Image)
				pixels = 0xaa | ((pixels ^ 0xaa) >> 1);
				break;
			case EPD_white: // B -> N, W -> W (Current Image)
				pixels = 0x55 + ((pixels ^ 0xaa) >> 1);
				break;
			case EPD_inverse: // B -> N, W -> B (New Image)
				pixels = 0x55 | (pixels ^ 0xaa);
				break;
			case EPD_normal: // B -> B, W -> W (New Image)
				pixels = 0xaa | (pixels >> 1);
				break;
			}
			SPI_put_wait(pixels, this->busy_pin);
		}
		else
		{
			SPI_put_wait(fixed_value, this->busy_pin);
		}
	}

	if (this->filler)
	{
		SPI_put_wait(0x00, this->busy_pin);
	}

	// CS high
	digitalWrite(this->cs_pin, HIGH);

	// output data to panel
	SPI_send(this->cs_pin, CU8(0x70, 0x02), 2);
	SPI_send(this->cs_pin, CU8(0x72, 0x2f), 2);
}

// Internal functions
static void SPI_put(uint8_t c)
{
	SPI.transfer(c);
}

static void SPI_put_wait(uint8_t c, int busy_pin)
{

	SPI_put(c);

	// wait for COG ready
	while (HIGH == digitalRead(busy_pin))
	{
	}
}

static void SPI_send(uint8_t cs_pin, const uint8_t *buffer, uint16_t length)
{
	// CS low
	digitalWrite(cs_pin, LOW);

	// send all data
	for (uint16_t i = 0; i < length; ++i)
	{
		SPI_put(*buffer++);
	}

	// CS high
	digitalWrite(cs_pin, HIGH);
	Delay_us(10);
}