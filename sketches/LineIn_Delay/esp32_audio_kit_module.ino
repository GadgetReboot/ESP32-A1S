/*
   AC101 interface 
   based on original looper project by Marcel Licence
*/

#include "AC101.h" /* only compatible with forked repo: https://github.com/marcel-licence/AC101 */

/* AC101 pins */
#define IIS_SCLK                    27
#define IIS_LCLK                    26
#define IIS_DSIN                    25
#define IIS_DSOUT                   35

#define IIC_CLK                     32
#define IIC_DATA                    33

static AC101 ac;

/* actually only supporting 16 bit */
#define SAMPLE_SIZE_16BIT
//#define SAMPLE_SIZE_24BIT
//#define SAMPLE_SIZE_32BIT

#define SAMPLE_RATE	44100
#define CHANNEL_COUNT	2
#define WORD_SIZE	16
#define I2S1CLK	(512*SAMPLE_RATE)
#define BCLK	(SAMPLE_RATE*CHANNEL_COUNT*WORD_SIZE)
#define LRCK	(SAMPLE_RATE*CHANNEL_COUNT)



/*
   complete setup of the ac101 to enable in/output
*/
void ac101_setup()
{
  Serial.printf("Connect to AC101 codec... ");
  while (not ac.begin(IIC_DATA, IIC_CLK))
  {
    Serial.printf("Failed!\n");
    delay(1000);
  }
  Serial.printf("OK\n");
#ifdef SAMPLE_SIZE_24BIT
  ac.SetI2sWordSize(AC101::WORD_SIZE_24_BITS);
#endif
#ifdef SAMPLE_SIZE_16BIT
  ac.SetI2sWordSize(AC101::WORD_SIZE_16_BITS);
#endif

#if (SAMPLE_RATE==44100)&&(defined(SAMPLE_SIZE_16BIT))
  ac.SetI2sSampleRate(AC101::SAMPLE_RATE_44100);
  /*
     BCLK: 44100 * 2 * 16 =

     I2S1CLK/BCLK1 -> 512 * 44100 / 44100*2*16
     BCLK1/LRCK -> 44100*2*16 / 44100 Obacht ... ein clock cycle goes high and low
     means 32 when 32 bits are in a LR word channel * word_size
  */
  ac.SetI2sClock(AC101::BCLK_DIV_16, false, AC101::LRCK_DIV_32, false);
  ac.SetI2sMode(AC101::MODE_SLAVE);
  ac.SetI2sWordSize(AC101::WORD_SIZE_16_BITS);
  ac.SetI2sFormat(AC101::DATA_FORMAT_I2S);
#endif

  ac.SetVolumeSpeaker(50);
  ac.SetVolumeHeadphone(99);

#if 1
  ac.SetLineSource();
#else
  ac.SetMicSource(); /* handle with care: mic is very sensitive and might cause feedback using amp!!! */
#endif

#if 0
  ac.DumpRegisters();
#endif
}

/*
   selects the microphone as audio source
   handle with care: mic is very sensitive and might cause feedback using amp!!!
*/
void ac101_setSourceMic(void)
{
  ac.SetMicSource();
}

/*
   selects the line in as input
*/
void ac101_setSourceLine(void)
{
  ac.SetLineSource();
}

void ac101_setSpkrVol(int val)
{
  ac.SetVolumeSpeaker(val);
}

void ac101_setHeadphoneVol(int val)
{
  ac.SetVolumeHeadphone(val);
}
