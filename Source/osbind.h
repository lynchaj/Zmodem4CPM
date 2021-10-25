
void *Setexc(short vnum,void *vptr);
short Bconin(short dev);
void  Bconout(short dev, short c);
short Bconstat(short dev);
long Bcostat(short dev);

long Dcreate(const char *path);
long Drvmap(void);
short Fattrib(const char *filename, short wflag, short attrib);
short Fcreate(const char *fname, short attr);
short Fclose(short handle);
void Fdatime(void *timeptr, short handle, short wflag);
long Fopen(const char *fname, short mode);
long Fread(short, long count, void *buf);
long Fseek(long offset, short handle, short seekmode);
void Fsetdta(void *buf);
long Fsfirst(const char *filename, short attr);
short Fclose(short handle);
long Fwrite(short handle, long count, void *buf);

long gemdos(short func, ...);

short Getrez(void);

void *Iorec(short dev);

void *Malloc(long number);
long Mfree(void *block);

long Pexec(short mode, ...);
void Pterm(short retcode);

long Rsconf(short baud, short ctl, short ucr, short rsr, short tsr, short scr);

long Super(void *stack);

long xbios(short func, ...);
