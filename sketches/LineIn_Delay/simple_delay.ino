/*
 * This is a simple implementation of a delay line
 * - level adjustable
 * - feedback
 * - length adjustable
 *
 * here is some magic the module also uses some PSRAM for the second channel!!!
 *
 * Author: Marcel Licence
 */


/* max delay can be changed but changes also the memory consumption */
#define MAX_DELAY	22050*2

/*
 * module variables
 */
int16_t *delayLine_l;
int16_t *delayLine_r;
float delayToMix = 0;
float delayInLvl = 1.0f;
float delayFeedback = 0;
uint32_t delayLen = 11098;
uint32_t delayIn = 0;
uint32_t delayOut = 0;

void Delay_Init(void)
{
    psramInit();
    Serial.printf("Total PSRAM: %d\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());

    delayLine_l = (int16_t *)malloc(sizeof(int16_t) * MAX_DELAY);
    if (delayLine_l == NULL)
    {
        Serial.printf("No more heap memory!\n");
    }
    delayLine_r = (int16_t *)ps_malloc(sizeof(int16_t) * MAX_DELAY);
    if (delayLine_r == NULL)
    {
        Serial.printf("No more heap memory!\n");
    }
    Delay_Reset();
}

void Delay_Reset(void)
{
    for (int i = 0; i < MAX_DELAY; i++)
    {
        delayLine_l[i] = 0;
        delayLine_r[i] = 0;
    }
}

void Delay_Process(float *signal_l, float *signal_r)
{
#if 0
    *signal_l *= (1.0f - delayFeedback);
    *signal_r *= (1.0f - delayFeedback);
#endif

    delayLine_l[delayIn] = (((float)0x8000) * *signal_l * delayInLvl);
    delayLine_r[delayIn] = (((float)0x8000) * *signal_r * delayInLvl);

    delayOut = delayIn + (1 + MAX_DELAY - delayLen);

    if (delayOut >= MAX_DELAY)
    {
        delayOut -= MAX_DELAY;
    }

    *signal_l += ((float)delayLine_l[delayOut]) * delayToMix / ((float)0x8000);
    *signal_r += ((float)delayLine_r[delayOut]) * delayToMix / ((float)0x8000);

    delayLine_l[delayIn] += (((float)delayLine_l[delayOut]) * delayFeedback);
    delayLine_r[delayIn] += (((float)delayLine_r[delayOut]) * delayFeedback);

    delayIn ++;

    if (delayIn >= MAX_DELAY)
    {
        delayIn = 0;
    }
}

void Delay_SetInputLevel(float value)
{
    delayInLvl = value;   
}

void Delay_SetFeedback(float value)
{
    delayFeedback = value;  
}

void Delay_SetLevel(float value)
{
    delayToMix = value;  
}

void Delay_SetLength(float value)
{
    delayLen = (uint32_t)(((float)MAX_DELAY - 1.0f) * value);  
}
