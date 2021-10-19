#if defined(__STDC__) || defined(__cplusplus)
# define P_(s) s
#else
# define P_(s) ()
#endif


/* common.c */
int mode P_((int n));
int sendbrk P_((void));
int Txoff P_((void));
int Txon P_((void));
int read_modem P_((char *buf, int count));
void stalarm P_((unsigned int n));
void write_modem P_((char *buf, int len));
void flushinput P_((void));
int isbinary P_((char *name));
int ustrcmp P_((char *s1, char *s2));
int pathensure P_((char *name));
int pathrest P_((char *name, char *prev));
int makesubtree P_((char *name, char *prev));
int existd P_((char *name));
int existf P_((char *name));
int stsystem P_((char *cmd));
int stsleep P_((int n));
int initz P_((void));
int aexit P_((int n));
int buserr P_((void));
int addrerr P_((void));
int bttyout P_((int c));
void sendline P_((int c));
void flush_modem P_((void));
char *substr P_((char *s, char *t));
int canit P_((void));
void SetIoBuf P_((void));
void ResetIoBuf P_((void));
int readline P_((int timeout));
int report P_((int sct));
int lreport P_((long sct));
int _initargs P_((void));
int wr_modem P_((char *s));
unsigned char *dalloc P_((void));
int getbaud P_((void));

/* expandar.c */
int expandargs P_((int (*routine )(), int argc, char **argv));

/* fileio.c */
int stfopen P_((char *name, char *mode));
int stfclose P_((int handl));
int stputc P_((unsigned int c, int handl));
int stgetc P_((int handl));
int stflush P_((int handl));
int stfseek P_((int handl, long disp, int mode));
long stread P_((int handl, unsigned char *b, long bytes));

/* main.c */
void setFlow P_((void));
void setBaud P_((void));
void help P_((void));
int finish P_((void));

/* phone.c */
void phone P_((void));
int writedir P_((void));

/* rz.c */
int rusage P_((void));
int wcreceive P_((int argc, char **argp));
int wcrxpn P_((char *rpn));
int wcrx P_((void));
int wcgetsec P_((char *rxbuf, int maxtime));
int procheader P_((char *name));
int putsec P_((unsigned char *buf, int n));
void chkinvok P_((char *s));
int tryz P_((void));
int rzfiles P_((void));
int rzfile P_((void));
void zmputs P_((char *s));
int closeit P_((long rxbytes));
void ackbibi P_((void));
int sys2 P_((char *s));
void exec2 P_((char *s));
void touch P_((char *name, unsigned int *timep));

/* sz.c */
void onintr P_((void));
int wcsend P_((int argc, char *argp[]));
int wcs P_((char *oname));
int wctxpn P_((char *name));
int getnak P_((void));
int wctx P_((long flen));
int wcputsec P_((char *buf, int sectnum, int cseclen));
int filbuf P_((unsigned char *buf, int count));
int zfilbuf P_((void));
int fooseek P_((int fptr, long pos, int whence));
int readock P_((int timeout));
int susage P_((void));
int getzrxinit P_((void));
int sendzsinit P_((void));
int zsendfile P_((char *buf, int blen, long szbytes));
int zsendfdata P_((void));
int getinsync P_((int flag));
void saybibi P_((void));
int zsendcmd P_((char *buf, int blen));
void schkinvok P_((char *s));

/* transfer.c */
void transfer P_((void));
int find_command P_((char *s));
int expnd_args P_((char *s, int expand_wild));
int add_argv P_((char *s));
int handl_wild P_((char *s));
char *mkpathname P_((char *spec, char *file));
void free_args P_((void));
int rm P_((int argc, char **argv));
int yesno P_((char *p1, char *p2));
int cp P_((int argc, char **argv));
int cpy P_((char *src, char *dest));
int ls P_((int argc, char **argv));
void lis P_((char *wild));
int cd P_((int argc, char **argv));
int pwd P_((void));
int df P_((int argc, char **argv));
int rd P_((int argc, char **argv));
char *myalloc P_((unsigned int size));
int hhelp P_((void));
char *alltolower P_((char *s));
char *basename P_((char *s));
int doszf P_((int argc, char **argv));
void Cconraux P_((char *l));
int get_a_c P_((void));

/* tyme.c */
unsigned long tm_to_time P_((unsigned int base_year, unsigned int year, unsigned int month, unsigned int day, unsigned int hours, unsigned int mins, unsigned int secs));
unsigned long st2unix P_((unsigned int time, unsigned int date));
void unix2st P_((unsigned long Unix, unsigned int *time, unsigned int *date));

/* util.c */
void rd_time P_((void));
void my_screen P_((void));
void his_screen P_((void));
void hit_key P_((void));
long filesize P_((char *name));
void Bconws P_((char *s));
int *aaddress P_((void));
void hi50 P_((void));
void hi25 P_((void));
int *aaddress P_((void));

/* zm.c */
void zsbhdr P_((int len, int type, char *hdr));
void zsbh32 P_((int len, char *hdr, int type, int flavour));
void zshhdr P_((int len, int type, char *hdr));
void zsdata P_((char *buf, int length, int frameend));
void zsda32 P_((char *buf, int length, int frameend));
int zrdata P_((char *buf, int length));
int zrdat32 P_((char *buf, int length));
void garbitch P_((void));
int zgethdr P_((char *hdr, int eflag));
int zrbhdr P_((char *hdr));
int zrbhd32 P_((char *hdr));
int zrhhdr P_((char *hdr));
void zputhex P_((int c));
void zsendline P_((int c));
int zgethex P_((void));
int zgeth1 P_((void));
int zdlread P_((void));
int noxrd7 P_((void));
void stohdr P_((long p));
long rclhdr P_((char *hdr));
void zsdar32 P_((char *buf, int length, int frameend));
int zrdatr32 P_((char *buf, int length));

void bibis P_((int n));
void bibi P_((int n));

#undef P_
