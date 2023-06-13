#ifndef _ProjectInit_H_
#define _ProjectInit_H_

#include <Arduino.h>
#include <RCSwitch.h>
#include "Nidec.h"


#define DEBUG                           True


/*********************************************************
 * V5 
 * ******************************************************/
// #define SW1                             GP4

// #define SW2                             GP2

// #define SW3                             GP15


/*********************************************************
 * V1 - V3
 * ******************************************************/
#define SW1                             GP5

#define SW2                             GP4

#define SW3                             GP2


#define LED                             GP2


typedef enum
{
    MODE_RF                             = 0,

    MODE_BUTTON                         = 1,

    MODE_ADDC                           = 2,
}
MODE_CONTROL_t;




void ProjectInit(void);





extern MODE_CONTROL_t Mode;

#endif
