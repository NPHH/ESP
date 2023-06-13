#ifndef _Nidec_H_
#define _Nidec_H_

#include <Arduino.h>
#include "ProjectInit.h"




#define PWMPin                                              D5

#define BREAKPin                                            D7


#define MAXFREQ                                             460

#define PWMSTEP                                             20


/*********************************************************
 * 5 Canh
 * ******************************************************/


// #define FREQSW1                                             180

// #define FREQSW2                                             260

// #define FREQSW3                                             460             


/*********************************************************
 * 3 Canh
 * ******************************************************/

#define FREQSW1                                             200

#define FREQSW2                                             240

#define FREQSW3                                             280           


#define RFKeyA                                              8949922

#define RFKeyB                                              8949928 

#define RFKeyC                                              8949924 

#define RFKeyD                                              8949921 


uint8_t PWMInc(uint16_t FREQ);

uint8_t PWMDec(uint16_t FREQ);

void FanPWM(uint16 FREQ);

void FanRun(void);

void FanStop(void);

void FanRFControl(void);

void FanButtonControl(void);


void ModeButtonControl(void);



extern uint16_t CurrentFREQ;

extern uint16_t FanFREQ;

extern uint8_t SW1Status;

extern uint8_t SW2Status;

extern uint8_t SW3Status;

extern uint8_t FanStatus;


extern uint8_t LastSW;

extern RCSwitch RF;

#endif