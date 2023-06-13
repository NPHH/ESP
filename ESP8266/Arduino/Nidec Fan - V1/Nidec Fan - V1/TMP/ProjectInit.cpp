#include "ProjectInit.h"




RCSwitch mySwitch = RCSwitch();

MODE_CONTROL_t Mode;




void ProjectInit(void)
{
    delay(3000);

    #if (DEBUG == True)

    Serial.begin(115200);

    Serial.println("Begin...");

    #endif
    

    PinMode(BREAKPin,OutPut);

    DigitalWrite(BREAKPin,High);


    PinMode(SW1,InPutPU);

    PinMode(SW2,InPutPU);

    PinMode(SW3,InPutPU);





    Mode = MODE_BUTTON;

}

