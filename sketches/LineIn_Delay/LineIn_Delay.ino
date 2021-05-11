/*
   This is the top level file for the sketch
   Required AC101 library: https://github.com/marcel-licence/AC101

   based on an audio looper by Marcel Licence
   original project:              https://github.com/marcel-licence/esp32_multitrack_looper
   original project demo video:   https://youtu.be/PKQmOsJ-g1I

   Hardware:  ESP32-A1S   https://www.makerfabs.com/desfile/files/ESP32-A1S%20Product%20Specification.pdf

   Arduino IDE board settings:
   Board: ESP32 Dev Module
   Flash Size: 32Mbit -> 4MB
   PSRAM: Enabled

   Gadget Reboot
*/

// pot ADC pins for parameter setting
#define pot0        4
#define pot1        15
#define pot2        34

static float click_supp_gain = 0.0f;  // minimize clicks when switching audio sources
float main_gain = 1.0f;

void setup()
{
  ac101_setup();
  setup_i2s();

  ac101_setSpkrVol(50);
  ac101_setHeadphoneVol(99);
  ac101_setSourceLine();

  Delay_Init();
  Delay_Reset();

  Delay_SetInputLevel(0.5);
  Delay_SetLevel(0.5);
  Delay_SetLength(0.5);
  Delay_SetFeedback(0.5);

}

void loop()
{
  static uint32_t loop_cnt;  // counter to determine interval for reading pots

  processAudio();            // get audio sample from line in, process with delay effect, send back out

  // read pots periodically
  loop_cnt ++;
  if (loop_cnt >= 40000)
  {
    loop_cnt = 0;
    readPots();
  }
}

// read samples, process, send back out
inline void processAudio()
{
  static float fl_sample, fr_sample;
  static float fl_offset = 0.0f;
  static float fr_offset = 0.0f;

  fl_sample = 0.0f;
  fr_sample = 0.0f;

  i2s_read_stereo_samples(&fl_sample, &fr_sample);

  fl_sample *= click_supp_gain;
  fr_sample *= click_supp_gain;

  if (click_supp_gain < 1.0f)
  {
    click_supp_gain += 0.00001f;
  }
  else
  {
    click_supp_gain = 1.0f;
  }

  fl_offset = fl_offset * 0.99 + fl_sample * 0.01;
  fr_offset = fr_offset * 0.99 + fr_sample * 0.01;

  fl_sample -= fl_offset;
  fr_sample -= fr_offset;

  Delay_Process(&fl_sample, &fr_sample);  // apply delay effect to sample

  /* apply main_gain */
  fl_sample *= main_gain;
  fr_sample *= main_gain;

  i2s_write_stereo_samples(&fl_sample, &fr_sample);
}

void readPots()
{
  int reading;
  float value;

  // set delay time (controls duration of audio sample to repeat, ie short/long repeats)
  reading = analogRead(pot0);
  value = fmap(reading, 0, 4095, 0.0, 1.0);
  Delay_SetLength(value);

  // set delay feedback level (controls how long delayed audio keeps repeating as it fades out on each repeat)
  reading = analogRead(pot1);
  value = fmap(reading, 0, 4095, 0.0, 1.0);
  Delay_SetFeedback(value);

  // set output volume levels
  reading = analogRead(pot2);
  value = fmap(reading, 0, 4095, 0, 99);
  ac101_setSpkrVol(value);
  ac101_setHeadphoneVol(value);

}

// map a floating point number range
float fmap(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
