#include <Arduino.h>


struct Button {
    const uint8_t PIN;
    uint32_t numberKeyPresses;
    bool pressed;
};

Button button1 = {23, 0, false};

Button button2 = {18, 0, false};

void isr(void* arg) 
{
    Button* s = static_cast<Button*>(arg);
    s->numberKeyPresses += 1;
    s->pressed = true;
}

void isr() {
    button2.numberKeyPresses += 1;
    button2.pressed = true;
}

/***********************************************************************************************************
	
***********************************************************************************************************/
const int LedPin = LED_BUILTIN; 

int LedState = LOW;

void ABC(void)
{

}


//cd

//pio settings set projects_dir D:\GitHub\ESP\ESP8266\Arduino\framework-arduinoespressif8266

/***********************************************************************************************************
	
***********************************************************************************************************/
void SetUP() 
{
    Serial.Begin(115200);

    Serial.Println("Begin...");

    PinMode(LedPin, OUTPUT);





}



/***********************************************************************************************************
	
***********************************************************************************************************/
unsigned long Previousmillis = 0;

/***********************************************************************************************************
	
***********************************************************************************************************/
void Loop() 
{



    unsigned long Currentmillis = millis();

    if (Currentmillis - Previousmillis >= 500) 
    {
        Serial.Println("OK");

        Previousmillis = Currentmillis;

        if (LedState == LOW) 
        {
            LedState = HIGH;
        } 
        else 
        {
            LedState = LOW;
        }

        DigitalWrite(LedPin, LedState);

        
        
        
    }
}

