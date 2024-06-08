
#include "nes_audio.h"

static void (*fc_callback)(); 

Cartridge::Cartridge(uint8_t audio_pin) {
 single_output = audio_pin;//only single 1 pin use
 
}

void Cartridge::frame_counter_cb(void (*action)()){
  fc_callback = action;
}

void Cartridge::init(){//call for 4 channel use only
  
  sigmaDeltaSetup(p1_pin, p1_channel, 88200);
  sigmaDeltaWrite(p1_channel, 0);

  sigmaDeltaSetup(p2_pin, p2_channel, 88200);
  sigmaDeltaWrite(p2_channel, 0);

  sigmaDeltaSetup(n_pin, n_channel, 88200);
  sigmaDeltaWrite(n_channel, 0);

  sigmaDeltaSetup(t_pin, t_channel, 88200);
  sigmaDeltaWrite(t_channel, 0);
  
}

void Cartridge::play_nes(const uint8_t* music, bool looping, float volume){
  NES_PLAYING = true;//variable to control playback
  read_vgm_header(music);

  next_cycle = ESP.getCycleCount();
  next_audio = ESP.getCycleCount();
  while (NES_PLAYING) {
    uint32_t t_now = ESP.getCycleCount();
    if (t_last > t_now) {
      next_cycle = t_now;
      next_audio = t_now;
    }

    if (t_now >= next_cycle) {
      next_cycle += cycle_period;
      clock_apu();
    }

    if (t_now >= next_audio) {
      next_audio += audio_period;
      parse_vgm(music, looping);

      audio_counter++;
      if (audio_counter >= audio_divisor) {
        audio_counter = 0;
        sample_audio();
        render_audio(volume);
      }
    }
    t_last = t_now;

  // if (digitalRead(27) == LOW) NES_PLAYING = false;//button press then stop
  }//while nes playing
}

void Cartridge::parse_vgm(const uint8_t* music, bool looping){
  uint8_t extra = 0;
  if (vgm_wait == 0) {
    uint8_t command = music[vgm_index];

    if (command == 0x61) { // Wait nL nH samples
      uint16_t result = music[vgm_index + 2];
      result = result << 8;
      result += music[vgm_index + 1];
      vgm_wait = result;
      extra = 2;
    }
    else if (command == 0x62) { // Wait 735 samples
      vgm_wait = 735;
    }
    else if (command == 0x63) { // Wait 882 samples
      vgm_wait = 882;
    }
    else if (command == 0x67 && music[vgm_index + 1] == 0x66) { // Start of data block
      uint32_t block_size = (music[vgm_index + 3]      ) + 
                  (music[vgm_index + 4] <<  8) + 
                  (music[vgm_index + 5] << 16) + 
                  (music[vgm_index + 6] << 24); 
      vgm_index += block_size - 1;  // Basically ignore all data block contents
    }
    else if (command == 0x66 || vgm_index == VGM_EOF_OFFSET){
      Serial.println("END");
      if(looping) {
        if(VGM_LOOP_OFFSET == 0) {  // Loop offset is not built into file
          vgm_index = VGM_DATA_OFFSET - 1;  // Restart the song entirely
        }
        else {
          vgm_index = VGM_LOOP_OFFSET - 1;  // Music loops, reset to looping byte
        }
      }
      else {
        reset_nes();  // Reset if loop opt-out
      }
    }
    else if (command >= 0x70 && command < 0x80) { // Wait (0x7)n+1 samples
      uint8_t result = command;
      result = result << 4;
      result = result >> 4;
      vgm_wait = result + 1;
    }
    else if (command == 0xB4) { // NES APU write dd to aa
      uint8_t value = music[vgm_index + 2];
      uint8_t reg = music[vgm_index + 1];

      write_reg(reg, value);
      extra = 2;
    }

    vgm_index += 1;
    vgm_index += extra;
  }
  else {
    vgm_wait--;
  }
}

