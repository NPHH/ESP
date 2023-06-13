#include "Nidec.h"



uint16_t CurrentFREQ = 0;

uint16_t FanFREQ = 0;

uint8_t RunMode = 0;


uint8_t SW1Status = false;

uint8_t SW2Status = false;

uint8_t SW3Status = false;

uint8_t LastSW = 0;

uint8_t FanStatus = false;


RCSwitch RF = RCSwitch();


void FanRFControl(void)
{
    if (RF.available()) 
    {
        Mode = MODE_RF;

        if(RF.getReceivedValue() == RFKeyA)
        {
            #if (DEBUG == true)

            Serial.println("CheckA Key OK");

            #endif

            FanFREQ = 0;

            FanStatus = false;
        }

        if(RF.getReceivedValue() == RFKeyB)
        {
            #if (DEBUG == true)

            Serial.println("CheckB Key OK");

            #endif

            FanFREQ = FREQSW1;

            FanStatus = true;

            FanRun();
        }

        if(RF.getReceivedValue() == RFKeyC)
        {
            #if (DEBUG == true)

            Serial.println("CheckC Key OK");

            #endif

            FanFREQ = FREQSW2;

            FanStatus = true;

            FanRun();
        }

        if(RF.getReceivedValue() == RFKeyD)
        {
            #if (DEBUG == true)

            Serial.println("CheckD Key OK");

            #endif

            FanFREQ = FREQSW3;

            FanStatus = true;

            FanRun();
        }

        RF.resetAvailable();   

        // SW1Status = false;

        // SW2Status = false;

        // SW3Status = false;

    }
}

void FanButtonControl(void)
{
    if((digitalRead(SW1) == HIGH) && (digitalRead(SW2) == HIGH) && (digitalRead(SW3) == HIGH) && ((SW1Status == true) || (SW2Status == true) || (SW3Status == true)))
    {
        delay(500);

        FanFREQ = 0;

        SW1Status = false;

        SW2Status = false;

        SW3Status = false;

        FanStatus = false;
    }

    if((digitalRead(SW1) == Low) && (SW1Status == false))
    {
        Mode = MODE_BUTTON;

        FanFREQ = FREQSW1;
        
        SW1Status = true;

        SW2Status = false;

        SW3Status = false;

        FanStatus = true;

        FanRun();
    }

    if((digitalRead(SW2) == Low) && (SW2Status == false))
    {
        Mode = MODE_BUTTON;

        FanFREQ = FREQSW2;

        SW1Status = false;

        SW2Status = true;

        SW3Status = false;

        FanStatus = true;

        FanRun();
    }

    if((digitalRead(SW3) == Low) && (SW3Status == false))
    {
        Mode = MODE_BUTTON;

        FanFREQ = FREQSW3;

        SW1Status = false;

        SW2Status = false;

        SW3Status = true;

        FanStatus = true;

        FanRun();
    }
}















void ModeButtonControl(void)
{
    if((digitalRead(SW1) == HIGH) && (digitalRead(SW2) == HIGH) && (digitalRead(SW3) == HIGH) && ((SW1Status == true) || (SW2Status == true) || (SW3Status == true)))
    {
        delay(500);

        if((digitalRead(SW1) == HIGH) && (digitalRead(SW2) == HIGH) && (digitalRead(SW3) == HIGH)) PWMDec(0);

        DigitalWrite(BREAKPin,HIGH);

        SW1Status = false;

        SW2Status = false;

        SW3Status = false;
    }

    if((digitalRead(SW1) == Low) && (SW1Status == false))
    {
        if(CurrentFREQ < FREQSW1)
        {
            SW1Status = PWMInc(FREQSW1);
        }
        else
        {
            SW1Status = PWMDec(FREQSW1);
        }

        SW2Status = false;

        SW3Status = false;
    }

    if((digitalRead(SW2) == Low) && (SW2Status == false))
    {
        if(CurrentFREQ < FREQSW2)
        {
            SW2Status = PWMInc(FREQSW2);
        }
        else
        {
            SW2Status = PWMDec(FREQSW2);
        }

        SW1Status = false;

        SW3Status = false;
    }

    if((digitalRead(SW3) == Low) && (SW3Status == false))
    {
        SW3Status = PWMInc(FREQSW3);

        SW1Status = false;

        SW2Status = false;
    }

    



}





uint8_t PWMInc(uint16_t FREQ)
{
    uint16_t i = CurrentFREQ;

    DigitalWrite(BREAKPin,Low);

    analogWriteRange(100);

    while (i <= FREQ)
    {
        Serial.println(i);

        analogWriteFreq(i);

        analogWrite(PWMPin, 50);

        i = i + PWMSTEP;

        delay(1000);        
    }

    CurrentFREQ = FREQ;

    return true;
}

uint8_t PWMDec(uint16_t FREQ)
{
    uint16_t i = CurrentFREQ - PWMSTEP;

    DigitalWrite(BREAKPin,Low);

    analogWriteRange(100);

    while (i >= FREQ)
    {
        Serial.println(i);
 
        analogWriteFreq(i);

        analogWrite(PWMPin, 50);

        if(i != 0) 
        {
            i = i - PWMSTEP;
        }
        else
        {
            break;
        }
        
        delay(1000);        
    }

    CurrentFREQ = FREQ;

    return true;
}



void FanPWM(uint16 FREQ)
{
    analogWriteFreq(FREQ); 

    analogWriteRange(100);    

    analogWrite(PWMPin, 50);  
}

void FanRun(void)
{
    DigitalWrite(BREAKPin,Low);
}

void FanStop(void)
{
    DigitalWrite(BREAKPin,HIGH);
}

