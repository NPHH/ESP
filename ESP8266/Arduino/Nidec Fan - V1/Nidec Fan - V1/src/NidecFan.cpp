
#include "NidecFan.h"









/******************************************************************************************************************************
						
******************************************************************************************************************************/
uint8_t SW1Status = false;

uint8_t SW2Status = false;

uint8_t SW3Status = false;

/******************************************************************************************************************************
						
******************************************************************************************************************************/
uint32_t CurrentFREQ = 0;

uint16_t FanFREQ = 0;

/******************************************************************************************************************************
						
******************************************************************************************************************************/
uint8_t FanStatus = false;

/******************************************************************************************************************************
						
******************************************************************************************************************************/
uint8_t State = 0;


/******************************************************************************************************************************
						
******************************************************************************************************************************/
unsigned long PreviousMillis = 0;

/******************************************************************************************************************************
						
******************************************************************************************************************************/


/******************************************************************************************************************************
						
******************************************************************************************************************************/

uint8_t* Duty = 0;

void setup() 
{
    pinMode(SW1, INPUT);

    pinMode(SW2, INPUT);

    pinMode(SW3, INPUT);


    NIDEC.Begin(NidecPWM, NidecBreak);
}

void loop() 
{
/*****************************************************************************************************************************/    
    if((digitalRead(SW1) == HIGH) && (digitalRead(SW2) == HIGH) && (digitalRead(SW3) == HIGH) && ((SW1Status == true) || (SW2Status == true) || (SW3Status == true)))
    {
        delay(500);

        SW1Status = false;

        SW2Status = false;

        SW3Status = false;

        Serial.println("Stop");

        NIDEC.Stop();
    }

/*****************************************************************************************************************************/  
    if((digitalRead(SW1) == LOW) && (SW1Status == false))
    {
        delay(500);

        SW1Status = true;

        SW2Status = false;

        SW3Status = false;

        NIDEC.Start(FREQSW1);
    }

/*****************************************************************************************************************************/  
    if((digitalRead(SW2) == LOW) && (SW2Status == false))
    {
        delay(500);

        SW1Status = false;

        SW2Status = true;

        SW3Status = false;

        NIDEC.Start(FREQSW2);
    }

/*****************************************************************************************************************************/  
    if((digitalRead(SW3) == LOW) && (SW3Status == false))
    {
        delay(500);

        SW1Status = false;

        SW2Status = false;

        SW3Status = true;

        NIDEC.Start(FREQSW3);      
    }

/*****************************************************************************************************************************/  

   

    

/******************************************************************************************************************************
						
******************************************************************************************************************************/
    unsigned long CurrentMillis = millis();

    if (CurrentMillis - PreviousMillis >= 1000) 
    {
        PreviousMillis = CurrentMillis;



    }  
}
