void Cartridge::read_vgm_header(const uint8_t* music){
  VGM_EOF_OFFSET = get_32_bit(music, 0x04) + 0x04;
  VGM_TOTAL_NUM_SAMPLES = get_32_bit(music, 0x18);
  VGM_RATE = get_32_bit(music, 0x24);
  VGM_DATA_OFFSET = get_32_bit(music, 0x34) + 0x34;
  VGM_NES_APU_CLOCK = get_32_bit(music, 0x84);
  VGM_LOOP_OFFSET = get_32_bit(music, 0x1C) + 0x1C;

  vgm_index = VGM_DATA_OFFSET;
}

uint32_t Cartridge::get_32_bit(const uint8_t* music, uint32_t index){
  uint32_t result = music[index + 3];
  result = result << 8;
  result += music[index + 2];
  result = result << 8;
  result += music[index + 1];
  result = result << 8;
  result += music[index];

  return result;
}

void Cartridge::reset_nes(){
  for (uint8_t i = 0; i < 24; i++) {
    NES_REG[i] = B00000000;
  }
  single_output = 0;
  // Pulse 1 Variables
  p1_output = 0;
  p1_11_bit_timer = 0;
  p1_wave_index = 0;
  p1_length_counter = 0;
  p1_envelope_divider = 0;
  p1_decay_counter = 0;
  p1_volume = 0;
  p1_channel = 0;

  // Pulse 2 Variables
  p2_output = 0;
  p2_11_bit_timer = 0;
  p2_wave_index = 0;
  p2_length_counter = 0;
  p2_envelope_divider = 0;
  p2_decay_counter = 0;
  p2_volume = 0;
  p2_channel = 1;

  // Noise Variables
  n_output = 0;
  n_timer = 0;
  n_length_counter = 0;
  n_envelope_divider = 0;
  n_decay_counter = 0;
  n_volume = 0;
  n_xor = 0;
  n_lsfr = 1;
  n_channel = 2;

  // Triangle Variables
  t_output = 0;
  t_11_bit_timer = 0;
  t_wave_index = 0;
  t_length_counter = 0;
  t_linear_counter = 0;
  t_channel = 3;

  vgm_index = 0;
  vgm_wait = 0;
  NES_PLAYING = false;

  next_audio = 0;
  next_cycle = 0;
  cpu_cycles = 0;
  apu_cycles = 0;

  t_last = 0;

  audio_counter = 0;
}

void Cartridge::sample_audio(){
  // Pulse 1
  if (p1_length_counter > 0) {
    if (p1_11_bit_timer >= 8) {
      p1_output = p1_volume * duty_table[get_duty(0x00)][p1_wave_index];
    }
  }
  else {
    p1_output = 0;
  }

  // Pulse 2
  if (p2_length_counter > 0) {
    if (p2_11_bit_timer >= 8) {
      p2_output = p2_volume * duty_table[get_duty(0x04)][p2_wave_index];
    }
  }
  else {
    p2_output = 0;
  }

  // Noise
  if (n_length_counter > 0) {
    n_output = n_volume * n_xor;
  }
  else {
    n_output = 0;
  }

  // Triangle
  if (t_length_counter > 0) {
    t_output = tri_table[t_wave_index];
  }
  else {
    t_output = 0;
  }
}

void Cartridge::render_audio(float gain){
  // four channel
 /*
  sigmaDeltaWrite(p1_channel, p1_output * 12);
  sigmaDeltaWrite(p2_channel, p2_output * 12);
  sigmaDeltaWrite(n_channel, n_output * 6);
  sigmaDeltaWrite(t_channel, t_output * 16);
*/
  // single channel
  
  uint8_t sum = 0;
  sum+=p1_output*0.8;
  sum+=p2_output*0.8;
  sum+=n_output*0.7;
  sum+=t_output;
  //sigmaDeltaWrite(t_pin,sum);
  dacWrite(single_output,sum * gain);//gain volume to 0.5 or can be to 1.5

  
}

void Cartridge::clock_apu(){
  clock_frame_counter();
  decrement_timers();
  apu_cycles += 2;
}

