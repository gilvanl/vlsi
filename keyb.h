// keyb.h - routines for keyboard
#ifndef __KEYB_H__
#define __KEYB_H__

#include <vstypes.h>

// keyboard pins on:
// port0 bit numbers: 10, 11, 14, 15. 
// port2 bit numbers: 0, 1, 2, 3, 4. 

// keyboard codes
#define  UP     0x01  // 0  gpio2_0
#define  DOWN   0x02  // 1
#define  RIGHT  0x04  // 2
#define  LEFT   0x08  // 3
#define  ALT    0x10  // 4  gpio2_4
#define  MENU  0x400  // MENU, MODE 10  gpio0_10
#define  ENTER 0x800  // PLAY,YES,PAUSE 11  gpio0_11
#define  ESC  0x4000  // NO, CANCEL 14  gpio0_14
#define  STOP 0x8000  // STOP, OFF 15  gpio0_15

u_int16 key;          // keyboard bufffer

// keyboard read
void keybr(void){
    key=0;
    key = USEY(GPIO0_IDATA);
    key&=0xCC00;
    if(!key){ 
        key =  USEY(GPIO2_IDATA); 
        key&=0x001F; 
    }
}

void initkeyboard(void){
    (USEY(GPIO0_DDR) &= (0x33FF)); // set inputs on port0 (all pins input by default)
    (USEY(GPIO2_DDR) &= (0xFFE0)); // set inputs on port2 
}

#endif  // __KEYB_H__ 
