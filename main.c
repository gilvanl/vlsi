//  'Mess' player - a simple player project just for fun.
//  Sources made from the project 'playfiles' in Vlsi Solution examples.
//  Plays directly from SD drive (only SD not USB).
//  Uses board 'VS1005 Breakout Board' and additional circuits, for details see docs.

#include <vo_stdio.h>
#include <stdio.h>
#include <volink.h>     /* Linker directives like DLLENTRY */
#include <apploader.h>  /* RunLibraryFunction etc */
#include <timers.h>
#include <libaudiodec.h>
#include <vsostasks.h>
#include <consolestate.h>
#include <ctype.h>
#include <uimessages.h>
#include <string.h>
#include <stdlib.h>
#include <audio.h>
#include <aucommon.h>
#include <sysmemory.h>
#include <vs1005g.h>
#include <vo_gpio.h>
#include <vstypes.h>
#include <kernel.h>
#include "lcdvs.h"
#include "keyb.h"
#include "id3.h"

#define  GPIO_WritePort(port,mask)  ( USEY(port) = (mask) )   // set gpio port to specified value

#define IDX_SIZE 512
#define NAMELENGTH 100

static FILE *fp = NULL;
static FILE *fps = NULL;
void *decoderLibrary = NULL;
AUDIO_DECODER *audioDecoder = NULL;
u_int16 __mem_y *idx=NULL;
char *errorString = "";
int eCode = 0;
static int fileNum = 0, files = 0;
static int sfileNum = 0, entries = 0;
int running = 1;
int pause = 0;
static char mesbuf[22];    // auxiliar string for messages in the displays
static char dirnam[22];
static char tmpnam[22];
static char fnumMode[16];
static char fnambuf[256];  // string name of the file in use
static char fnambufb[256];  // string for backup of file name
char TIT[FRLEN+2];  // opus / music / songtitle / workname
char ALB[FRLEN+2];  // album
char PERF[FRLEN+2]; // performer, player, band, singer
char D4T[12];       // date of issued album
char TRK[12];       // number track in album
static u_int16 scpos=0;    // position for textscroll
static u_int16 selmusinf=0; 
static u_int16 selscr=0;

void PlayerThread(void) {
      eCode = DecodeAudio(decoderLibrary, audioDecoder, &errorString);
}

void volume_up(void){
      s_int16 t = ioctl(stdaudioout, IOCTL_AUDIO_GET_VOLUME, NULL)-256;
      if (t >= -256) {
            if (t > 0) {  t--;  }
            ioctl(stdaudioout, IOCTL_AUDIO_SET_VOLUME, (void *)(t+256));
            printf("~%04x=%d\n", UIMSG_S16_SET_VOLUME, t);
            sprintf(mesbuf, "-%ddB",t/2);
            lcdmsg(2, mesbuf, 7, 7, 1);
            printf(mesbuf);
      }
}

void volume_down(void){
      s_int16 t = ioctl(stdaudioout, IOCTL_AUDIO_GET_VOLUME, NULL)-256;
      if (t >= -256) {
            if (t < 100) {  t++;  }
            ioctl(stdaudioout, IOCTL_AUDIO_SET_VOLUME, (void *)(t+256));
            printf("~%04x=%d\n", UIMSG_S16_SET_VOLUME, t);
            sprintf(mesbuf, "-%ddB",t/2);
            lcdmsg(2, mesbuf, 7, 7, 1);
            printf(mesbuf);
      }
}

void RunDir(void){
        *idx = IDX_SIZE;  // do not alter this
        files = RunLibraryFunction("DIR", ENTRY_1, (int)idx);  // idx seems to need to be passed as parameter
        if(files){
              sprintf(mesbuf, "%d", files);
              lcdmsg(1, mesbuf, 3, 18, 4);
              fileNum = 0;
        }
}

void showmusinfo(u_int16 sel){
       scpos=0;  selmusinf=sel;
       if(selmusinf>4)selmusinf=0;
       if(selmusinf){ lcd_cleanline(2,2); fnambuf[FRLEN]=0; }
       switch(selmusinf){
             case 1 : sprintf(fnambuf, "title: %s", TIT); lcdmsgl(1, TIT, 20, 1);  break;
             case 2 : sprintf(fnambuf, "artist: %s", PERF); lcdmsgl(1, PERF, 20, 1); break;
             case 3 : sprintf(fnambuf, "album: %s", ALB); lcdmsgl(1, ALB, 20, 1);  break;
             case 4 : sprintf(fnambuf, "%s", fnambufb); lcdmsgl(1, fnambuf, 20, 1); break;
            default :
                lcd_clear(1);
                lcdmsg(1, TIT, 20, 1, 1);
                lcdmsg(1, PERF, 20, 1, 2);
                lcdmsg(1, ALB, 20, 1, 3);
                lcdmsg(1, D4T, 4, 1, 4);
                lcdmsg(1, TRK, 5, 6, 4);
            break;
       }
}