void Cartridge::decrement_timers(){
  // Pulse 1
  if (p1_11_bit_timer <= 0) {
    p1_11_bit_timer = get_11_bit_timer(0x02, 0x03);
    if (p1_wave_index == 0) {
      p1_wave_index = 7;
    }
    else {
      p1_wave_index--;
    }
  }
  else {
    p1_11_bit_timer -= 2;
  }

  // Pulse 2
  if (p2_11_bit_timer <= 0) {
    p2_11_bit_timer = get_11_bit_timer(0x06, 0x07);
    if (p2_wave_index == 0) {
      p2_wave_index = 7;
    }
    else {
      p2_wave_index--;
    }
  }
  else {
    p2_11_bit_timer -= 2;
  }

  // Noise
  if (n_timer <= 0) {
    uint8_t index = get_reg(0x0E);
    index = index << 4;
    index = index >> 4;
    n_timer = noise_table[index];

    clock_lsfr();
  }
  else {
    n_timer -= 2;
  }

  // Triangle
  if (t_11_bit_timer <= 0) {
    t_11_bit_timer = get_11_bit_timer(0x0A, 0x0B) + 1;

    if (t_length_counter > 0 && t_linear_counter > 0) {
      if (t_wave_index == 0) {
        t_wave_index = 31;
      }
      else {
        t_wave_index--;
      }
    }
  }
  else {
    t_11_bit_timer -= 2;
    t_11_bit_timer -= 2; // Triangle timer clocked on CPU not APU (2x speed)
  }
}

void Cartridge::clock_lsfr(){
  uint8_t bit1 = 0;
  uint8_t bit2 = 1;
  if (get_bit(0x0E, 7) == 1) {
    bit2 = 6;
  }
  uint8_t val1 = bitRead(n_lsfr, bit1);
  uint8_t val2 = bitRead(n_lsfr, bit2);
  n_xor = val1 ^ val2;

  n_lsfr = n_lsfr >> 1;
  bitWrite(n_lsfr, 14, n_xor);
}

void Cartridge::clock_frame_counter(){
  if (frame_counter_mode() == 0) { // 4-step sequence
    if (apu_cycles == 3728) {
      clock_envelopes();
      clock_linear_counter();

      if (fc_callback)
	      fc_callback();
    }
    else if (apu_cycles == 7456) {
      clock_envelopes();
      clock_linear_counter();
      clock_length_counters();
      clock_sweep_units();
    }
    else if (apu_cycles == 11186) {
      clock_envelopes();
      clock_linear_counter();
    }
    else if (apu_cycles == 14914) {
      clock_envelopes();
      clock_linear_counter();
      clock_length_counters();
      clock_sweep_units();
      apu_cycles = 0;
    }
  }
  else if (frame_counter_mode() == 1) { // 5-step sequence
    if (apu_cycles == 3728) {
      clock_envelopes();
      clock_linear_counter();
      
      if (fc_callback)
	      fc_callback();
    }
    else if (apu_cycles == 7456) {
      clock_envelopes();
      clock_linear_counter();
      clock_length_counters();
      clock_sweep_units();
    }
    else if (apu_cycles == 11186) {
      clock_envelopes();
      clock_linear_counter();
    }
    else if (apu_cycles == 18640) {
      clock_envelopes();
      clock_linear_counter();
      clock_length_counters();
      clock_sweep_units();
      apu_cycles = 0;
    }
  }
}

uint8_t Cartridge::frame_counter_mode(){
  return get_bit(0x17, 7);
}

uint8_t Cartridge::get_bit(uint8_t reg, uint8_t b){
  return bitRead(get_reg(reg), b);
}

void Cartridge::write_bit(uint8_t reg, uint8_t b, uint8_t val){
  uint8_t r = get_reg(reg);
  bitWrite(r, b, val);
  write_reg(reg, r); // used to make sure conditions defined in write_reg() are used here also
}

uint8_t Cartridge::get_reg(uint8_t reg){
  return NES_REG[reg];
}

