// lcdvs.c - funtions for lcd's HD44780 in 4 bit mode - processors VLSI VS1005 - see header lcdvs.h

#include "lcdvs.h"
#include <timers.h>
#include <string.h>
#include <clockspeed.h>   // microsecond delay
#include <vs1005g.h>

#define  GPIO_WritePort(gpioport,mask)  ( USEY(gpioport) = (mask) )   // set gpio p0rt to specified value
#define  CLEAR_MASK (port+6)

static u_int16 port;      // signals and data - upper 4 bits hardwired to nibble bits 4-7 of the port
static u_int16 pos=0;     // position on string to scroll

void lcd_send_nibble(u_int16 display, u_int16 n, u_int16 rs){
      u_int16 msk=0;
      n &= 0x00F0;
      if(rs)n |= 0x01;
      GPIO_WritePort(port, n);
      DelayMicroSec(1);   // 140 ns minimum	
      switch(display){
         case 2 : msk=0x04; break;
         case 3 : msk=0x08; break;
        default : msk=0x02; break;
      }
      n |= msk;
      GPIO_WritePort(port, n);   // lcd.enable=1
      DelayMicroSec(1);   //450 ns minimum
      msk=0x0F;  if(rs)msk = 0x0E;
      GPIO_WritePort(CLEAR_MASK, msk);  // enable=0
      DelayMicroSec(1);   //450 ns minimum
      GPIO_WritePort(CLEAR_MASK, 0x0F);
}

void lcd_putc(u_int16 display, u_int16 datachar){
     GPIO_WritePort(port, 0x01);  //rs=1 en=0
     DelayMicroSec(1);    // 140 ns minimo
     lcd_send_nibble(display, datachar, 0x01);
     lcd_send_nibble(display, (datachar << 4), 0x01);
     DelayMicroSec(50);   //40 us minimum
}

void lcd_send_command(u_int16 display, u_int16 comm){
     GPIO_WritePort(port, 0);  //rs=0 en=0
     DelayMicroSec(1);    // 140 ns minimum
     lcd_send_nibble(display, comm, 0);
     lcd_send_nibble(display, (comm << 4), 0);
     DelayMicroSec(240);  // 200 us minimum
}

void lcd_init(u_int16 display, u_int16 porta){
    port = porta;
    DelayMicroSec(1);
    GPIO_WritePort(port, 0x00);  //clear first 8 output bits - rs=0 and en=0
    Delay(16);
    lcd_send_nibble(display, 0x30, 0);
    Delay(5);
    lcd_send_nibble(display, 0x30, 0);
    Delay(5);
    lcd_send_nibble(display, 0x30, 0);
    Delay(5);
    lcd_send_nibble(display, 0x20, 0);
    DelayMicroSec(90);  // 40 us minimo
    lcd_send_command(display, 0x28);
    lcd_send_command(display, 0x08);
    lcd_send_command(display, 0x0C);
    Delay(2);
    lcd_send_command(display, 0x06);
    Delay(2);
    lcd_send_command(display, 1);  // clear - additional instruction
    Delay(3);
}

void lcd_clear(u_int16 display){
    lcd_send_command(display, 1);
    Delay(3);
}

// lcd_gotoxy()  -- put the cursor in desired coordinates - do not use parameters below 1
void lcd_gotoxy(u_int16 display, u_int16 x, u_int16 y){
     u_int16 laddrs=0;
     switch(y){
          case 2  : laddrs=lcd_line_two;   break;
          case 3  : laddrs=lcd_line_three; break;
          case 4  : laddrs=lcd_line_four;  break;
          default : laddrs=0;              break;
     }
     laddrs+=x;   laddrs--;
     lcd_send_command(display, 0x80|laddrs);  // command: set ddram address - 40 us
}

// lcd_cl3anline - cleans a line of the display (20 characters wide):
void lcd_cleanline(u_int16 display, u_int16 lin){
     u_int16 num_pos;
     num_pos=20;
     lcd_gotoxy(display, 1, lin);
     while(num_pos) {
          lcd_putc(display, ' ');
          num_pos--;
     }
}

void lcd_puts(u_int16 display, const char *string){  // puts a string
     while(*string)lcd_putc(display, *string++);
}

void lcdmsg(u_int16 display, char *string, u_int16 len, u_int16 x, u_int16 y){ // puts a string in defined coordinates
     lcd_gotoxy(display, x,y);
     while(*string){ if(len<1)break; lcd_putc(display, *string++); len--; }
}

void lcdmsgl(u_int16 display, char *string, u_int16 len, u_int16 y){  // prints messages in several lines of the display
     u_int16 leng, printed, mxlen;
     printed=0; mxlen=(len*3);
     leng = strlen(string);
     if(leng>mxlen)leng=mxlen;
     lcd_cleanline(display, y);
     if(leng>=len)lcd_cleanline(display, y+1);
     if(leng>=(len*2))lcd_cleanline(display, y+2);
     lcd_gotoxy(display, 1, y);
     while(leng){
         lcd_putc(display, *string++);
         leng--; printed++;
         if(printed>=len){ y++; printed=0; lcd_gotoxy(display, 1, y); }
         if(!(*string))break;
     }
}

void textscrollb(u_int16 display, char *text, u_int16 line, u_int16 nmbrchrs, u_int16 *scposi){   // textscrollb - scrolls text in one line
     u_int16 i, pos, aux, l;
     l = strlen(text);
     pos=*scposi; i=0;
     lcd_gotoxy(display, 1, line);
     while(i<nmbrchrs){
         aux = text[pos+i];
         if(!aux){
             aux = ' '; pos=0;
             lcd_putc(display, ' ');
         }
         lcd_putc(display, aux);
         i++;
     }
     if(!(text[*scposi]))(*scposi)=0;
     else if(l>nmbrchrs) (*scposi)++;
}

void textscroll(u_int16 display, char *text, u_int16 line, u_int16 nmbrchrs, u_int16 *scposi){   // textscroll - scrolls text in one line
     u_int16 i, j, pos, aux, l;
     l = strlen(text);
     pos=*scposi; i=0; j=0;
     lcd_gotoxy(display, 1, line);
     while(i<nmbrchrs){
         aux = text[pos+j]; j++;
         if(!aux){
             if(l<nmbrchrs)break;
             aux = ' '; pos=0; j=0;
             lcd_putc(display, ' '); i++; if(i>=nmbrchrs)break;
             lcd_putc(display, ' '); i++; if(i>=nmbrchrs)break;
         }
         lcd_putc(display, aux);
         i++;
     }
     if(!(text[*scposi]))(*scposi)=0;
     else if(l>nmbrchrs)(*scposi)++;
}

// eof

