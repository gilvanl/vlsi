// id3.h  - functions for extract info from id3 tags
#ifndef __ID3H_H__
#define __ID3H_H__

#include <vstypes.h>

#define FRLEN 60    // frame length - max size of a id3 text string

int seekid3tag(const char *fnam3, char *tit, char *alb, char *perf, char *d4t, char *trk); // seeks for id3 tag in a buffer
int getvorbistag(const char *fnam3,  const char *vrbt, char *dest, unsigned mdigi);   // seeks for vorbis tags in a file
int getid3frame(const char *frame, char *sourc, char *dest, unsigned mdigi);  // extracts an id3 frame from a buffer
int getid3tit(char *source, char *dest);   // returns the title, song, opus name
int getid3alb(char *source, char *dest);   // returns the album name
int getid3perf(char *source, char *dest);  // returns the performer, band, singer, player, orchestra
int getid3dat(char *source, char *dest);   // returns the album date
int getid3trk(char *source, char *dest);   // returns the track number

#endif   // __ID3H_H__