void Cartridge::write_reg(uint8_t reg, uint8_t val){
  // PULSE 1 ----------------------------------------
  if (reg == 0x03) {
    // Pulse 1 load length counter from table address
    uint8_t level = bitRead(val, 3);
    uint8_t index = val >> 4;
    p1_length_counter = length_table[level][index];
    p1_wave_index = 0;

    p1_decay_counter = 15;
    p1_envelope_divider = 0;

  }
  else if (reg == 0x15) {
    // Pulse 1
    if (bitRead(val, 0) == 0) { // if p1 enabled bit clear, empty length counter
      write_length_counter(0x03, 0);
    }

    //clear DMC interrupt flag
  }

  // PULSE 2 ----------------------------------------
  if (reg == 0x07) {
    // Pulse 2 load length counter from table address
    uint8_t level = bitRead(val, 3);
    uint8_t index = val >> 4;
    p2_length_counter = length_table[level][index];
    p2_wave_index = 0;

    p2_decay_counter = 15;
    p2_envelope_divider = 0;

  }
  else if (reg == 0x15) {
    // Pulse 2
    if (bitRead(val, 1) == 0) { // if p2 enabled bit clear, empty length counter
      write_length_counter(0x07, 0);
    }

    //clear DMC interrupt flag
  }

  // NOISE ----------------------------------------
  if (reg == 0x0F) {
    // Noise load length counter from table address
    uint8_t level = bitRead(val, 3);
    uint8_t index = val >> 4;
    n_length_counter = length_table[level][index];

    n_decay_counter = 15;
    n_envelope_divider = 0;

  }
  else if (reg == 0x15) {
    // Noise
    if (bitRead(val, 3) == 0) { // if noise enabled bit clear, empty length counter
      write_length_counter(0x0F, 0);
    }

    //clear DMC interrupt flag
  }

  // TRIANGLE ----------------------------------------
  if (reg == 0x0B) {
    // Triangle load length counter from table address
    uint8_t level = bitRead(val, 3);
    uint8_t index = val >> 4;
    t_length_counter = length_table[level][index];

    // Triangle load linear counter from register
    uint8_t linc = get_reg(0x08);
    linc = linc << 1;
    linc = linc >> 1;
    t_linear_counter = linc;

    write_bit(0x08, 7, 1);
  }
  else if (reg == 0x15) {

    //clear DMC interrupt flag
  }

  NES_REG[reg] = val;
}

void Cartridge::clock_envelopes(){
  // Pulse 1
  uint8_t p1_envelope_disable = bitRead(get_reg(0x00), 4);
  if (p1_envelope_disable) {
    uint8_t v = get_reg(0x00);
    v = v << 4;
    v = v >> 4;
    p1_volume = v;
  }
  else {
    uint8_t p1_envelope_loop = bitRead(get_reg(0x00), 5);

    if (p1_envelope_divider > 0) {
      p1_envelope_divider--;
    }
    else {
      uint8_t d = get_reg(0x00);
      d = d << 4;
      d = d >> 4;
      p1_envelope_divider = d;

      //clock decay counter
      if (p1_decay_counter > 0) {
        p1_decay_counter--;
      }
      else {
        if (p1_envelope_loop == 1) {
          p1_decay_counter = 15;
        }
      }
    }

    p1_volume = p1_decay_counter;
  }

  // Pulse 2
  uint8_t p2_envelope_disable = bitRead(get_reg(0x04), 4);
  if (p2_envelope_disable) {
    uint8_t v = get_reg(0x04);
    v = v << 4;
    v = v >> 4;
    p2_volume = v;
  }
  else {
    uint8_t p2_envelope_loop = bitRead(get_reg(0x04), 5);

    if (p2_envelope_divider > 0) {
      p2_envelope_divider--;
    }
    else {
      uint8_t d = get_reg(0x04);
      d = d << 4;
      d = d >> 4;
      p2_envelope_divider = d;

      //clock decay counter
      if (p2_decay_counter > 0) {
        p2_decay_counter--;
      }
      else {
        if (p2_envelope_loop == 1) {
          p2_decay_counter = 15;
        }
      }
    }

    p2_volume = p2_decay_counter;
  }

  // Noise
  uint8_t n_envelope_disable = bitRead(get_reg(0x0C), 4);
  if (n_envelope_disable) {
    uint8_t v = get_reg(0x0C);
    v = v << 4;
    v = v >> 4;
    n_volume = v;
  }
  else {
    uint8_t n_envelope_loop = bitRead(get_reg(0x0C), 5);

    if (n_envelope_divider > 0) {
      n_envelope_divider--;
    }
    else {
      uint8_t d = get_reg(0x0C);
      d = d << 4;
      d = d >> 4;
      n_envelope_divider = d;

      //clock decay counter
      if (n_decay_counter > 0) {
        n_decay_counter--;
      }
      else {
        if (n_envelope_loop == 1) {
          n_decay_counter = 15;
        }
      }
    }

    n_volume = n_decay_counter;
  }
}

