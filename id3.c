// id3.c  - functions for extract info from id3 and vorbis tags

#include <sysmemory.h>
#include <stdio.h>
#include <vo_stdio.h>
#include <ctype.h>
#include "id3.h"
#include "lcdvs.h"

#define IDSALLOC 512
static char *srce=NULL;

int seekid3tag(const char *fnam3, char *tit, char *alb, char *perf, char *d4t, char *trk){ // seeks for id3 tag in a buffer
    unsigned char c;
    unsigned i,j;
    FILE *fp;
    unsigned char id3[]="ID3";
    fp = fopen("*",fnam3);
    if(!fp){ return 0; }
    j=0;
    for(i=0;i<8000;i++){
        c = fgetc(fp);
        if(c==id3[j])j++; else j=0;
        if(j>2)break;
    }
    if(j<3){
        fclose(fp);
        lcdmsg(1, "No ID3 info ", 12, 1, 1);
        return 0;
    }
    srce = calloc(IDSALLOC, sizeof(srce));
    if(!srce){ printf(" \n No ID3 Mem!\n"); return 0;  }
    lcdmsg(1, "ID3 found   ", 12, 1, 1);
    for(i=0;i<IDSALLOC;i++){ c = fgetc(fp); srce[i] = c;  }
    getid3tit(srce, tit);
    getid3alb(srce, alb);
    getid3perf(srce, perf);
    getid3dat(srce, d4t);
    getid3trk(srce, trk);
    free(srce);
    fclose(fp);
    return 1;
}

int getvorbistag(const char *fnam3,  const char *vrbt, char *dest, unsigned mdigi){ // seeks for vorbis tags in a file
    unsigned char c;
    unsigned i,j;
    FILE *fp;
    for(i=0;i<(mdigi-2);i++){ dest[i]=0; }
    fp = fopen("*",fnam3);
    if(!fp){ return 0; }
    for(i=0;i<8000;i++){
        c = fgetc(fp);
        if(c==vrbt[j])j++; else j=0;
        if(vrbt[j]==0)break;
    }
    if(j<4){ fclose(fp); return 0; }
    i=0;
    while(c!='='){   // 61d  0x3D
        c = fgetc(fp);  i++;
        if(i>20){ fclose(fp); return 0; }
    }
    for(i=0;i<mdigi;i++){
        c = fgetc(fp);
        if(c<32)break;  // minor than space char
        dest[i]=c;
    }
    fclose(fp);
    dest[i]=0;
    printf("\n%s ", dest);
    return 1;
}

int getid3frame(const char *frame, char *sourc, char *dest, unsigned mdigi){  // extracts an id3 frame from a buffer
    unsigned t, frameSize, i, j=0;
    unsigned char encoding;
    for(i=0;i<512;i++){ // search in buffer for string passed in *frame
        if(sourc[i]==frame[j])j++; else j=0;
        if(j>3)break; // frame encontrado
    }
    if(j<4)return 0; // frame not found
    i+=4;  frameSize=sourc[i];    // gets the size of frame
    if(frameSize>(mdigi-1))frameSize=(mdigi-1);
    t=1;  if(frameSize>2)t=frameSize-2;
    i+=3;  encoding = sourc[i];  // gets the encoding code
    if(encoding==1){ // unicode utf16 w/bom 2 bytes
        i+=3; j=0; t/=2;
        while(j<frameSize){  // loads the found frame into array
             if(j>=t){
                  if(sourc[i]=='T')break;
                  if(sourc[i]=='C')break;
                  if(sourc[i]=='P')break;
                  if(sourc[i]=='M')break;
                  if(sourc[i]=='A')break;
             }
             dest[j]=sourc[i];
             j++; i+=2;
        }
    }
    else { // normal ascii and utf8
        i++; j=0;
        while(j<frameSize){  // loads the found frame into array
             if(j>=t){
                  if(sourc[i]=='T')break;
                  if(sourc[i]=='C')break;
                  if(sourc[i]=='P')break;
                  if(sourc[i]=='M')break;
             }
             dest[j]=sourc[i];
             j++; i++;
        }
    }
    dest[j]=0;     // ends array with zero
    printf("\n%s ", dest);
    return 1;
}

int getid3tit(char *source, char *dest){   // returns the title, song, opus name
    int r;
    r=getid3frame("TIT2", source, dest, FRLEN);
    if(!r)r=getid3frame("TIT3", source, dest, FRLEN);
    if(!r)r=getid3frame("TSOT", source, dest, FRLEN);
    return r;
}

int getid3alb(char *source, char *dest){   // returns the album name
    int r;
    r=getid3frame("TALB", source, dest, FRLEN);
    if(!r)r=getid3frame("TOAL", source, dest, FRLEN);
    if(!r)r=getid3frame("TSOA", source, dest, FRLEN);
    return r;
}

int getid3perf(char *source, char *dest){   // returns the performer, band, singer, player, orchestra
    int r;
    r=getid3frame("TPE1", source, dest, FRLEN);
    if(!r)r=getid3frame("TPE2", source, dest, FRLEN);
    if(!r)r=getid3frame("TPE3", source, dest, FRLEN);
    if(!r)r=getid3frame("TOPE", source, dest, FRLEN);
    if(!r)r=getid3frame("TSOP", source, dest, FRLEN);
    return r;
}

int getid3dat(char *source, char *dest){   // returns the album date
    int r;
    r=getid3frame("TYER", source, dest, 6);  // year
    if(!r)r=getid3frame("TORY", source, dest, 6);
    if(!r)r=getid3frame("TDOR", source, dest, 6);
    if(!r)r=getid3frame("TDRC", source, dest, 6);
    return r;
}

int getid3trk(char *source, char *dest){   // returns the track number
    int r;
    r=getid3frame("TRCK", source, dest, 6);  // track number in album
    return r;
}


