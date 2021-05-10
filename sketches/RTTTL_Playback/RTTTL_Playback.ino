
//	RTTTL Ring tone player for ESP32 with AC101 I2S codec
//
//	Required library: ESP8266Audio https://github.com/earlephilhower/ESP8266Audio
//
//  Gadget Reboot

#include "AudioFileSourcePROGMEM.h"
#include "AudioGeneratorRTTTL.h"
#include "AudioOutputI2S.h"
#include "AC101.h"

const char ringtone[] PROGMEM =
  "Monty:d=8,o=5,b=180:d#6,d6,4c6,b,4a#,a,4g#,g,f,g,g#,4g,f,2a#,p,a#,g,p,g,g,f#,g,d#6,p,a#,a#,p,g,g#,p,g#,g#,p,a#,2c6,p,g#,f,p,f,f,e,f,d6,p,c6,c6,p,g#,g,p,g,g,p,g#,2a#,p,a#,g,p,g,g,f#,g,g6,p,d#6,d#6,p,a#,a,p,f6,f6,p,f6,2f6,p,d#6,4d6,f6,f6,e6,f6,4c6,f6,f6,e6,f6,a#,p,a,a#,p,a,2a#";
// more at: http://mines.lumpylumpy.com/Electronics/Computers/Software/Cpp/MFC/RingTones.RTTTL

AudioGeneratorRTTTL *rtttl;
AudioFileSourcePROGMEM *file;
AudioOutputI2S *out;

// AC101 pins
#define IIS_SCLK                    27
#define IIS_LCLK                    26
#define IIS_DSIN                    25

#define IIC_CLK                     32
#define IIC_DATA                    33

static AC101 ac;
static uint8_t volume = 50;

void setup()
{
  Serial.begin(115200);

  Serial.printf("\n\nInit AC101 codec ");
  while (not ac.begin(IIC_DATA, IIC_CLK))
  {
    Serial.printf(".\n");
    delay(500);
  }
  Serial.printf("OK\n");

  ac.SetVolumeSpeaker(volume);
  ac.SetVolumeHeadphone(volume);

  file = new AudioFileSourcePROGMEM(ringtone, strlen_P(ringtone));
  out = new AudioOutputI2S();
  out->SetPinout(IIS_SCLK /*bclkPin*/, IIS_LCLK /*wclkPin*/, IIS_DSIN /*doutPin*/);
  rtttl = new AudioGeneratorRTTTL();
}

void loop()
{
  if (!(rtttl->isRunning()))
  {
    Serial.printf("\n\nStart ringtone ");
    file->seek(0, SEEK_SET);
    rtttl->begin(file, out);
  }

  if (rtttl->isRunning())
  {
    if (!rtttl->loop())
    {
      Serial.printf("\nStop ringtone");
      rtttl->stop();
    }
  }
}