void showpscreen(void){
             printf(" %s ", fnambuf);
             if(seekid3tag(fnumMode, TIT, ALB, PERF, D4T, TRK)){
                 showmusinfo(0);
             }
             else if(getvorbistag(fnumMode, "TITLE",  TIT, FRLEN)){
                 getvorbistag(fnumMode, "ARTIST",PERF, FRLEN);
                 getvorbistag(fnumMode, "ALBUM",  ALB, FRLEN);
                 getvorbistag(fnumMode, "DATE", D4T, 6);
                 getvorbistag(fnumMode, "TRACKNUMBER", TRK, 6);
                 showmusinfo(0);
             }
             else {  lcdmsgl(1, fnambuf, 20, 1);  }
             sprintf(mesbuf, "%02d/%02d ",fileNum+1, files);
             lcdmsg(1, mesbuf, 7, 15, 4);
}

void showpinfoscreen(void){
     switch(selscr){
           case 1 : break;
          default : textscroll(2, fnambuf, 2, 16, &scpos); break;
     }

}

ioresult PlayFile(void) {
  s_int32 lastSeconds = -1, newSeconds;
  s_int32 lastPerCent = -1;
  s_int32 newPerCent = -1;
  static s_int16 cmdLen = -1;
  int hadAnythingToDo = 0;
  u_int16 press=0;
  audioDecoder = CreateAudioDecoder(decoderLibrary, fp, stdaudioout, NULL, auDecFGuess);
  if (!audioDecoder) {
       printf("fail! createAuDec\n");
       return S_ERROR;
  }
  audioDecoder->pause = pause;
  audioDecoder->cs.fastForward = 1;
  StartTask(TASK_DECODER, PlayerThread);
  key=0; Delay(100); key=0;
  // below - while decoder task running:
  while (pSysTasks[TASK_DECODER].task.tc_State && pSysTasks[TASK_DECODER].task.tc_State != TS_REMOVED) {
    newSeconds = audioDecoder->cs.playTimeSeconds;
    if (newSeconds != lastSeconds && newSeconds >= 0 && (cmdLen < 0 || !(appFlags & APP_FLAG_ECHO))) {
       newPerCent = (audioDecoder->cs.Tell(&audioDecoder->cs)/(audioDecoder->cs.fileSize/100));
       hadAnythingToDo = 1;
       lastSeconds = newSeconds;   // lastSeconds-time played in seconds
       if (newPerCent != lastPerCent) {
            lastPerCent = newPerCent;   // newPerCent -actual percentual of file that has been played
            sprintf(mesbuf, "%02ld%%  ", newPerCent);
            lcdmsg(2, mesbuf, 5, 7, 1);
       }
       sprintf(mesbuf, "%02ld:%02ld ",lastSeconds/60,lastSeconds%60);
       lcdmsg(2, mesbuf, 6, 1, 1);
       showpinfoscreen();
    }  // end if (newSec..
    keybr();
    if(!key)press=0;
    if(!press){
       switch(key){
           case UP   : volume_up();  printf(" Up ");  break;
           case DOWN : volume_down(); printf(" Down ");  break;
           case RIGHT :
                audioDecoder->cs.cancel = 1;
                audioDecoder->pause = 0;
           break;
           case LEFT :
                fileNum -= 2;
                audioDecoder->cs.cancel = 1;
                audioDecoder->pause = 0;
                if(fileNum<-1)fileNum=-1;
           break;
           case ALT  : printf(" Alt ");   break; // out of hardware
           case MENU : selscr++; if(selscr>1)selscr=0;  break;
           case ENTER:
                pause = !pause;
                if(pause){
                     lcdmsg(1, "pause ", 6, 1, 1);
                     lcdmsg(2, "pause ", 6, 1, 1);
                }
                audioDecoder->pause = pause && !audioDecoder->cs.cancel;
           break;
           case ESC  : showmusinfo(++selmusinf);  break;
           case STOP :
                lcdmsg(1, "STOP  ", 6, 1, 1);
                lcdmsg(2, "STOP  ", 6, 1, 1);
                audioDecoder->cs.cancel = 1;
                audioDecoder->pause = 0;
                running = 0;
           break;
           default : break;
       }  // end switch(k3y
       if(key){  press=3;  }
    }  // end if(!pres
    else {  press++; Delay(5); if(press>150)press=0;  }
    if (appFlags & APP_FLAG_QUIT) {
         audioDecoder->cs.cancel = 1;
         audioDecoder->pause = 0;
         running = 0;
    }
    if (!hadAnythingToDo){  Delay(5);  }
  }    // end while decoder task running
  printf("\nP-r-cod:%d %s \n", eCode, errorString);
  DeleteAudioDecoder(decoderLibrary, audioDecoder);
  return S_OK;
}

