#include <stdio.h>

void *Setexc(short vnum,void *vptr) { 
    puts("Setexc() is not implemented\n");
}
short Bconin(short dev) { 
    puts("Bconin() is not implemented\n");
    return 0;
}
void  Bconout(short dev, short c) { 
    puts("Bconout() is not implemented\n");
}
short Bconstat(short dev) { 
    puts("Bconstat() is not implemented\n");
    return 0;
}
long Bcostat(short dev) { 
    puts("Bostat() is not implemented\n");
    return 0;
}
long Dcreate(const char *path) {
    puts("Dcreate() is not implemented\n");
    return 0;
}
long Drvmap(void) { 
    puts("Drvmap() is not implemented\n");
}
short Fattrib(const char *filename, short wflag, short attrib) { 
    puts("Fattrib() is not implemented\n");
    return 0;
}
short Fcreate(const char *fname, short attr) { 
    puts("Fcreate() is not implemented\n");
    return 0;
}
short Fclose(short handle) { 
    puts("Fclose() is not implemented\n");
    return 0;
}
void Fdatime(void *timeptr, short handle, short wflag) { 
    puts("Fdatime() is not implemented\n");
    return 0;
}
long Fopen(const char *fname, short mode) { 
    puts("Fopen() is not implemented\n");
    return 0;
}
long Fread(short handle, long count, void *buf) {
    puts("Fread() is not implemented\n");
    return 0;
}
long Fseek(long offset, short handle, short seekmode) { 
    puts("Fseek() is not implemented\n");
    return 0;
}
void Fsetdta(void *buf) { 
    puts("Fsetdta() is not implemented\n");
}
long Fsfirst(const char *filename, short attr) { 
    puts("Fsfirst() is not implemented\n");
    return 0;
}
long Fwrite(short handle, long count, void *buf) { 
    puts("Fwrite() is not implemented\n");
    return 0;
}
long gemdos(short func, ...) { 
    printf("gemdos() is not implemented\n");
    return 0;
}
short Getrez(void) { 
    puts("Getrez() is not implemented\n");
    return 0;
}
void *Iorec(short dev) { 
    puts("Iorec() is not implemented\n");
    return NULL;
}
void *Malloc(long number) { 
    puts("Malloc() is not implemented\n");
    return NULL;
}
long Mfree(void *block) { 
    puts("Mfree() is not implemented\n");
    return 0;
}
long Pexec(short mode, ...) { 
    puts("Pexec() is not implemented\n");
    return 0;
}
void Pterm(short retcode) { 
    puts("Pterm() is not implemented\n");
}
long Rsconf(short baud, short ctl, short ucr, short rsr, short tsr, short scr) { 
    puts("Rsconf() is not implemented\n");
}
long Super(void *stack) { 
    puts("Super() is not implemented\n");
    return 0;
}
long xbios(short func, ...) { 
    puts("xbios() is not implemented\n");
    return 0;
}

