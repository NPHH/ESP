#include "Nidec.h"

/******************************************************************************************************************************
						
******************************************************************************************************************************/
NidecClass NIDEC;

/******************************************************************************************************************************
						
******************************************************************************************************************************/
os_timer_t TimerTick;

/******************************************************************************************************************************
						
******************************************************************************************************************************/
void ICACHE_RAM_ATTR TimerPWMFunc(void) 
{
    NIDEC.HandlePWM();
}

void ICACHE_RAM_ATTR TimerTickFunc(void)    
{
   NIDEC.HandleTICK();
}

/******************************************************************************************************************************
						
******************************************************************************************************************************/
void NidecClass::Begin(uint8_t PWMPin, uint8_t BreakPin)
{
    #ifdef DEBUG_NIDEC

    Serial.begin(9600);

    Serial.println("Begin...");

    #endif

    pinMode(PWMPin, OUTPUT);

    pinMode(BreakPin, OUTPUT);

    this->_PWMPin = PWMPin;

    this->_BreakPin = BreakPin;

    this->_CurrentFREQ = 0;

    this->_FREQDir = FREQ_INC;


    timer1_attachInterrupt(TimerPWMFunc);

    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);   //  5 Tick/us


    os_timer_disarm(&TimerTick);

	os_timer_setfn(&TimerTick, (os_timer_func_t *)TimerTickFunc, (void *) 0);
}

/******************************************************************************************************************************
						
******************************************************************************************************************************/
void NidecClass::FREQ(uint32_t FREQ_)
{
    if(FREQ_ == 0) return;

    if(FREQ_ > MAXFREQ) return;

    this->_Reload = 2500000 / FREQ_;

    #ifdef DEBUG_NIDEC

    Serial.print("Reload Value: ");

    Serial.println(this->_Reload);

    Serial.println();

    #endif

    timer1_write(this->_Reload); 
}

/******************************************************************************************************************************
						
******************************************************************************************************************************/
void NidecClass::Start(uint32_t FREQ_)
{
    #ifdef DEBUG_NIDEC

    if(FREQ_ == FREQSW1)
    {
        Serial.println("NumBer 1.");
    }
    else
        if(FREQ_ == FREQSW2)
        {
            Serial.println("NumBer 2.");
        }
        else
            if(FREQ_ == FREQSW3)
            {
                Serial.println("NumBer 3.");
            } 
            else
                if(FREQ_ == 0)
                {
                    Serial.println("NumBer 0.");
                } 
    
    Serial.println();

    #endif

    if(this->_FREQ != FREQ_)
    {
        this->_FREQ = FREQ_;

        digitalWrite(this->_BreakPin, LOW);

        if(this->_CurrentFREQ < this->_FREQ)
        {
            this->_CurrentFREQ += PWMSTEP;

            this->_FREQDir = FREQ_INC;
        }
        else
        {
            this->_CurrentFREQ -= PWMSTEP;

            this->_FREQDir = FREQ_DEC;
        }
        
        os_timer_arm(&TimerTick, 1000, 1);

        #ifdef DEBUG_NIDEC

        Serial.println("Tick Enable");

        Serial.print("Frequency: ");

        Serial.println(this->_FREQ);

        Serial.print("Current Frequency: ");

        Serial.println(this->_CurrentFREQ);

        Serial.println();

        #endif

        this->FREQ(this->_CurrentFREQ);
    }
}

/******************************************************************************************************************************
						
******************************************************************************************************************************/
void NidecClass::Stop(void)
{
    this->Start(0);
}

/******************************************************************************************************************************
						
******************************************************************************************************************************/
void NidecClass::HandlePWM(void)
{
    if(this->_State == HIGH)
    {
        this->_State = LOW;
    }
    else 
    {
        this->_State = HIGH;
    }
    
    digitalWrite(this->_PWMPin, this->_State);  

    timer1_write(this->_Reload);
}

/******************************************************************************************************************************
						
******************************************************************************************************************************/
void NidecClass::HandleTICK(void)
{
    if(this->_FREQDir == FREQ_INC)
    {
        this->_CurrentFREQ += PWMSTEP;

        if(this->_CurrentFREQ > this->_FREQ) 
        {
            os_timer_disarm(&TimerTick);

            #ifdef DEBUG_NIDEC

            Serial.println("Tick Disable");

            #endif
        }
        else
        {
            #ifdef DEBUG_NIDEC

            Serial.print("Frequency: ");

            Serial.println(this->_FREQ);

            Serial.print("Current Frequency: ");

            Serial.println(this->_CurrentFREQ);

            Serial.println();

            #endif
            
            this->FREQ(this->_CurrentFREQ);
        } 
    }
    else
    {
        this->_CurrentFREQ -= PWMSTEP;

        if(this->_CurrentFREQ < this->_FREQ) 
        {
            os_timer_disarm(&TimerTick);
        }
        else
        {
            #ifdef DEBUG_NIDEC

            Serial.print("Frequency: ");

            Serial.println(this->_FREQ);

            Serial.print("Current Frequency: ");

            Serial.println(this->_CurrentFREQ);

            Serial.println();

            #endif

            if(this->_CurrentFREQ == 0)
            {
                digitalWrite(this->_BreakPin, HIGH);

                os_timer_disarm(&TimerTick);

                #ifdef DEBUG_NIDEC

                Serial.println("Tick Disable And Turn Off");

                #endif
            }
            else
            {
                this->FREQ(this->_CurrentFREQ);
            }
        }
    } 

    


    
}