ioresult playfiles(void){
   u_int16 found;
   int playErr = S_ERROR;
   running=1;
   RunDir();
   found=0;
   do{
        sprintf(fnumMode,"rb#%d", idx[fileNum]-1);
        fp = fopen("*",fnumMode);
        if(fp){
             sprintf(fnambuf, "%s", fp->Identify(fp, NULL, 0));
             if(!strncmp(fnambufb, fnambuf, 50)){
                found=7;
             }
             else fileNum++;
             fclose(fp);
        }
        if(fileNum>=files){ fileNum=0; found=7; }
   }while(!found);
   while(running){
        lcd_clear(1); lcd_clear(2);
        if(appFlags & APP_FLAG_QUIT){  break;  }
        sprintf(mesbuf, "%02d/%02d ",fileNum+1, files);
        lcdmsg(2, mesbuf, 6, 12, 1); lcdmsg(1, mesbuf, 6, 14, 1);
        printf("\nFile nr. %d: ", fileNum+1);
        sprintf(fnumMode,"rb#%d", idx[fileNum]-1);
        fp = fopen("*",fnumMode);
        if(fp){
             scpos=0;
             sprintf(fnambuf, "%s", fp->Identify(fp, NULL, 0));
             sprintf(fnambufb, "%s", fnambuf);
             showpscreen();
             playErr = PlayFile();
             fclose(fp);
             if (playErr) {  running = 0;  }
             fileNum++;
             if (fileNum >= files) {  running = 0;  }
        }   // end if(fp..
        else {  running = 0;  }
   }  // end while(running...
   lcd_clear(1); lcd_clear(2);
   lcdmsg(2, "End play", 8, 1, 2);
   if(playErr){ sprintf(mesbuf, "err:%d ", playErr); lcdmsg(2, mesbuf, 7, 1, 1); }
   printf("\nplayErr:%d ", playErr); Delay(1000); key=0;
   return S_OK;
}

////////////////////     File Browsing Functions     ////////////////////
void showfscreen(void){
         lcd_clear(1); lcd_clear(2);
         lcdmsg(1, "Dir: ", 5, 1, 1);  lcdmsg(1, dirnam, 15, 6, 1);
         lcdmsg(2, "Dir: ", 5, 1, 1);  lcdmsg(2, dirnam, 15, 6, 1);
         if(fps->ungetc_buffer == __ATTR_VOLUMEID)lcdmsg(1, "Vol: ", 5, 16, 1);
         if(fps->ungetc_buffer >= __ATTR_DIRECTORY)lcdmsg(1,"SDir:", 5, 16, 1);
         if(fps->ungetc_buffer >= __ATTR_ARCHIVE)lcdmsg(1,  "File:", 5, 16, 1);
         lcdmsgl(1, fnambuf, 20, 2);  lcdmsg(2, fnambuf, 16, 1, 2);
}

static u_int16 fpsop=0;

int SetDir(void) {
    ioresult retCode = S_ERROR;
    sfileNum=0; entries=0;
    if(fpsop){  fclose(fps); fpsop=0; }
    fps = fopen(currentDirectory, "s");
    if(fps){
          fpsop=1;
          if(FatFindFirst(fps, currentDirectory+2, fnambuf, NAMELENGTH) == S_OK){
               printf("\n %02X  %-12s  %s ",  fps->ungetc_buffer, fps->extraInfo, fnambuf);  //attribute - shortname - longname
               strncpy(dirnam,fps->extraInfo,20);
               while(S_OK == FatFindNext(fps, fnambuf, NAMELENGTH)){
                    entries++;
               }
               FatFindFirst(fps, currentDirectory+2, fnambuf, NAMELENGTH);
               printf("  entries:%d \n", entries); printf("\n %s ",dirnam);
               showfscreen();
               retCode = S_OK;
          }
          else {  printf("\nCant open entry %s ", currentDirectory+2);  }
    }
    else {  printf("\nCant open dir %s ", currentDirectory);  }
    return retCode;
}

void GoNext(void){
    if(S_OK == FatFindNext(fps, fnambuf, NAMELENGTH)){
         if(sfileNum<=entries)sfileNum++;
         printf("\n%d %s %XH\n", sfileNum, fnambuf, fps->ungetc_buffer);
         showfscreen();
    }
    else printf("  end_dir:%d  ", sfileNum);
}

