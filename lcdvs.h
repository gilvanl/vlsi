/**************************************************************************
*  File     :  lcd4s.h                                                    *
*  Summary  :  Arquivo de funcoes para LCD em processadores VLSI VS1005   *
*  Version  :  1.0                                                        *
*  Date     :  25.10.2020                                                 *
*  Author   :  Gilvan Luiz Latreille                                      *
*  Compiler :  VLSI VSIDE 2.47                                            *
***************************************************************************
***************************************************************************
*  Hardware Environment:                                                  *
*   CPU: VLSI VS1005G                                                     *
*   Placa de desenvolvimento VLSI VS1005 BREAKOUT BOARD  v1.0.5           *
*  Description:                                                           *
*   Funcoes em geral para LCD's do tipo Hitachi HD44780 e similares.      *
*   Usa interface 4 bit,                                                  *
**************************************************************************/

#ifndef _lcdvs_h_
#define _lcdvs_h_ true

#include <vstypes.h>   // contem definicoes de tipos e macros: typedef unsigned short u_int16; USEY

//INTERFACE DEFINITIONS:
//#define two_lines true
 #define four_lines true


//ADDRESS OF LCD LINE FIRST POSITIONS :
//You must change the values below to match the addresses of the LCD lines that you are using:
#ifdef four_lines      //if using four lines lcd:
   #define lcd_line_two   0x40    // LCD RAM address for the start of the second line 0x40
   #define lcd_line_three 0x14    // LCD RAM address for the start of the third  line 0x10
   #define lcd_line_four  0x54    // LCD RAM address for the start of the fourth line 0x50
#else                  //if using two lines lcd:
   #define lcd_line_two   0x40    // LCD RAM address for the second line
   #define lcd_line_three 0x40
   #define lcd_line_four  0x40
#endif

// sets brightness in Noritake vfd displays 
#define  lcd_brighness_set(display, level)  lcd_send_command(display, 0x22);  lcd_putc(display, level);

//FUNCTION DEFINITIONS:
void lcd_init(u_int16 display, u_int16 porta);
void lcd_putc(u_int16 display, u_int16 datachar);
void lcd_send_command(u_int16 display, u_int16 comm);
void lcd_clear(u_int16 display);
void lcd_gotoxy(u_int16 display, u_int16 x, u_int16 y);
void lcd_cleanline(u_int16 display, u_int16 lin);
void lcd_puts(u_int16 display, const char *string);
void lcdmsg(u_int16 display, char *string, u_int16 len, u_int16 x, u_int16 y);
void lcdmsgl(u_int16 display, char *string, u_int16 len, u_int16 y);  // prints messages in several lines
void textscroll(u_int16 display, char *text, u_int16 line, u_int16 positions, u_int16 *scposi);  // textscroll - faz um texto longo ser movimentado em uma linha do display lcd
void textscrollb(u_int16 display, char *text, u_int16 line, u_int16 nmbrchrs, u_int16 *scposi);
#endif  // _lcdvs_h_
