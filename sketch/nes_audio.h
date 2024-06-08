
/*
https://github.com/connornishijima/Cartridge
  Cartridge.cpp - Library for parsing VGM into an NES APU emulation!
  Created by Connor Nishijima, October 14th 2018.
  Released under the GPLv3 license.
*/

#ifndef cartridge_h
#define cartridge_h

#include "Arduino.h"

class Cartridge
{
  public:
  Cartridge(uint8_t audio_pin);
	
	void init();
	void play_nes(const uint8_t* music, bool looping = false, float volume = 1.0);
	void frame_counter_cb(void (*action)());
	
  private:
	void parse_vgm(const uint8_t* music, bool looping);
	void read_vgm_header(const uint8_t* music);
	uint32_t get_32_bit(const uint8_t* music, uint32_t index);
	void reset_nes();
	void sample_audio();
	void render_audio(float gain);
	void clock_apu();
	void decrement_timers();
	void clock_lsfr();
	void clock_frame_counter();
	uint8_t frame_counter_mode();
	uint8_t get_bit(uint8_t reg, uint8_t b);
	void write_bit(uint8_t reg, uint8_t b, uint8_t val);
	uint8_t get_reg(uint8_t reg);
	void write_reg(uint8_t reg, uint8_t val);
	void clock_envelopes();
	void clock_linear_counter();
	void clock_length_counters();
	uint8_t get_duty(uint8_t reg);
	uint16_t get_11_bit_timer(uint8_t reg_low, uint8_t reg_high);
	void set_11_bit_timer(uint8_t reg_low, uint8_t reg_high, uint16_t val);
	uint8_t get_length_counter(uint8_t reg);
	uint8_t write_length_counter(uint8_t reg, uint8_t val);
	void clock_sweep_units();
	
	uint8_t NES_REG[24] = {
	  // PULSE 1
	  B00000000, // $4000
	  B00000000, // $4001
	  B00000000, // $4002
	  B00000000, // $4003

	  // PULSE 2
	  B00000000, // $4004
	  B00000000, // $4005
	  B00000000, // $4006
	  B00000000, // $4007

	  // TRIANGLE
	  B00000000, // $4008

	  B00000000, // $4009 (UNUSED)

	  B00000000, // $400A
	  B00000000, // $400B

	  // NOISE
	  B00000000, // $400C
	  B00000000, // $400E
	  B00000000, // $400F

	  // DMC
	  B00000000, // $4010
	  B00000000, // $4011
	  B00000000, // $4012
	  B00000000, // $4013

	  B00000000, // $4014 (UNUSED)

	  // Control/Status
	  B00000000, // $4015

	  B00000000, // $4016 (UNUSED)

	  // Frame Counter
	  B00000000, // $4017
	};

	const uint32_t NES_APU_FREQ =  1789773 / 2 / 2; // APU is half speed of NES CPU, and we are running half the resolution of that to stay light.
	const uint32_t cycle_period = F_CPU / NES_APU_FREQ;

	const uint16_t audio_rate = 44100;
	const uint32_t audio_period = F_CPU / audio_rate;
	uint32_t next_audio = 0;

	uint32_t next_cycle = 0;
	uint32_t cpu_cycles = 0;
	uint32_t apu_cycles = 0;

	uint32_t t_last = 0;
	uint32_t cycles_delta = 0;
	uint32_t cycles_so_far = 0;

	const uint8_t audio_divisor = 2;
	uint8_t audio_counter = 0;

	const uint8_t length_table[2][16] = {
	  {0x0A, 0x14, 0x28, 0x50, 0xA0, 0x3C, 0x0E, 0x1A, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x48, 0x10, 0x20},
	  {0xFE, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x12, 0x14, 0x16, 0x18, 0x1A, 0x1C, 0x1E}
	};

	const uint8_t duty_table[4][8] = {
	  {0, 0, 0, 0, 0, 0, 0, 1}, // 12.5%
	  {0, 0, 0, 0, 0, 0, 1, 1}, // 25%
	  {0, 0, 0, 0, 1, 1, 1, 1}, // 50%
	  {1, 1, 1, 1, 1, 1, 0, 0}, // 25% (inv.)
	};

	const uint16_t noise_table[16] = {
	  4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
	};

	const uint8_t tri_table[32] = {
	  15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
	  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
	};
  uint8_t single_output = 0;
	// Pulse 1 Variables
	uint8_t p1_output = 0;
	int16_t p1_11_bit_timer = 0;
	uint8_t p1_wave_index = 0;
	uint16_t p1_length_counter = 0;
	uint8_t p1_envelope_divider = 0;
	uint8_t p1_decay_counter = 0;
	uint8_t p1_volume = 0;
	uint8_t p1_channel = 0;
	uint8_t p1_pin = 12;

	// Pulse 2 Variables
	uint8_t p2_output = 0;
	int16_t p2_11_bit_timer = 0;
	uint8_t p2_wave_index = 0;
	uint16_t p2_length_counter = 0;
	uint8_t p2_envelope_divider = 0;
	uint8_t p2_decay_counter = 0;
	uint8_t p2_volume = 0;
	uint8_t p2_channel = 1;
	uint8_t p2_pin = 14;

	// Noise Variables
	uint8_t n_output = 0;
	int16_t n_timer = 0;
	uint16_t n_length_counter = 0;
	uint8_t n_envelope_divider = 0;
	uint8_t n_decay_counter = 0;
	uint8_t n_volume = 0;
	uint8_t n_xor = 0;
	uint16_t n_lsfr = 1;
	uint8_t n_channel = 2;
	uint8_t n_pin = 27;

	// Triangle Variables
	uint8_t t_output = 0;
	int16_t t_11_bit_timer = 0;
	uint8_t t_wave_index = 0;
	uint16_t t_length_counter = 0;
	uint16_t t_linear_counter = 0;
	uint8_t t_channel = 3;
	uint8_t t_pin = 26;

	uint32_t vgm_index = 0;
	uint16_t vgm_wait = 0;
	bool NES_PLAYING = false;

	uint32_t VGM_EOF_OFFSET = 0;
	uint32_t VGM_TOTAL_NUM_SAMPLES = 0;
	uint32_t VGM_RATE = 0;
	uint32_t VGM_DATA_OFFSET = 0;
	uint32_t VGM_NES_APU_CLOCK = 0;
	uint32_t VGM_LOOP_OFFSET = 0;
	
};

#endif