void GoAhead(void){
     u_int16 i;
     for(i=0;i<5;i++){
          if(S_OK == FatFindNext(fps, fnambuf, NAMELENGTH)){
               if(sfileNum<=entries)sfileNum++;
               else i=7;
          }
     }
     showfscreen();
}

void GoPrev(void){
    int n=0;
    n = sfileNum-1; if(n<0)n=0; sfileNum=0;
    if(FatFindFirst(fps, currentDirectory+2, fnambuf, NAMELENGTH) == S_OK) {
          sfileNum=0;
          while(sfileNum<n){
               if(S_OK == FatFindNext(fps, fnambuf, NAMELENGTH)){ sfileNum++; }
          }
          printf("\n%d %s %XH\n",  sfileNum, fnambuf, fps->ungetc_buffer);
          showfscreen();
    }
}

void GoInto(void){
    if( (fps->ungetc_buffer==__ATTR_DIRECTORY) && (fnambuf[0]!='.') ){
          if(!RunProgram("cd", fnambuf)){
               strncpy(tmpnam,fnambuf,20);
               SetDir();
               strncpy(dirnam,tmpnam,20);
               showfscreen();
          }
    }
    else {
          if(fnambuf[0]=='.')printf("\nAlready here! ");
          else printf("\n%XH Not Dir ", fps->ungetc_buffer);
    }
}

void GoUp(void){
    if(!RunProgram("cd", ".."))SetDir();   // zero return is success
}

void fbrowse(void){
     u_int16 press=0;
     lcd_clear(1); lcd_clear(2);
     SetDir();
     do{
          keybr();
          if(!key)press=0;
          if(!press){
              switch(key){
                  case LEFT  :  GoPrev(); break;
                  case RIGHT :  GoNext(); break;
                  case UP    :  GoUp();   break;
                  case DOWN  :
                  case ENTER :
                     if(fps->ungetc_buffer>=__ATTR_ARCHIVE){
                          sprintf(fnambufb, "%s", fnambuf);
                          playfiles(); SetDir();
                     }
                     else GoInto();
                  break;
                  case ALT   :  break;
                  case ESC   :  GoAhead(); break;
                  case STOP  :  break;
                  case MENU  :  break;
                  default    :  break;
              }  // end switch(k3y
              if(key){  press=3;  }
          }  // end if(!pres
          else {  press++; Delay(5); if(press>150)press=0;  }
     }while(key!=STOP);
}
////////////////////     File Browsing Functions end    ////////////////////

//  main function
ioresult main(char *parameters) {
   int retCode = S_ERROR;
   GPIO_WritePort(GPIO0_MODE, 0x02000);  // bit 13 - spdif output
   GPIO_WritePort(GPIO0_DDR, 0x0FF);  // up to bit 8 set to output mode
   lcd_init(1, GPIO0_ODATA);
   lcd_init(2, GPIO0_ODATA);
   lcdmsg(1, "VLSI VS1005G ", 13, 1, 1);
   lcdmsg(1, "Break-out Board", 16, 1, 2);
   lcdmsg(1, "Full Mess Player", 16, 1, 3);
   lcd_brighness_set(2, 0x03);  // minimum brightness in Noritake displays is issued by number 3
   lcdmsg(2, " VLSI  VS1005G ", 15, 1, 1);
   lcdmsg(2, "Full Mess Player", 16, 1, 2);
   initkeyboard();
   idx = callocy(sizeof(*idx), IDX_SIZE);  // do not alter this
   if (!idx) {
        printf("!outOfMem\n");
        lcdmsg(1, "outOfMem! ", 10, 1, 4);
        goto finally;
   }  // end do not alter
   decoderLibrary = LoadLibrary("audiodec");
   if (!decoderLibrary) {
        printf("!decLib\n");
        lcdmsg(1, "!decLib  ", 10, 1, 4);
        goto finally;
   }
   Delay(2500);
   // disabled - (new way in line below) -  strcpy(currentDirectory,"D:"); // set drive here
   if(RunProgram("cd", "D:")){  printf("\nFailing open SD drive in D:\ \n"); return S_ERROR;  }
   sprintf(currentDirectory, "D:");
   fbrowse(); // here browse files and play them
   finally:
   if (decoderLibrary){  DropLibrary(decoderLibrary);  }
   if (idx){  freey(idx);  }
   if(retCode){ sprintf(mesbuf, "%d ", retCode); lcdmsg(2, mesbuf, 4, 1, 2); }
   printf("\nret:%d \n", retCode);
   return retCode;
}  // end main(...