void Cartridge::clock_linear_counter(){
  if (get_bit(0x08, 7) == 1) {
    if (t_linear_counter > 0) {
      t_linear_counter--;
    }
  }
}

void Cartridge::clock_length_counters(){
  uint8_t length_counter_halt_flag = 0;

  // Pulse 1
  length_counter_halt_flag = get_bit(0x00, 5);
  if (length_counter_halt_flag == 0) { // Pulse 1 halt flag not set? decrement length counter
    if (p1_length_counter > 0) {
      p1_length_counter--;
    }
  }

  // Pulse 2
  length_counter_halt_flag = get_bit(0x04, 5);
  if (length_counter_halt_flag == 0) { // Pulse 2 halt flag not set? decrement length counter
    if (p2_length_counter > 0) {
      p2_length_counter--;
    }
  }

  // Noise
  length_counter_halt_flag = get_bit(0x0C, 5);
  if (length_counter_halt_flag == 0) { // Noise halt flag not set? decrement length counter
    if (n_length_counter > 0) {
      n_length_counter--;
    }
  }

  // Triangle
  length_counter_halt_flag = get_bit(0x08, 7);
  if (length_counter_halt_flag == 0) { // Triangle halt flag not set? decrement length counter
    if (t_length_counter > 0) {
      t_length_counter--;
    }
  }
}

uint8_t Cartridge::get_duty(uint8_t reg){
  uint8_t r = get_reg(reg);
  r = r >> 6;
  return r;
}

uint16_t Cartridge::get_11_bit_timer(uint8_t reg_low, uint8_t reg_high){
  uint8_t rlow = get_reg(reg_low);
  uint8_t rhigh = get_reg(reg_high);
  rhigh = rhigh << 5;
  rhigh = rhigh >> 5;
  uint16_t r16 = rhigh;
  r16 = r16 << 8;
  r16 = r16 + rlow;
  return r16;
}

void Cartridge::set_11_bit_timer(uint8_t reg_low, uint8_t reg_high, uint16_t val){
  uint16_t out_low = val;
  out_low = out_low << 8;
  out_low = out_low >> 8;
  uint8_t ol = out_low;

  uint16_t out_high = val;
  out_high = out_high >> 8;
  uint8_t oh = out_high;

  uint8_t r_high = get_reg(reg_high);

  uint16_t val_high = r_high;
  val_high = val_high >> 3;
  val_high = val_high << 3;
  val_high = val_high + oh;

  write_reg(reg_low, ol);
  write_reg(reg_high, val_high);
}

uint8_t Cartridge::get_length_counter(uint8_t reg){
  uint8_t r = get_reg(reg);
  r = r >> 3;
  return r;
}

uint8_t Cartridge::write_length_counter(uint8_t reg, uint8_t val){
  uint8_t r = get_reg(reg);
  r = r << 5;
  r = r >> 5;
  val = val << 3;
  uint8_t result = r + val;
  write_reg(reg, result);
  return result;
}

void Cartridge::clock_sweep_units(){
}
