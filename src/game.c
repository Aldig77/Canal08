/* ============================================================
   CANAL 0 : NOCHE DE LLUVIA   -   Dreamcore / VHS para GBA
   C puro, sin librerias. Modo 4 (240x160, 256 colores) con
   buffer en EWRAM, efecto VHS aplicado al copiar a VRAM.
   ============================================================ */
#include <stddef.h>
#include "hw.h"
#include "game.h"

#define SW 240
#define SH 160

/* ---------------- paleta ---------------- */
enum { C_VOID=0,C_INK,C_PUR,C_PUR2,C_LAV,C_WHITE,C_YEL,C_YELD,C_BLUE,C_BLUED,C_SKIN,C_SKIND,
       C_HAIR,C_CREAM,C_CREAMD,C_WOOD,C_WOODD,C_RED,C_REDD,C_CYAN,C_CYAND,C_MAG,C_MAGD,
       C_GRN,C_GRND,C_GRAY,C_GRAYD,C_TV,C_PALEY,C_CORAL,C_LBLUE,C_BRIGHT };

static const u8 PALRGB[32][3]={
 {11,10,18},{28,24,48},{58,47,90},{107,91,149},{169,161,201},{242,238,230},{232,193,58},{184,138,30},
 {44,79,160},{27,48,112},{224,169,138},{181,122,94},{59,35,20},{216,208,176},{176,164,126},{122,74,42},
 {82,48,26},{192,57,43},{138,34,48},{60,199,192},{30,127,138},{217,79,160},{122,44,100},{76,175,80},
 {45,106,48},{143,143,154},{85,85,95},{42,42,53},{245,226,122},{255,111,97},{154,208,255},{255,255,255}};

#define RGB15(r,g,b) ((u16)(((r)>>3)|(((g)>>3)<<5)|(((b)>>3)<<10)))
static void pal_init(void){
    for(int i=0;i<32;i++){
        int r=PALRGB[i][0],g=PALRGB[i][1],b=PALRGB[i][2];
        PAL[i]=RGB15(r,g,b);
        PAL[64+i]=RGB15(r*3/4,g*3/4,b*4/5+6);   /* banco oscuro = lineas de barrido VHS */
    }
}

/* ---------------- utilidades ---------------- */
static u32 rs=12345;
static u32 rnd(void){ rs=rs*1664525u+1013904223u; return rs>>16; }
static int iabs(int a){ return a<0?-a:a; }
static int sgn(int a){ return a>0?1:(a<0?-1:0); }
static const s8 bobt[16]={0,1,1,2,2,2,1,1,0,-1,-1,-2,-2,-2,-1,-1};

/* ---------------- render ---------------- */
static u8 fb[SW*SH] __attribute__((aligned(4))) SBSS;
static u8 *tb=fb; static int tw=SW, th=SH;
static void settgt(u8*b,int w,int h){tb=b;tw=w;th=h;}
static void px(int x,int y,u8 c){ if((unsigned)x<(unsigned)tw&&(unsigned)y<(unsigned)th) tb[y*tw+x]=c; }
FAST static void rect(int x,int y,int w,int h,u8 c){
    int x1=x+w,y1=y+h; if(x<0)x=0; if(y<0)y=0; if(x1>tw)x1=tw; if(y1>th)y1=th;
    if(x>=x1||y>=y1) return;
    u32 cw=c*0x01010101u;
    for(int j=y;j<y1;j++){
        u8*p=tb+j*tw; int i=x;
        while(i<x1&&(i&3)){p[i++]=c;}
        while(i+4<=x1){*(u32*)(p+i)=cw;i+=4;}
        while(i<x1){p[i++]=c;}
    }
}
static void frame(int x,int y,int w,int h,u8 c){rect(x,y,w,1,c);rect(x,y+h-1,w,1,c);rect(x,y,1,h,c);rect(x+w-1,y,1,h,c);}
static void disc(int cx,int cy,int r,u8 c){
    for(int j=-r;j<=r;j++)for(int i=-r;i<=r;i++) if(i*i+j*j<=r*r+r/2) px(cx+i,cy+j,c);
}
static void blit(const u8*s,int sw,int sh,int x,int y,int sc){
    for(int j=0;j<sh;j++)for(int i=0;i<sw;i++){u8 c=s[j*sw+i]; if(c!=255){ if(sc==1)px(x+i,y+j,c); else rect(x+i*sc,y+j*sc,sc,sc,c);} }
}

/* ---------------- fuente 5x7 (solo mayusculas, estilo VHS) ---------------- */
static const char FCH[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?:;'\"-*()/+_<>=[]";
static const u8 FNT[][7]={
 {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11},{0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E},{0x0E,0x11,0x10,0x10,0x10,0x11,0x0E},
 {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E},{0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F},{0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},
 {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F},{0x11,0x11,0x11,0x1F,0x11,0x11,0x11},{0x0E,0x04,0x04,0x04,0x04,0x04,0x0E},
 {0x07,0x02,0x02,0x02,0x02,0x12,0x0C},{0x11,0x12,0x14,0x18,0x14,0x12,0x11},{0x10,0x10,0x10,0x10,0x10,0x10,0x1F},
 {0x11,0x1B,0x15,0x15,0x11,0x11,0x11},{0x11,0x19,0x15,0x13,0x11,0x11,0x11},{0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},
 {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10},{0x0E,0x11,0x11,0x11,0x15,0x12,0x0D},{0x1E,0x11,0x11,0x1E,0x14,0x12,0x11},
 {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E},{0x1F,0x04,0x04,0x04,0x04,0x04,0x04},{0x11,0x11,0x11,0x11,0x11,0x11,0x0E},
 {0x11,0x11,0x11,0x11,0x11,0x0A,0x04},{0x11,0x11,0x11,0x15,0x15,0x1B,0x11},{0x11,0x11,0x0A,0x04,0x0A,0x11,0x11},
 {0x11,0x11,0x0A,0x04,0x04,0x04,0x04},{0x1F,0x01,0x02,0x04,0x08,0x10,0x1F},
 {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},{0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},{0x0E,0x11,0x01,0x02,0x04,0x08,0x1F},
 {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E},{0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},{0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},
 {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},{0x1F,0x01,0x02,0x04,0x08,0x08,0x08},{0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
 {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},
 /* . , ! ? : ; ' " - * ( ) / + _ < > = [ ] */
 {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C},{0x00,0x00,0x00,0x00,0x0C,0x04,0x08},{0x04,0x04,0x04,0x04,0x04,0x00,0x04},
 {0x0E,0x11,0x01,0x02,0x04,0x00,0x04},{0x00,0x0C,0x0C,0x00,0x0C,0x0C,0x00},{0x00,0x0C,0x0C,0x00,0x0C,0x04,0x08},
 {0x04,0x04,0x08,0x00,0x00,0x00,0x00},{0x0A,0x0A,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x1F,0x00,0x00,0x00},
 {0x00,0x0A,0x04,0x1F,0x04,0x0A,0x00},{0x02,0x04,0x08,0x08,0x08,0x04,0x02},{0x08,0x04,0x02,0x02,0x02,0x04,0x08},
 {0x01,0x01,0x02,0x04,0x08,0x10,0x10},{0x00,0x04,0x04,0x1F,0x04,0x04,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x1F},
 {0x02,0x04,0x08,0x10,0x08,0x04,0x02},{0x08,0x04,0x02,0x01,0x02,0x04,0x08},{0x00,0x00,0x1F,0x00,0x1F,0x00,0x00},
 {0x04,0x00,0x04,0x04,0x04,0x04,0x04},{0x04,0x00,0x04,0x08,0x10,0x11,0x0E}
};
_Static_assert(sizeof(FNT)/7==sizeof(FCH)-1,"font table size");
static s8 fidx[128];

static void gl(u16 g,int x,int y,u8 c,int sc){
    int ch=g&255, ac=g>>8; if(ch>=128||ch==' '||ch==0) return;
    int k=fidx[ch]; if(k<0) return;
    for(int r=0;r<7;r++){ u8 bits=FNT[k][r];
        for(int b=0;b<5;b++) if(bits&(0x10>>b)) rect(x+b*sc,y+(r+1)*sc,sc,sc,c); }
    if(ac==1) rect(x+3*sc,y,sc,sc,c);
    else if(ac==2){ rect(x+1*sc,y,3*sc,sc,c); }
    else if(ac==3){ rect(x+1*sc,y,sc,sc,c); rect(x+3*sc,y,sc,sc,c); }
}
/* UTF-8 -> glifo (byte bajo = ASCII mayuscula, byte alto = acento) */
static const char* dec1(const char*p,u16*o){
    u8 c=(u8)*p++;
    if(c<0x80){ if(c>='a'&&c<='z')c-=32; *o=c; return p; }
    if(c==0xC3){ u8 d=(u8)*p++; u8 base=0,ac=0;
        switch(d){
         case 0xA1:case 0x81:base='A';ac=1;break; case 0xA9:case 0x89:base='E';ac=1;break;
         case 0xAD:case 0x8D:base='I';ac=1;break; case 0xB3:case 0x93:base='O';ac=1;break;
         case 0xBA:case 0x9A:base='U';ac=1;break; case 0xB1:case 0x91:base='N';ac=2;break;
         case 0xBC:case 0x9C:base='U';ac=3;break; }
        *o=(u16)(base|(ac<<8)); return p; }
    if(c==0xC2){ u8 d=(u8)*p++; *o= d==0xBF?']': d==0xA1?'[':0; return p; }
    while(((u8)*p&0xC0)==0x80)p++;
    *o=0; return p;
}
static int txt_n(const char*s){ int n=0; u16 g; while(*s){ s=dec1(s,&g); if(g)n++; } return n; }
static void txt(const char*s,int x,int y,u8 c,int sc){
    u16 g; while(*s){ s=dec1(s,&g); if(!g)continue; gl(g,x,y,c,sc); x+=6*sc; }
}
static void txtc(const char*s,int y,u8 c,int sc){ txt(s,(SW-txt_n(s)*6*sc)/2,y,c,sc); }
static void txt_shadow(const char*s,int x,int y,u8 c,int sc){ txt(s,x+1,y+1,C_INK,sc); txt(s,x,y,c,sc); }

/* ---------------- entrada ---------------- */
#define K_A 1
#define K_B 2
#define K_SEL 4
#define K_START 8
#define K_RIGHT 16
#define K_LEFT 32
#define K_UP 64
#define K_DOWN 128
static u16 keys,kp;
static void poll(void){ u16 k=(u16)((~REG_KEYINPUT)&0x3FF); kp=k&~keys; keys=k; }

/* ---------------- audio chiptune (PSG de la GBA) ---------------- */
enum { RR=0, C2=36,Cs2,D2,Ds2,E2,F2,Fs2,G2,Gs2,A2,As2,B2, C3,Cs3,D3,Ds3,E3,F3,Fs3,G3,Gs3,A3,As3,B3,
       C4,Cs4,D4,Ds4,E4,F4,Fs4,G4,Gs4,A4,As4,B4, C5,Cs5,D5,Ds5,E5,F5,Fs5,G5,Gs5,A5,As5,B5, C6 };
static const u16 HZ4[12]={262,277,294,311,330,349,370,392,415,440,466,494};
static u16 note_x(int m){ int o=m/12-5; int f=HZ4[m%12]; if(o>0)f<<=o; else if(o<0)f>>=-o; return (u16)(2048-131072/f); }

typedef struct { const u8*mel,*bas,*nz; u8 len,tempo,duty,mvol; } Song;
#define Z8 0,0,0,0,0,0,0,0
static const u8 TI_M[32]={A4,RR,C5,RR,E5,RR,C5,RR, D5,RR,F5,RR,E5,RR,RR,RR, C5,RR,E5,RR,G5,RR,E5,RR, B4,RR,D5,RR,A4,RR,RR,RR};
static const u8 TI_B[32]={A2,RR,A2,RR,A2,RR,A2,RR, D3,RR,D3,RR,E3,RR,E3,RR, C3,RR,C3,RR,C3,RR,C3,RR, G2,RR,G2,RR,A2,RR,A2,RR};
static const u8 TI_N[32]={1,0,0,0,2,0,0,0, 1,0,0,0,2,0,0,0, 1,0,0,0,2,0,0,0, 1,0,0,0,2,0,2,0};
static const u8 RO_M[32]={E5,RR,C5,RR,B4,RR,A4,RR, E5,RR,D5,C5,B4,RR,RR,RR, C5,RR,A4,RR,G4,RR,E4,RR, A4,RR,B4,C5,A4,RR,RR,RR};
static const u8 RO_B[32]={A2,RR,RR,RR,A2,RR,RR,RR, C3,RR,RR,RR,E3,RR,RR,RR, F2,RR,RR,RR,F2,RR,RR,RR, E2,RR,RR,RR,E2,RR,RR,RR};
static const u8 NO_N[32]={Z8,Z8,Z8,Z8};
static const u8 VO_M[32]={A4,RR,RR,RR,Ds5,RR,RR,RR, G4,RR,RR,RR,Cs5,RR,RR,RR, F4,RR,RR,RR,B4,RR,RR,RR, E4,RR,RR,RR,As4,RR,RR,RR};
static const u8 VO_B[32]={A2,RR,RR,RR,RR,RR,RR,RR, G2,RR,RR,RR,RR,RR,RR,RR, F2,RR,RR,RR,RR,RR,RR,RR, E2,RR,RR,RR,RR,RR,RR,RR};
static const u8 BA_M[32]={A4,C5,E5,C5,A4,C5,E5,C5, G4,B4,D5,B4,G4,B4,D5,B4, F4,A4,C5,A4,F4,A4,C5,A4, E4,Gs4,B4,Gs4,E4,Gs4,B4,E5};
static const u8 BA_B[32]={A2,RR,A2,RR,A2,RR,A2,RR, G2,RR,G2,RR,G2,RR,G2,RR, F2,RR,F2,RR,F2,RR,F2,RR, E2,RR,E2,RR,E2,RR,E2,RR};
static const u8 BA_N[32]={1,0,2,0,1,0,2,2, 1,0,2,0,1,0,2,2, 1,0,2,0,1,0,2,2, 1,0,2,0,1,0,2,2};
static const u8 BO_M[32]={E5,RR,E5,D5,C5,RR,B4,RR, A4,RR,A4,B4,C5,RR,D5,RR, E5,RR,E5,G5,F5,RR,E5,RR, D5,RR,C5,RR,B4,RR,Gs4,RR};
static const u8 BO_B[32]={A2,A2,RR,A2,A2,RR,A2,RR, A2,A2,RR,A2,A2,RR,A2,RR, F2,F2,RR,F2,F2,RR,F2,RR, E2,E2,RR,E2,E2,RR,E2,RR};
static const u8 BO_N[32]={1,0,2,1,0,2,1,2, 1,0,2,1,0,2,1,2, 1,0,2,1,0,2,1,2, 1,0,2,1,0,2,1,2};
enum { SG_TITLE,SG_ROOM,SG_VOID,SG_BATTLE,SG_BOSS };
static const Song SONGS[]={
 {TI_M,TI_B,TI_N,32,12,2,9},{RO_M,RO_B,NO_N,32,16,1,8},{VO_M,VO_B,NO_N,32,22,2,7},
 {BA_M,BA_B,BA_N,32,7,2,8},{BO_M,BO_B,BO_N,32,6,1,9}};
static int cursong=-2,mstep,mt,sfx1,sfx4;

static void snd_init(void){ IO16(0x84)=0x80; IO16(0x80)=0xFF77; IO16(0x82)=2; IO16(0x60)=0; }
static void music_set(int s){
    if(cursong==s) return; cursong=s; mstep=0; mt=0;
}
static void music_tick(void){
    if(sfx1>0)sfx1--; if(sfx4>0)sfx4--;
    if(cursong<0) return;
    const Song*S=&SONGS[cursong];
    if(mt==0){
        int m=S->mel[mstep], b=S->bas[mstep], n=S->nz[mstep];
        if(m&&!sfx1){ IO16(0x62)=(u16)((S->mvol<<12)|(3<<8)|(S->duty<<6)); IO16(0x64)=(u16)(0x8000|note_x(m)); }
        if(b){ IO16(0x68)=(u16)((8<<12)|(1<<8)|(2<<6)); IO16(0x6C)=(u16)(0x8000|note_x(b)); }
        if(n&&!sfx4){
            if(n==1){ IO16(0x78)=(u16)((12<<12)|(2<<8)); IO16(0x7C)=(u16)(0x8000|(7<<4)|3); }
            else    { IO16(0x78)=(u16)((6<<12)|(1<<8));  IO16(0x7C)=(u16)(0x8000|(1<<4)|(1<<3)|1); }
        }
    }
    if(++mt>=S->tempo){ mt=0; if(++mstep>=S->len) mstep=0; }
}
enum { SF_BLIP,SF_HIT,SF_SWING,SF_HURT,SF_PICK,SF_SEL,SF_DIE,SF_SPIN };
static void sfx(int t){
    switch(t){
     case SF_BLIP: IO16(0x62)=(u16)((5<<12)|(1<<8)|(2<<6)); IO16(0x64)=(u16)(0x8000|1899); sfx1=3; break;
     case SF_SEL:  IO16(0x62)=(u16)((9<<12)|(2<<8)|(2<<6)); IO16(0x64)=(u16)(0x8000|note_x(B5)); sfx1=6; break;
     case SF_PICK: IO16(0x62)=(u16)((10<<12)|(3<<8)|(2<<6)); IO16(0x64)=(u16)(0x8000|note_x(C6)); sfx1=12; break;
     case SF_HURT: IO16(0x62)=(u16)((14<<12)|(3<<8)|(1<<6)); IO16(0x64)=(u16)(0x8000|1175); sfx1=20; break;
     case SF_HIT:  IO16(0x78)=(u16)((12<<12)|(2<<8)); IO16(0x7C)=(u16)(0x8000|(5<<4)|1); sfx4=10; break;
     case SF_SWING:IO16(0x78)=(u16)((7<<12)|(1<<8)); IO16(0x7C)=(u16)(0x8000|(2<<4)|(1<<3)|1); sfx4=6; break;
     case SF_SPIN: IO16(0x78)=(u16)((9<<12)|(3<<8)); IO16(0x7C)=(u16)(0x8000|(3<<4)|1); sfx4=18; break;
     case SF_DIE:  IO16(0x78)=(u16)((14<<12)|(4<<8)); IO16(0x7C)=(u16)(0x8000|(8<<4)|3); sfx4=24; break;
    }
}

/* ---------------- personajes (16x32, pixel art procedural) ---------------- */
static int hflip;
static void pr(int x,int y,int w,int h,u8 c){ if(hflip) x=16-x-w; rect(x,y,w,h,c); }
static void hero(int g,int dir,int fr){
    u8 hair = g? C_REDD : C_HAIR;
    hflip=(dir==3);
    if(dir<2){
        int h0=8-(fr==1)*2, h1=8-(fr==3)*2;
        pr(4,24,4,h0-2,C_BLUE); pr(3,24+h0-2,5,2,C_INK); pr(7,24,1,h0-2,C_BLUED);
        pr(8,24,4,h1-2,C_BLUE); pr(8,24+h1-2,5,2,C_INK); pr(8,24,1,h1-2,C_BLUED);
        int a=(fr==1)?1:(fr==3?-1:0);
        pr(0,14+a,3,8,C_INK); pr(1,14+a,2,7,C_YEL); pr(1,21+a,2,2,C_SKIN);
        pr(13,14-a,3,8,C_INK); pr(13,14-a,2,7,C_YELD); pr(13,21-a,2,2,C_SKIN);
        pr(2,13,12,11,C_INK); pr(3,13,10,10,C_YEL); pr(11,13,2,10,C_YELD); pr(8,14,1,9,C_YELD);
        pr(4,19,3,2,C_YELD);
        if(g){ pr(1,21,14,3,C_INK); pr(2,21,12,2,C_YEL); pr(11,21,3,2,C_YELD); }
        pr(2,1,12,13,C_INK); pr(5,0,6,1,C_INK); pr(3,1,10,12,C_YEL);
        if(dir==0){
            pr(4,4,8,8,C_YELD); pr(5,5,6,7,C_SKIN);
            pr(6,8,1,2,C_INK); pr(9,8,1,2,C_INK); pr(7,11,2,1,C_SKIND);
            if(g){ pr(5,5,6,3,hair); pr(4,6,1,9,hair); pr(11,6,1,9,hair); pr(3,1,3,2,C_MAG); }
            else { pr(5,5,6,2,hair); pr(5,7,1,1,hair); pr(10,7,1,1,hair); }
        } else {
            pr(8,2,1,10,C_YELD); pr(3,1,10,1,C_YELD);
            if(g){ pr(7,12,3,7,hair); pr(7,12,3,1,C_MAG); pr(3,1,3,2,C_MAG); }
            else { pr(5,11,6,2,hair); }
        }
    } else {
        int a=(fr==1)?1:(fr==3?-1:0);
        pr(5+a,24,3,6,C_BLUE); pr(4+a,30,4,2,C_INK);
        pr(8-a,24,3,6,C_BLUE); pr(7-a,30,4,2,C_INK);
        pr(3,13,10,11,C_INK); pr(4,13,8,10,C_YEL); pr(9,13,3,10,C_YELD);
        if(g){ pr(2,21,11,3,C_INK); pr(3,21,9,2,C_YEL); }
        pr(5-a,14,1,8,C_INK); pr(6-a,14,3,7,C_YELD); pr(9-a,14,1,8,C_INK); pr(6-a,21,3,2,C_SKIN);
        pr(2,1,12,13,C_INK); pr(5,0,6,1,C_INK); pr(3,1,10,12,C_YEL); pr(3,1,10,1,C_YELD);
        pr(3,4,6,8,C_YELD); pr(3,5,5,7,C_SKIN); pr(4,8,1,2,C_INK); pr(3,10,1,1,C_SKIND);
        if(g){ pr(3,5,5,3,hair); pr(9,5,3,10,hair); pr(10,2,3,2,C_MAG); }
        else { pr(3,5,5,2,hair); pr(8,6,1,2,hair); }
    }
    hflip=0;
}
static u8 hbuf[16*32];
static void hero_blit(int x,int y,int g,int dir,int fr,int sc){
    settgt(hbuf,16,32); rect(0,0,16,32,255); hero(g,dir,fr); settgt(fb,SW,SH);
    blit(hbuf,16,32,x,y,sc);
}

/* ---------------- corazones ---------------- */
static void heart(int x,int y,int fill){
    static const u8 H[6]={0x36,0x7F,0x7F,0x3E,0x1C,0x08};
    for(int r=0;r<6;r++)for(int b=0;b<7;b++) if(H[r]&(0x40>>b)){
        u8 c; if(fill==2)c=C_RED; else if(fill==1&&b<4)c=C_RED; else c=C_GRAYD;
        px(x+b,y+r,c); }
}

/* ---------------- datos del mundo ---------------- */
enum { K_BED,K_WARD,K_POSTA,K_POSTB,K_POSTC,K_WIN,K_CLOCK,K_DOOR,K_DESK,K_PLUSH,K_LAMP,K_RUG,
       K_TV,K_TVD,K_NPC,K_SIGN,K_LADDER,K_CHAIR,K_DIR,K_FOUNT,K_BENCH,K_PLANT,K_TABLE,K_BAL };
typedef struct { s16 x,y; u8 w,h,kind,solid,arg; s8 to; u8 sx,sy; const char*txt; } Obj;
typedef struct { s16 x,y; u8 w,h; s8 to; u8 sx,sy,lock; } Exit;
typedef struct { u8 type; s16 x,y; } Spawn;
typedef struct { const char*name; u8 theme,song,nw; s16 wr[3][4]; const Obj*o; u8 no; const Exit*e; u8 ne; const Spawn*s; u8 ns; } Room;
enum { TH_BED,TH_VOID,TH_POOL,TH_MALL,TH_PARTY };
enum { E_DRIP,E_TV,E_TAPE,E_BOSS };

#define NONE 0
static const Obj O_BED[]={
 {20,14,24,40,K_POSTA,0,0,-1,0,0,"* Un póster de un payaso sin cara. Abajo dice: ¡FELIZ CUMPLEAÑOS!|* No recuerdas haber cumplido años en esta casa."},
 {54,16,24,38,K_POSTB,0,0,-1,0,0,"* Un póster de una piscina vacía. Hueles a cloro, aunque no hay agua.|* Sientes que ya has estado ahí."},
 {96,8,36,46,K_WIN,0,0,-1,0,0,"* Por la ventana solo hay estática blanca. Suena como lluvia.|* Pegas la oreja al vidrio. Alguien del otro lado también la pegó."},
 {144,16,14,36,K_CLOCK,0,0,-1,0,0,"* El reloj marca las 3:33. No avanza. Ni siquiera hace tic."},
 {166,12,20,42,K_POSTC,0,0,-1,0,0,"* Un póster de la tele: CANAL 0, TRANSMISIÓN NOCTURNA.|* Las letras se mueven cuando no las miras."},
 {200,6,28,48,K_DOOR,0,0,1,12,82,"* La puerta no tiene manija por dentro.|* Pero se abre sola. Del otro lado no hay pasillo: solo oscuridad."},
 {96,114,64,32,K_RUG,0,0,-1,0,0,"* Una alfombra con ondas. Si la miras mucho, se mueven."},
 {8,62,56,34,K_BED,1,0,-1,0,0,"* Una cama pequeña. Las sábanas tienen nubes con cara. Todas sonríen igual.|* Aún está tibia. ¿Hace cuánto te levantaste?"},
 {170,70,36,44,K_WARD,1,0,-1,0,0,"* Un armario viejo. Dentro solo hay impermeables amarillos, todos de tu talla.|* Ya llevas uno puesto. Cierras la puerta rápido: algo respiraba detrás de la ropa."},
 {92,68,44,30,K_DESK,1,0,-1,0,0,"* Una tele de tubo con un VHS dentro. La etiqueta dice: NOCHE DE LLUVIA, con tu letra.|* El botón de expulsar no funciona. Alguien lo pegó con cinta."},
 {40,122,14,14,K_PLUSH,1,0,-1,0,0,"* Un conejo de peluche sin ojos. Te mira de todos modos."},
 {140,78,12,24,K_LAMP,1,0,-1,0,0,"* La luz parpadea con un ritmo. Parece que intenta decirte algo.|* Tres cortos, dos largos. Lo has soñado antes."},
};
static const Exit X_BED[]={{0,0,0,0,-1,0,0,0}};
static const Obj O_VOID[]={
 {44,36,28,26,K_TV,0,0,-1,0,0,"* Una tele flotando. Solo estática. Escuchas tu nombre, @, muy bajito."},
 {106,26,28,26,K_TV,0,1,-1,0,0,"* Barras de colores. Debajo parpadea: POR FAVOR, NO CAMBIES DE CANAL."},
 {168,36,28,26,K_TV,0,2,-1,0,0,"* Una cara sonriente en la pantalla. Sonríe tanto que parece dolerle."},
 {8,66,28,26,K_TV,0,3,-1,0,0,"* Muestra lluvia. Un niño con impermeable amarillo corre bajo ella. Eres tú. O lo eras."},
 {204,66,28,26,K_TV,0,4,-1,0,0,"* Una luz roja: REC. La tele te graba desde que despertaste.|* Saludas. Ella también."},
 {10,8,28,26,K_TVD,0,1,-1,0,0,NULL},{200,10,28,26,K_TVD,0,0,-1,0,0,NULL},
 {14,136,28,26,K_TVD,0,2,-1,0,0,NULL},{212,134,28,26,K_TVD,0,3,-1,0,0,NULL},
 {112,96,16,24,K_NPC,1,0,-1,0,0,NULL},
};
static const Exit X_VOID[]={{0,96,3,32,0,206,34,0},{237,96,3,32,2,12,70,2}};
static const Obj O_POOL[]={
 {24,22,24,32,K_SIGN,0,0,-1,0,0,"* Letrero: PROHIBIDO CORRER, NADAR Y RECORDAR. Alguien tachó la última palabra."},
 {190,18,16,36,K_LADDER,0,0,-1,0,0,"* Una escalera de piscina que baja a un lugar donde ya estás. Mejor no."},
 {100,40,24,36,K_CHAIR,1,0,-1,0,0,"* La silla del salvavidas. Hay un silbato y una nota: VUELVO EN 5 MINUTOS.|* La nota tiene 40 años."},
 {204,84,16,24,K_NPC,1,1,-1,0,0,NULL},
};
static const Exit X_POOL[]={{0,90,3,50,1,216,80,0},{237,90,3,50,3,12,70,1}};
static const Spawn S_POOL[]={{E_DRIP,60,96},{E_DRIP,120,118},{E_DRIP,160,84}};
static const Obj O_MALL[]={
 {20,22,28,32,K_DIR,0,0,-1,0,0,"* Directorio: USTED ESTÁ AQUÍ. El puntito rojo se mueve cada vez que parpadeas."},
 {104,84,32,28,K_FOUNT,1,0,-1,0,0,"* Una fuente seca. En el fondo hay monedas. Cada una tiene una cara distinta. Todas tuyas."},
 {40,116,32,14,K_BENCH,1,0,-1,0,0,"* Una banca con una bolsa. Dentro hay una cinta de VHS vacía."},
 {214,70,12,20,K_PLANT,1,0,-1,0,0,"* Una planta de plástico. Tiene polvo de treinta años y una hoja nueva."},
 {170,88,16,24,K_NPC,1,2,-1,0,0,NULL},
};
static const Exit X_MALL[]={{0,90,3,50,2,214,70,0},{237,90,3,50,4,12,70,1}};
static const Spawn S_MALL[]={{E_TV,56,84},{E_TV,150,124},{E_TAPE,190,70}};
static const Obj O_PARTY[]={
 {16,18,14,24,K_BAL,0,0,-1,0,0,NULL},{40,28,14,24,K_BAL,0,1,-1,0,0,NULL},
 {190,22,14,24,K_BAL,0,2,-1,0,0,NULL},{214,14,14,24,K_BAL,0,3,-1,0,0,NULL},
 {88,38,64,36,K_TABLE,1,0,-1,0,0,"* Un pastel con velas apagadas. Dice: BIENVENIDO DE NUEVO.|* El número de velas cambia cada vez que lo cuentas."},
};
static const Exit X_PARTY[]={{0,90,3,50,3,214,70,0}};
static const Spawn S_PARTY[]={{E_BOSS,104,88}};
static const Room ROOMS[]={
 {"HABITACIÓN",TH_BED,SG_ROOM,1,{{4,58,236,158}},O_BED,sizeof(O_BED)/sizeof(Obj),X_BED,0,NULL,0},
 {"EL VACÍO",TH_VOID,SG_VOID,3,{{40,76,200,148},{0,96,56,128},{184,96,240,128}},O_VOID,sizeof(O_VOID)/sizeof(Obj),X_VOID,2,NULL,0},
 {"PISCINA VACÍA",TH_POOL,SG_BATTLE,1,{{0,58,240,152}},O_POOL,sizeof(O_POOL)/sizeof(Obj),X_POOL,2,S_POOL,3},
 {"CENTRO COMERCIAL 1994",TH_MALL,SG_BATTLE,1,{{0,58,240,152}},O_MALL,sizeof(O_MALL)/sizeof(Obj),X_MALL,2,S_MALL,3},
 {"SALA DEL CUMPLEAÑOS",TH_PARTY,SG_BOSS,1,{{0,58,240,152}},O_PARTY,sizeof(O_PARTY)/sizeof(Obj),X_PARTY,1,S_PARTY,1},
};

static const char* const NPC1[3]={
 /* Beta */
 "¡Shh! No grites, @. Aquí las paredes graban todo.|Me llamo Beta. Fui la copia de seguridad de la cinta. Nadie hace copias de las copias, así que me olvidaron aquí.|Estás dentro de NOCHE DE LLUVIA, un VHS que grabó tu abuelo el día en que todos salieron con impermeables amarillos.|La cinta se atascó en bucle. Y Don Rebobinado no quiere que salgas.|Cruza la Piscina Vacía y el Centro Comercial 1994. Al fondo está la Sala del Cumpleaños.|Toma esto: tu ESPADA DE JUGUETE. B para atacar. Si mantienes B, giras como un remolino.",
 /* Ramon */
 "Soy Ramón, salvavidas. Cuarenta años vigilando una piscina sin agua.|Aquí nadie se ahoga. Solo se olvida. Las Goteras son recuerdos que se evaporaron y quedaron enojados.|Págales con la espada. La puerta de la derecha se abre cuando no quede ninguna.",
 /* Gerente */
 "¡BIENVENIDO al Centro Comercial 1994! Hoy todo está en oferta... menos tú.|Soy el Gerente Sonrisas. Me pegaron esta sonrisa y ya no sé cómo quitarla.|Cuidado con los Pantallazos y las Cintavoraces. Se comen la cinta de quien olvida su cumpleaños.|Al fondo está la Sala del Cumpleaños. Don Rebobinado celebra el mismo día desde hace treinta años.|Toma un helado de cortesía. Cura hasta lo que no duele."
};
static const char* const NPC2[3]={
 "Sigue a la derecha, @. Y no mires directo a las pantallas. Se acuerdan de ti.",
 "La puerta de la derecha se abre cuando no queden Goteras. Siempre ha sido así.",
 "¡Feliz cumpleaños, @! ...Perdona, es lo único que sé decir. Toma otro helado."
};
static const char TXT_INTRO[]="Despiertas en un lugar extraño, @, o quizás no, parece... Conocido, ¿Dónde crees que estamos?";
static const char TXT_VOID1[]="* Detrás de la puerta solo hay oscuridad. Y madera. Y televisores... flotando.";
static const char TXT_BOSS1[]="¿Ya llegas, @? Tarde. Siempre llegas tarde.|Hace treinta años que repito este día. La lluvia nunca para y nadie crece.|Quédate. Te rebobinaré cada vez que quieras volver.";
static const char TXT_WIN[]="Don Rebobinado se detiene. Su pantalla parpadea 0:00.|Solo quería que el día no terminara... Era la última vez que el abuelo grabó algo.|La cinta sale expulsada con un clac. Beta aparece en tu bolsillo: Despierta, @. Yo me quedo a cuidar el final.|La estática lo cubre todo...|Despiertas. Tu habitación. Los pósters siguen ahí.|En la almohada hay una cinta sin etiqueta.|O quizás no...";
static const char TXT_CLEAR[]="* La estática se calma. Algo se abre a la derecha.";
static const char TXT_LOCK1[]="* Algo te impide avanzar: aún quedan criaturas en la sala.";
static const char TXT_LOCK2[]="* Una voz te llama desde el centro del pasillo. Habla con ella antes de seguir.";

/* ---------------- estado global ---------------- */
enum { M_TITLE,M_CRED,M_SEL,M_NAME,M_PLAY,M_PAUSE,M_OVER,M_END };
static int mode,t,gender,nlen,menu_sel;
static u16 pname[9];
static int room;
static int pl_x,pl_y,pl_dir,pl_fr,pl_hp,pl_inv,pl_atk,pl_charge,pl_spin,pl_kx,pl_ky,pl_kb;
static u8 cleared[5],seen[5],talked[3],has_sword,boss_dead;
static int banner,tr,trc,tr_to,tr_sx,tr_sy,pend_ev;
static int gl_t,gl_y;
static int name_cur;

typedef struct { u8 type,alive,hp,st,hurt,tm,kb; s16 x,y; s8 vx,vy,kx,ky; } En;
static En en[6];
typedef struct { s16 x,y; u8 on,life; } Drop;
static Drop drops[4];
typedef struct { s16 x,y; s16 vx,vy; u8 on,life; } Prj;
static Prj prj[8];
typedef struct { s16 x,y; u8 t; } Fx;
static Fx fxs[6];

/* ---------------- fondos ---------------- */
static void bg_bed(void){
    rect(0,0,SW,56,C_CREAM);
    for(int x=0;x<SW;x+=12) rect(x,0,1,52,C_CREAMD);
    for(int x=6;x<SW;x+=24) for(int y=6;y<48;y+=14) rect(x,y,3,2,C_LAV);
    rect(0,50,SW,6,C_WOODD); rect(0,50,SW,1,C_WOOD);
    rect(0,56,SW,104,C_WOOD);
    for(int y=56;y<SH;y+=12){ rect(0,y,SW,1,C_WOODD); int o=((y/12)&1)*20; for(int x=o;x<SW;x+=40) rect(x,y,1,12,C_WOODD); }
}
static void bg_void(void){
    rect(0,0,SW,SH,C_VOID);
    for(int i=0;i<34;i++){ int x=(i*97+13)%SW, y=(i*53+7)%150; if(((t>>4)+i)%6) px(x,y,(i&1)?C_PUR2:C_PUR); }
    rect(40,76,160,72,C_WOOD); rect(0,96,56,32,C_WOOD); rect(184,96,56,32,C_WOOD);
    for(int y=76;y<148;y+=8){ rect(40,y,160,1,C_WOODD); int o=((y>>3)&1)*24; for(int x=40+o;x<200;x+=48) rect(x,y,1,8,C_WOODD); }
    for(int y=96;y<128;y+=8){ rect(0,y,56,1,C_WOODD); rect(184,y,56,1,C_WOODD); }
    rect(40,148,160,5,C_WOODD); rect(0,128,56,4,C_WOODD); rect(184,128,56,4,C_WOODD);
    rect(40,148,160,1,C_INK);
}
static void bg_pool(void){
    rect(0,0,SW,56,C_WHITE);
    for(int x=0;x<SW;x+=16) rect(x,0,1,56,C_LAV); for(int y=0;y<56;y+=16) rect(0,y,SW,1,C_LAV);
    rect(0,48,SW,8,C_CYAND);
    rect(0,56,SW,104,C_CYAN);
    for(int x=0;x<SW;x+=16) rect(x,56,1,104,C_CYAND); for(int y=56;y<SH;y+=16) rect(0,y,SW,1,C_CYAND);
    for(int i=0;i<14;i++){ int x=(i*37+(t>>1))%SW, y=62+((i*23)%92); rect(x,y,8,1,C_LBLUE); rect(x+2,y+1,4,1,C_LBLUE); }
}
static void bg_mall(void){
    rect(0,0,SW,56,C_MAGD);
    for(int x=0;x<SW;x+=16) rect(x,0,8,50,C_MAG);
    rect(0,50,SW,6,C_GRAY);
    int fl=((t%97)<5);
    for(int x=32;x<SW;x+=80) rect(x,0,24,4,fl?C_GRAY:C_PALEY);
    for(int y=56;y<SH;y+=16)for(int x=0;x<SW;x+=16) rect(x,y,16,16,(((x+y)>>4)&1)?C_GRAY:C_CREAM);
}
static void bg_party(void){
    rect(0,0,SW,56,C_PUR);
    for(int x=0;x<SW;x+=20) rect(x,0,10,50,C_PUR2);
    for(int x=0;x<SW;x+=16){ u8 c=((x>>4)&1)?C_YEL:C_MAG; for(int i=0;i<6;i++) rect(x+i,i*1+2,16-i*2>0?16-i*2:1,1,c); }
    rect(0,50,SW,6,C_INK);
    rect(0,56,SW,104,C_INK);
    for(int i=0;i<46;i++){ int x=(i*83+11)%SW, y=60+(i*41)%98; px(x,y,(i%3==0)?C_CYAN:(i%3==1)?C_MAG:C_YEL); px(x+1,y,C_PUR2); }
}

/* ---------------- objetos ---------------- */
static void tv_screen(int x,int y,int ch){   /* pantalla 22x14 */
    switch(ch){
     case 0: rect(x,y,22,14,C_GRAYD); for(int i=0;i<70;i++) px(x+rnd()%22,y+rnd()%14,(rnd()&1)?C_WHITE:C_GRAY); break;
     case 1: { static const u8 bc[7]={C_WHITE,C_PALEY,C_CYAN,C_GRN,C_MAG,C_RED,C_BLUE};
               for(int i=0;i<7;i++) rect(x+i*3+(i>3?0:0),y,3,14,bc[i]); rect(x,y+10,22,4,C_INK); if((t>>4)&1) rect(x+2,y+11,8,2,C_WHITE); } break;
     case 2: rect(x,y,22,14,C_PALEY); rect(x+5,y+3,3,1,C_INK);rect(x+6,y+2,1,3,C_INK);
             rect(x+14,y+3,3,1,C_INK);rect(x+15,y+2,1,3,C_INK);
             rect(x+5,y+9,12,1,C_INK); rect(x+4,y+8,1,1,C_INK); rect(x+17,y+8,1,1,C_INK); break;
     case 3: rect(x,y,22,14,C_PUR2);
             for(int i=0;i<6;i++) rect(x+2+i*4,y+((t*2+i*5)%14),1,3,C_LBLUE);
             rect(x+9,y+6,4,3,C_YEL); rect(x+10,y+9,2,4,C_YEL); rect(x+10,y+3,2,2,C_SKIN); break;
     default: rect(x,y,22,14,C_INK); if((t>>5)&1) disc(x+5,y+7,3,C_RED); txt("REC",x+10,y+3,C_WHITE,1); break;
    }
}
static void draw_tv(const Obj*o){
    int b=bobt[((t>>2)+o->arg*3+(o->x>>2))&15];
    int x=o->x,y=o->y+b;
    rect(x+8,y-3,1,3,C_GRAY); rect(x+17,y-3,1,3,C_GRAY); rect(x+9,y-4,3,1,C_GRAY); rect(x+14,y-4,3,1,C_GRAY);
    rect(x,y,28,22,C_INK); rect(x+1,y+1,26,20,C_GRAYD); rect(x+2,y+2,24,16,C_INK);
    tv_screen(x+3,y+3,o->arg);
    rect(x+20,y+19,2,1,C_RED); rect(x+4,y+19,6,1,C_GRAY);
}
static void draw_npc(const Obj*o){
    int x=o->x,y=o->y,b=bobt[((t>>2)+(o->arg<<2))&15];
    if(o->arg==0){ y+=b;                                     /* Beta: fantasma-cassette */
        rect(x,y+2,16,18,C_INK); rect(x+1,y+3,14,16,C_LAV); rect(x+3,y+5,10,5,C_PALEY);
        rect(x+4,y+7,2,3,C_INK); rect(x+10,y+7,2,3,C_INK); rect(x+7,y+14,2,1,C_INK);
        for(int i=0;i<4;i++) rect(x+1+i*4,y+19+(i&1),3,2,C_LAV);
        rect(x+14,y+12,1,8,C_BRIGHT);
    } else if(o->arg==1){                                      /* Ramon: pato salvavidas */
        rect(x+3,y,10,9,C_INK); rect(x+4,y+1,8,7,C_YEL); rect(x+11,y+4,5,3,C_CORAL); rect(x+5,y+3,2,2,C_INK);
        rect(x+2,y+9,12,11,C_INK); rect(x+3,y+9,10,10,C_YEL); rect(x+3,y+13,10,3,C_RED);
        rect(x+7,y+9,2,3,C_WHITE); rect(x+4,y+20,3,4,C_CORAL); rect(x+9,y+20,3,4,C_CORAL);
    } else {                                                   /* Gerente Sonrisas */
        rect(x+2,y,12,11,C_INK); rect(x+3,y+1,10,9,C_PALEY); rect(x+5,y+3,1,2,C_INK); rect(x+10,y+3,1,2,C_INK);
        rect(x+3,y+7,10,3,C_INK); rect(x+4,y+8,8,1,C_WHITE);
        rect(x+3,y+11,10,10,C_INK); rect(x+4,y+11,8,9,C_BLUE); rect(x+7,y+11,2,6,C_RED);
        rect(x+4,y+21,3,3,C_INK); rect(x+9,y+21,3,3,C_INK);
    }
}
static void draw_obj(const Obj*o){
    int x=o->x,y=o->y;
    switch(o->kind){
     case K_BED: rect(x,y,56,34,C_INK); rect(x+1,y+1,54,32,C_WOODD); rect(x+3,y+3,14,12,C_WHITE); rect(x+3,y+17,14,12,C_WHITE);
                 rect(x+18,y+3,35,28,C_LBLUE); rect(x+24,y+9,10,4,C_WHITE); rect(x+40,y+19,10,4,C_WHITE); rect(x+18,y+3,2,28,C_BLUE); break;
     case K_WARD: rect(x,y,36,44,C_INK); rect(x+1,y+1,34,42,C_WOOD); rect(x+1,y+1,34,3,C_WOODD);
                 rect(x+17,y+4,2,39,C_INK); rect(x+4,y+8,11,30,C_WOODD); rect(x+21,y+8,11,30,C_WOODD);
                 rect(x+13,y+22,2,5,C_PALEY); rect(x+21,y+22,2,5,C_PALEY); break;
     case K_POSTA: rect(x,y,24,32,C_INK); rect(x+1,y+1,22,30,C_PALEY); disc(x+6,y+12,5,C_RED); disc(x+18,y+12,5,C_RED);
                 disc(x+12,y+14,8,C_WHITE); rect(x+4,y+23,16,6,C_CYAN); break;
     case K_POSTB: rect(x,y,24,24,C_INK); rect(x+1,y+1,22,22,C_LBLUE); rect(x+3,y+10,18,11,C_CYAN); rect(x+16,y+4,2,10,C_GRAY); rect(x+19,y+4,2,10,C_GRAY); break;
     case K_POSTC: rect(x,y,20,28,C_INK); for(int i=0;i<7;i++) rect(x+1+i*17/7*1,y+1,2,12,(u8[]){C_WHITE,C_PALEY,C_CYAN,C_GRN,C_MAG,C_RED,C_BLUE}[i]);
                 txt("CH0",x+1,y+16,C_WHITE,1); break;
     case K_WIN: rect(x,y,36,32,C_WOODD); rect(x+2,y+2,32,28,C_GRAY); for(int i=0;i<50;i++) px(x+2+rnd()%32,y+2+rnd()%28,(rnd()&1)?C_WHITE:C_GRAYD);
                 rect(x+17,y+2,2,28,C_WOODD); rect(x+2,y+15,32,2,C_WOODD); break;
     case K_CLOCK: disc(x+7,y+7,7,C_INK); disc(x+7,y+7,6,C_WHITE); rect(x+7,y+7,4,1,C_INK); rect(x+7,y+7,1,5,C_RED); break;
     case K_DOOR: rect(x,y,28,50,C_INK); rect(x+1,y+1,26,49,C_WOOD); rect(x+4,y+4,20,18,C_WOODD); rect(x+4,y+26,20,20,C_WOODD);
                 rect(x+24,y+1,3,49,C_VOID); disc(x+14,y+2,0,C_INK); break;
     case K_DESK: rect(x,y+14,44,16,C_WOOD); rect(x,y+14,44,2,C_WOODD); rect(x+2,y+28,3,2,C_INK); rect(x+39,y+28,3,2,C_INK);
                 rect(x+8,y,28,16,C_INK); rect(x+9,y+1,26,14,C_GRAYD); rect(x+11,y+2,16,11,(t>>4&1)?C_CYAND:C_INK);
                 for(int i=0;i<6;i++) px(x+11+rnd()%16,y+2+rnd()%11,C_WHITE); rect(x+29,y+3,4,2,C_GRAY); rect(x+30,y+8,2,2,C_RED); break;
     case K_PLUSH: rect(x+3,y+5,8,9,C_WHITE); rect(x+4,y+1,6,6,C_WHITE); rect(x+4,y-4,2,6,C_WHITE); rect(x+8,y-4,2,6,C_WHITE);
                 rect(x+5,y+3,1,1,C_LAV); rect(x+8,y+3,1,1,C_LAV); break;
     case K_LAMP: rect(x+5,y+8,2,16,C_GRAY); rect(x+1,y,10,8,((t%90)<6)?C_CREAMD:C_PALEY); rect(x+2,y+22,8,2,C_GRAYD); break;
     case K_RUG: rect(x,y,64,32,C_MAGD); rect(x+3,y+3,58,26,C_PUR2);
                 for(int i=0;i<4;i++)for(int k=0;k<14;k++) rect(x+4+k*4,y+6+i*6+bobt[(k+(t>>3))&15]/2,2,1,C_LAV); break;
     case K_TV: case K_TVD: draw_tv(o); break;
     case K_NPC: draw_npc(o); break;
     case K_SIGN: rect(x,y,24,18,C_INK); rect(x+1,y+1,22,16,C_PALEY); rect(x+3,y+3,18,2,C_RED); rect(x+3,y+7,14,2,C_INK); rect(x+3,y+11,10,2,C_INK); break;
     case K_LADDER: rect(x+2,y,2,36,C_GRAY); rect(x+12,y,2,36,C_GRAY); for(int i=0;i<4;i++) rect(x+2,y+6+i*8,12,2,C_GRAY); break;
     case K_CHAIR: rect(x+2,y+16,2,20,C_GRAYD); rect(x+20,y+16,2,20,C_GRAYD); rect(x,y+14,24,4,C_WHITE);
                 rect(x+2,y,20,14,C_RED); rect(x+2,y+5,20,3,C_WHITE); rect(x+3,y+26,18,2,C_GRAYD); break;
     case K_DIR: rect(x,y,28,32,C_INK); rect(x+1,y+1,26,30,C_GRAYD); rect(x+3,y+3,22,20,C_CREAM);
                 rect(x+5,y+8,18,1,C_GRAY); rect(x+11,y+4,1,18,C_GRAY); rect(x+5,y+16,18,1,C_GRAY);
                 rect(x+5+((t>>3)%14),y+6+(((t>>3)*3)%12),3,3,C_RED); break;
     case K_FOUNT: rect(x,y+14,32,14,C_INK); rect(x+1,y+15,30,12,C_GRAY); rect(x+4,y+17,24,7,C_GRAYD);
                 px(x+8,y+20,C_PALEY); px(x+15,y+22,C_PALEY); px(x+22,y+19,C_PALEY);
                 rect(x+13,y+2,6,14,C_GRAY); rect(x+9,y,14,4,C_GRAY); break;
     case K_BENCH: rect(x,y,32,5,C_WOOD); rect(x,y+6,32,4,C_WOODD); rect(x+2,y+10,3,4,C_INK); rect(x+27,y+10,3,4,C_INK); break;
     case K_PLANT: rect(x+2,y+12,8,8,C_REDD); rect(x+1,y+2,4,10,C_GRN); rect(x+7,y,4,12,C_GRND); rect(x+4,y+4,4,8,C_GRN); break;
     case K_TABLE: rect(x,y+14,64,22,C_PALEY); rect(x,y+14,64,3,C_WHITE); rect(x+2,y+34,3,2,C_INK); rect(x+59,y+34,3,2,C_INK);
                 rect(x+22,y+4,20,12,C_WHITE); rect(x+22,y+4,20,4,C_CORAL);
                 for(int i=0;i<5;i++){ rect(x+25+i*4,y,1,4,C_CYAN); if((t+i*7)&8) px(x+25+i*4,y-1,C_YEL);} break;
     case K_BAL: { static const u8 bc[4]={C_RED,C_CYAN,C_YEL,C_MAG}; int b=bobt[((t>>3)+o->arg*4)&15];
                 disc(x+7,y+7+b,6,bc[o->arg&3]); px(x+5,y+5+b,C_WHITE); rect(x+7,y+14+b,1,10,C_GRAY); } break;
    }
}

/* ---------------- colisiones ---------------- */
static int ov(int ax,int ay,int aw,int ah,int bx,int by,int bw,int bh){ return ax<bx+bw&&bx<ax+aw&&ay<by+bh&&by<ay+ah; }
static int inwalk(int x,int y,int w,int h){
    const Room*R=&ROOMS[room];
    for(int i=0;i<R->nw;i++){ const s16*r=R->wr[i]; if(x>=r[0]&&y>=r[1]&&x+w<=r[2]&&y+h<=r[3]) return 1; }
    return 0;
}
static int walk_ok(int x,int y,int w,int h){
    if(!inwalk(x,y,w,h)) return 0;
    const Room*R=&ROOMS[room];
    for(int i=0;i<R->no;i++){ const Obj*o=&R->o[i]; if(!o->solid) continue;
        int top=o->y+o->h*2/5; if(ov(x,y,w,h,o->x,top,o->w,o->h-(top-o->y))) return 0; }
    return 1;
}
static void pl_move(int dx,int dy){
    while(dx){ int s=sgn(dx); if(walk_ok(pl_x+3+s,pl_y+26,10,6)) pl_x+=s; else break; dx-=s; }
    while(dy){ int s=sgn(dy); if(walk_ok(pl_x+3,pl_y+26+s,10,6)) pl_y+=s; else break; dy-=s; }
}
static int ewalk(const En*e,int x,int y){ int sz=e->type==E_BOSS?32:16; return inwalk(x+2,y+sz/2,sz-4,sz/2); }

/* ---------------- dialogo (estilo Undertale) ---------------- */
enum { AF_NONE,AF_GOTO,AF_SWORD,AF_HEAL,AF_END };
static int dl_on,dl_af,dl_a,dl_b,dl_shown,dl_tot,dl_ln;
static const char *dl_src,*dl_next;
static u16 dl_pg[4][40]; static u8 dl_len[4];
#define COLS 35
static void dl_build(void){
    u16 tmp[260]; int n=0; const char*p=dl_src; u16 g;
    while(*p&&*p!='|'&&n<250){
        if(*p=='@'){ p++; for(int i=0;i<nlen;i++) tmp[n++]=pname[i]; continue; }
        p=dec1(p,&g); if(g) tmp[n++]=g;
    }
    dl_next=(*p=='|')?p+1:0;
    for(int l=0;l<4;l++){ dl_len[l]=0; for(int c=0;c<40;c++) dl_pg[l][c]=0; }
    int ln=0,col=0,i=0;
    while(i<n&&ln<4){
        if(tmp[i]==' '){ i++; continue; }
        int j=i; while(j<n&&tmp[j]!=' ') j++;
        int wl=j-i;
        if(col>0&&col+1+wl>COLS){ dl_len[ln]=(u8)col; ln++; col=0; if(ln>=4) break; }
        if(col>0) dl_pg[ln][col++]=' ';
        for(int k=0;k<wl&&col<COLS;k++) dl_pg[ln][col++]=tmp[i+k];
        i=j;
    }
    if(ln<4) dl_len[ln]=(u8)col;
    dl_ln=(ln<4)?ln+1:4;
    dl_tot=0; for(int l=0;l<dl_ln;l++) dl_tot+=dl_len[l];
    dl_shown=0;
}
static void say(const char*s,int af,int a,int b){ dl_src=s; dl_af=af; dl_a=a; dl_b=b; dl_on=1; dl_build(); }
static void goto_room(int r,int sx,int sy){ tr=1; trc=0; tr_to=r; tr_sx=sx; tr_sy=sy; }
static void dialog_end(void){
    dl_on=0;
    switch(dl_af){
     case AF_GOTO: goto_room(dl_a,dl_b&255,dl_b>>8); break;
     case AF_SWORD: has_sword=1; sfx(SF_PICK); break;
     case AF_HEAL: pl_hp=6; sfx(SF_PICK); break;
     case AF_END: mode=M_END; t=0; music_set(SG_ROOM); break;
    }
}
static void dialog_update(void){
    if(dl_shown<dl_tot){
        if(kp&(K_A|K_B)) dl_shown=dl_tot;
        else { dl_shown+=1+((t&1)?0:1); if(t%3==0) sfx(SF_BLIP); if(dl_shown>dl_tot) dl_shown=dl_tot; }
    } else if(kp&K_A){
        if(dl_next){ dl_src=dl_next; dl_build(); sfx(SF_SEL); } else dialog_end();
    }
}
static void draw_box(int x,int y,int w,int h){ rect(x,y,w,h,C_INK); frame(x,y,w,h,C_WHITE); frame(x+2,y+2,w-4,h-4,C_PUR2); }
static void dialog_draw(int top){
    int by=top?6:100; draw_box(4,by,232,54);
    int left=dl_shown;
    for(int l=0;l<dl_ln;l++){
        for(int c=0;c<dl_len[l]&&left>0;c++,left--) gl(dl_pg[l][c],10+c*6,by+7+l*11,C_WHITE,1);
    }
    if(dl_shown>=dl_tot && ((t>>4)&1)) rect(222,by+44,5,3,C_WHITE);
}

/* ---------------- enemigos ---------------- */
static void fx_add(int x,int y){ for(int i=0;i<6;i++) if(!fxs[i].t){ fxs[i].x=(s16)x; fxs[i].y=(s16)y; fxs[i].t=16; return; } }
static void en_add(int type,int x,int y){
    for(int i=0;i<6;i++) if(!en[i].alive){
        En*e=&en[i]; e->type=(u8)type; e->alive=1; e->x=(s16)x; e->y=(s16)y; e->vx=e->vy=e->kx=e->ky=0;
        e->hurt=0; e->kb=0; e->st=0; e->tm=(u8)(40+rnd()%40);
        e->hp= type==E_DRIP?2: type==E_TV?3: type==E_TAPE?4: 16;
        return; }
}
static void spawn_room(void){
    for(int i=0;i<6;i++) en[i].alive=0;
    for(int i=0;i<4;i++) drops[i].on=0;
    for(int i=0;i<8;i++) prj[i].on=0;
    const Room*R=&ROOMS[room];
    if(room==4){ if(!boss_dead) en_add(E_BOSS,R->s[0].x,R->s[0].y); return; }
    if(cleared[room]) return;
    for(int i=0;i<R->ns;i++) en_add(R->s[i].type,R->s[i].x,R->s[i].y);
}
static void pl_damage(int n,int fx,int fy){
    if(pl_inv) return;
    pl_hp-=n; pl_inv=70; sfx(SF_HURT);
    pl_kx=sgn(pl_x+8-fx)*3; pl_ky=sgn(pl_y+20-fy)*3; pl_kb=7;
    if(pl_hp<=0){ pl_hp=0; mode=M_OVER; t=0; sfx(SF_DIE); music_set(-1); IO16(0x68)=0; }
}
static void en_hurt(En*e,int dmg,int dx,int dy){
    e->hp-=dmg; e->hurt=16; sfx(SF_HIT);
    if(e->type!=E_BOSS){ e->kx=(s8)(dx*3); e->ky=(s8)(dy*3); e->kb=7; if(e->type==E_TAPE) e->st=3,e->tm=30; }
    if(e->hp<=0){
        e->alive=0; fx_add(e->x+(e->type==E_BOSS?16:8),e->y+(e->type==E_BOSS?16:8)); sfx(SF_DIE);
        if(e->type==E_BOSS){ boss_dead=1; cleared[4]=1; for(int i=0;i<8;i++) prj[i].on=0; }
        else if(rnd()%3==0) for(int i=0;i<4;i++) if(!drops[i].on){ drops[i].on=1; drops[i].x=e->x+4; drops[i].y=e->y+6; drops[i].life=250; break; }
    }
}
static void prj_add(int x,int y,int vx,int vy){ for(int i=0;i<8;i++) if(!prj[i].on){ prj[i].on=1; prj[i].x=(s16)(x*8); prj[i].y=(s16)(y*8); prj[i].vx=(s16)vx; prj[i].vy=(s16)vy; prj[i].life=200; return; } }
static int dist_approx(int dx,int dy){ dx=iabs(dx);dy=iabs(dy); int d=dx>dy?dx+dy/2:dy+dx/2; return d?d:1; }
static void en_move(En*e,int dx,int dy){
    while(dx){ int s=sgn(dx); if(ewalk(e,e->x+s,e->y)) e->x+=s; else break; dx-=s; }
    while(dy){ int s=sgn(dy); if(ewalk(e,e->x,e->y+s)) e->y+=s; else break; dy-=s; }
}
static void en_update(En*e){
    if(!e->alive) return;
    if(e->hurt) e->hurt--;
    int sz=e->type==E_BOSS?32:16;
    int ex=e->x+sz/2, ey=e->y+sz/2, dx=pl_x+8-ex, dy=pl_y+20-ey;
    if(e->kb){ e->kb--; en_move(e,e->kx,e->ky); }
    else switch(e->type){
     case E_DRIP:
        if(!e->vx){ e->vx=(rnd()&1)?1:-1; e->vy=(rnd()&1)?1:-1; }
        if(!ewalk(e,e->x+e->vx,e->y)) e->vx=-e->vx; else e->x+=e->vx;
        if(!ewalk(e,e->x,e->y+e->vy)) e->vy=-e->vy; else e->y+=e->vy;
        break;
     case E_TV:
        if(iabs(dx)+iabs(dy)<90){ e->st=1; if(t&1) en_move(e,sgn(dx),sgn(dy)); }
        else { e->st=0; if(e->tm==0){ e->vx=(s8)((int)(rnd()%3)-1); e->vy=(s8)((int)(rnd()%3)-1); e->tm=(u8)(40+rnd()%40);} else e->tm--; if(t&1) en_move(e,e->vx,e->vy); }
        break;
     case E_TAPE:
        if(e->tm) e->tm--;
        if(e->st==0){ if(!e->tm){ e->st=1; e->tm=28; } }
        else if(e->st==1){ if(!e->tm){ int d=dist_approx(dx,dy); e->vx=(s8)(dx*3/d); e->vy=(s8)(dy*3/d); e->st=2; e->tm=26; } }
        else if(e->st==2){ en_move(e,e->vx,e->vy); if(!e->tm){ e->st=3; e->tm=40; } }
        else { if(!e->tm){ e->st=0; e->tm=(u8)(30+rnd()%40);} }
        break;
     case E_BOSS: {
        int ph2 = e->hp<=8;
        if(e->st==0){
            if((t&1)&&iabs(dx)>6) en_move(e,sgn(dx),0);
            if(e->tm) e->tm--;
            else {
                int d=dist_approx(dx,dy);
                for(int k=-1;k<=1;k++){ int vx=dx*16/d+k*(-dy)*5/d, vy=dy*16/d+k*dx*5/d; prj_add(ex-3,ey,vx,vy); }
                sfx(SF_SWING);
                e->kb=0; e->tm=(u8)(ph2?70:100);
                e->vy++; if(e->vy>=3){ e->vy=0; e->st=1; e->tm=36; }
            }
        } else if(e->st==1){ if(!e->tm){ e->st=2; e->tm=34; e->vx=(s8)(dx>0?3:-3); } else e->tm--; }
        else if(e->st==2){ if(!ewalk(e,e->x+e->vx,e->y)) e->vx=-e->vx; en_move(e,e->vx,0); if(e->tm) e->tm--; else { e->st=3; e->tm=50; } }
        else { if(e->tm) e->tm--; else { e->st=0; e->tm=40; } }
        break; }
    }
    /* dano por contacto */
    if(!pl_inv && ov(pl_x+3,pl_y+10,10,20,e->x+2,e->y+2,sz-4,sz-4)) pl_damage(e->type==E_BOSS?2:1,ex,ey);
}
static void draw_enemy(const En*e){
    int x=e->x,y=e->y; int fl=(e->hurt>>1)&1;
    switch(e->type){
     case E_DRIP: { int b=bobt[((t>>2)+x)&15]/2;
        disc(x+8,y+10,7,fl?C_WHITE:C_BLUED); disc(x+8,y+10,6,fl?C_WHITE:C_LBLUE); rect(x+6,y+1+b,4,5,fl?C_WHITE:C_BLUED); rect(x+7,y+2+b,2,4,fl?C_WHITE:C_LBLUE);
        rect(x+5,y+9,2,3,C_INK); rect(x+10,y+9,2,3,C_INK); px(x+5,y+7,C_WHITE); px(x+6,y+7,C_WHITE); } break;
     case E_TV:
        rect(x+5,y,1,4,C_GRAY); rect(x+10,y,1,4,C_GRAY); rect(x+1,y+3,14,10,fl?C_WHITE:C_INK);
        rect(x+3,y+5,10,6,e->st?C_RED:C_GRAYD); for(int i=0;i<10;i++) px(x+3+rnd()%10,y+5+rnd()%6,(rnd()&1)?C_WHITE:C_GRAY);
        rect(x+3,y+13,2,3,C_INK); rect(x+11,y+13,2,3,C_INK); break;
     case E_TAPE:
        rect(x,y+3,16,11,fl?C_WHITE:C_INK); rect(x+1,y+4,14,9,(e->st==1&&((t>>2)&1))?C_RED:(fl?C_WHITE:C_GRAYD));
        rect(x+2,y+5,12,4,C_PALEY); disc(x+5,y+11,2,C_INK); disc(x+11,y+11,2,C_INK); px(x+5,y+11,C_WHITE); px(x+11,y+11,C_WHITE);
        rect(x+4,y+14,1,2+(t>>3&1),C_BRIGHT); rect(x+12,y+14,1,2,C_BRIGHT); break;
     case E_BOSS: {
        int ph2=e->hp<=8;
        for(int r=0;r<8;r++){ int w=2+r*2; rect(x+16-w/2,y-8+r,w,1,(r&1)?C_YEL:C_MAG); }
        rect(x,y,32,32,C_INK); rect(x+1,y+1,30,30,fl?C_WHITE:C_GRAYD); rect(x+3,y+3,26,9,C_INK);
        txt("0:00",x+5,y+3,((t>>4)&1)?C_RED:C_REDD,1);
        disc(x+9,y+21,6,C_PALEY); disc(x+23,y+21,6,C_PALEY);
        { int a=(t>>2)&7; static const s8 ox[8]={0,1,2,1,0,-1,-2,-1}, oy[8]={-2,-1,0,1,2,1,0,-1};
          disc(x+9+ox[a],y+21+oy[a],2,ph2?C_RED:C_INK); disc(x+23+ox[a],y+21+oy[a],2,ph2?C_RED:C_INK); }
        rect(x+9,y+29,14,3,C_INK); rect(x+11,y+30,1,3+(e->st==2),C_BRIGHT);
        if(e->st==1&&((t>>2)&1)) frame(x-2,y-2,36,36,C_RED);
        } break;
    }
}

/* ---------------- logica de juego ---------------- */
static int ending;
static void load_room(int r,int sx,int sy){
    room=r; pl_x=sx; pl_y=sy; pl_kb=0; pl_atk=0; pl_spin=0; pl_charge=0;
    spawn_room(); music_set(ROOMS[r].song); banner=150;
    if(!seen[r]){ seen[r]=1; pend_ev= r==0?1: r==1?2: r==4?3: 0; }
}
static void new_game(void){
    for(int i=0;i<5;i++){cleared[i]=0;seen[i]=0;} for(int i=0;i<3;i++) talked[i]=0;
    has_sword=0; boss_dead=0; ending=0; pl_hp=6; pl_inv=0; pl_dir=0; pl_fr=0; dl_on=0; pend_ev=0;
    for(int i=0;i<6;i++) fxs[i].t=0;
    mode=M_PLAY; load_room(0,112,92); tr=2; trc=12;
}
static void run_pending(void){
    int p=pend_ev; pend_ev=0;
    if(p==1) say(TXT_INTRO,AF_NONE,0,0);
    else if(p==2) say(TXT_VOID1,AF_NONE,0,0);
    else if(p==3) say(TXT_BOSS1,AF_NONE,0,0);
}
static void npc_talk(int id){
    if(!talked[id]){ talked[id]=1; say(NPC1[id], id==0?AF_SWORD:(id==2?AF_HEAL:AF_NONE),0,0); }
    else say(NPC2[id], id==2?AF_HEAL:AF_NONE,0,0);
}
static void interact(void){
    int qx=pl_x+3,qy=pl_y+26,qw=10,qh=6;
    switch(pl_dir){ case 0: qh+=10; break; case 1: qy-=10; qh+=10; break; case 2: qx-=10; qw+=10; break; default: qw+=10; }
    const Room*R=&ROOMS[room];
    for(int i=0;i<R->no;i++){ const Obj*o=&R->o[i];
        if(!ov(qx,qy,qw,qh,o->x-4,o->y-4,o->w+8,o->h+8)) continue;
        if(o->kind==K_NPC){ npc_talk(o->arg); return; }
        if(!o->txt) continue;
        if(o->to>=0) say(o->txt,AF_GOTO,o->to,o->sx|(o->sy<<8)); else say(o->txt,AF_NONE,0,0);
        sfx(SF_SEL); return;
    }
}
static void sword_box(int*x,int*y,int*w,int*h){
    switch(pl_dir){
     case 0: *x=pl_x+1; *y=pl_y+24; *w=14; *h=16; break;
     case 1: *x=pl_x+1; *y=pl_y-2;  *w=14; *h=18; break;
     case 2: *x=pl_x-14;*y=pl_y+12; *w=18; *h=16; break;
     default:*x=pl_x+12;*y=pl_y+12; *w=18; *h=16; break; }
}
static void hit_enemies(int x,int y,int w,int h,int dmg){
    for(int i=0;i<6;i++){ En*e=&en[i]; if(!e->alive||e->hurt) continue;
        int sz=e->type==E_BOSS?32:16;
        if(ov(x,y,w,h,e->x,e->y,sz,sz)){
            int dx=sgn(e->x+sz/2-(pl_x+8)), dy=sgn(e->y+sz/2-(pl_y+20));
            en_hurt(e,dmg,dx,dy); } }
}
static void play_update(void){
    if(tr==1){ if(++trc>=12){ load_room(tr_to,tr_sx,tr_sy); tr=2; trc=12; } return; }
    if(tr==2){ if(--trc<=0){ tr=0; trc=0; if(pend_ev) run_pending(); } return; }
    if(dl_on){ dialog_update(); return; }
    if(kp&K_START){ mode=M_PAUSE; sfx(SF_SEL); return; }
    if(banner>0) banner--;
    if(pl_inv) pl_inv--;
    int moving=0;
    if(pl_kb>0){ pl_kb--; pl_move(pl_kx,pl_ky); }
    else if(!pl_atk&&!pl_spin){
        int dx=0,dy=0;
        if(keys&K_LEFT)dx=-1; else if(keys&K_RIGHT)dx=1;
        if(keys&K_UP)dy=-1; else if(keys&K_DOWN)dy=1;
        if(dx&&!dy) pl_dir=dx<0?2:3; else if(dy&&!dx) pl_dir=dy<0?1:0;
        else if(dx&&dy){ if(kp&(K_LEFT|K_RIGHT)) pl_dir=dx<0?2:3; else if(kp&(K_UP|K_DOWN)) pl_dir=dy<0?1:0; }
        int sp=(pl_charge>0)?1:2;
        if(dx||dy){ moving=1; pl_move(dx*sp,dy*sp); }
        if(kp&K_A) interact();
    }
    pl_fr = moving ? ((t/6)&3) : 0;
    if(pl_atk>0){ pl_atk--; if(pl_atk>=3&&pl_atk<=11){ int x,y,w,h; sword_box(&x,&y,&w,&h); hit_enemies(x,y,w,h,1); } }
    if(pl_spin>0){ pl_spin--; hit_enemies(pl_x-18,pl_y-4,52,52,2); }
    if(has_sword){
        if((kp&K_B)&&!pl_atk&&!pl_spin){ pl_atk=14; pl_charge=0; sfx(SF_SWING); }
        else if(keys&K_B){ if(!pl_atk&&!pl_spin){ pl_charge++; if(pl_charge==40) sfx(SF_SEL); } else pl_charge=0; }
        else { if(pl_charge>=40&&!pl_atk){ pl_spin=20; sfx(SF_SPIN); } pl_charge=0; }
    }
    if(mode!=M_PLAY||dl_on) return;
    /* enemigos */
    int alive=0;
    for(int i=0;i<6;i++){ en_update(&en[i]); if(en[i].alive) alive++; }
    for(int i=0;i<8;i++) if(prj[i].on){ Prj*p=&prj[i];
        p->x+=p->vx; p->y+=p->vy; if(--p->life==0||p->x<0||p->y<0||p->x>SW*8||p->y>SH*8){ p->on=0; continue; }
        if(!pl_inv&&ov(pl_x+3,pl_y+10,10,20,p->x/8-2,p->y/8-2,5,5)){ pl_damage(1,p->x/8,p->y/8); p->on=0; } }
    for(int i=0;i<4;i++) if(drops[i].on){ if(--drops[i].life==0){drops[i].on=0;continue;}
        if(ov(pl_x+3,pl_y+14,10,16,drops[i].x,drops[i].y,8,7)){ drops[i].on=0; if(pl_hp<6) pl_hp+=2; if(pl_hp>6) pl_hp=6; sfx(SF_PICK); } }
    for(int i=0;i<6;i++) if(fxs[i].t) fxs[i].t--;
    if(mode!=M_PLAY) return;
    if((room==2||room==3)&&!cleared[room]&&alive==0){ cleared[room]=1; sfx(SF_PICK); say(TXT_CLEAR,AF_NONE,0,0); return; }
    if(room==4&&boss_dead&&!ending){ ending=1; say(TXT_WIN,AF_END,0,0); return; }
    /* salidas */
    const Room*R=&ROOMS[room];
    for(int i=0;i<R->ne;i++){ const Exit*x=&R->e[i];
        if(!ov(pl_x+3,pl_y+26,10,6,x->x,x->y,x->w,x->h)) continue;
        if(x->lock==1&&!cleared[room]){ say(TXT_LOCK1,AF_NONE,0,0); pl_x+=(x->x<100)?10:-10; return; }
        if(x->lock==2&&!has_sword){ say(TXT_LOCK2,AF_NONE,0,0); pl_x+=(x->x<100)?10:-10; return; }
        goto_room(x->to,x->sx,x->sy); return; }
}

/* ---------------- dibujo del juego ---------------- */
typedef struct { int key; u8 type,idx; } Dr;
static Dr drl[24]; static int ndr;
static void dr_add(int key,int type,int idx){
    int i=ndr++; while(i>0&&drl[i-1].key>key){ drl[i]=drl[i-1]; i--; }
    drl[i].key=key; drl[i].type=(u8)type; drl[i].idx=(u8)idx;
}
static void draw_sword(void){
    int cx=pl_x+8, cy=pl_y+20;
    if(pl_atk>0){ int s=14-pl_atk; int o=-10+s*2;
        switch(pl_dir){
         case 0: rect(cx+o-1,cy,3,14,C_LAV); rect(cx+o-1,cy+12,3,3,C_WHITE); rect(cx+o-2,cy,5,1,C_YEL); break;
         case 1: rect(cx-o-1,cy-18,3,14,C_LAV); rect(cx-o-1,cy-18,3,3,C_WHITE); rect(cx-o-2,cy-5,5,1,C_YEL); break;
         case 2: rect(cx-18,cy+o-1,14,3,C_LAV); rect(cx-18,cy+o-1,3,3,C_WHITE); rect(cx-5,cy+o-2,1,5,C_YEL); break;
         default:rect(cx+4,cy+o-1,14,3,C_LAV); rect(cx+15,cy+o-1,3,3,C_WHITE); rect(cx+4,cy+o-2,1,5,C_YEL); }
    }
    if(pl_spin>0){ static const s8 sx[16]={22,20,16,8,0,-8,-16,-20,-22,-20,-16,-8,0,8,16,20};
        static const s8 sy[16]={0,8,16,20,22,20,16,8,0,-8,-16,-20,-22,-20,-16,-8};
        int a=(pl_spin*2)&15;
        for(int k=0;k<8;k++){ int i=(a+k)&15; rect(cx+sx[i]-1,cy+sy[i]-1,3,3,k>5?C_WHITE:C_LAV); } }
    if(pl_charge>=40&&((t>>1)&1)){ rect(pl_x+7,pl_y-5,2,2,C_WHITE); rect(pl_x+6,pl_y-4,4,1,C_WHITE); }
}
static void render_play(void){
    const Room*R=&ROOMS[room];
    switch(R->theme){ case TH_BED: bg_bed(); break; case TH_VOID: bg_void(); break; case TH_POOL: bg_pool(); break; case TH_MALL: bg_mall(); break; default: bg_party(); }
    for(int i=0;i<R->no;i++) if(!R->o[i].solid) draw_obj(&R->o[i]);
    for(int i=0;i<4;i++) if(drops[i].on&&(drops[i].life>60||((t>>2)&1))) heart(drops[i].x,drops[i].y,2);
    ndr=0;
    for(int i=0;i<R->no;i++) if(R->o[i].solid) dr_add(R->o[i].y+R->o[i].h,0,i);
    dr_add(pl_y+32,1,0);
    for(int i=0;i<6;i++) if(en[i].alive) dr_add(en[i].y+(en[i].type==E_BOSS?32:16),2,i);
    for(int i=0;i<ndr;i++){
        if(drl[i].type==0) draw_obj(&R->o[drl[i].idx]);
        else if(drl[i].type==2) draw_enemy(&en[drl[i].idx]);
        else { if(!(pl_inv&&((pl_inv>>2)&1))) hero_blit(pl_x,pl_y,gender,pl_dir,pl_fr,1); draw_sword(); }
    }
    for(int i=0;i<8;i++) if(prj[i].on){ int x=prj[i].x/8,y=prj[i].y/8; disc(x,y,3,C_MAG); disc(x,y,2,C_WHITE); }
    for(int i=0;i<6;i++) if(fxs[i].t){ int r=(16-fxs[i].t); disc(fxs[i].x,fxs[i].y,r,(fxs[i].t>8)?C_WHITE:C_LAV); if(r>3) disc(fxs[i].x,fxs[i].y,r-3,C_VOID); }
    /* HUD */
    for(int i=0;i<3;i++) heart(5+i*9,5,pl_hp>=2*(i+1)?2:(pl_hp==2*i+1?1:0));
    txt_shadow(">PLAY",4,15,C_WHITE,1);
    if(banner>0&&banner<140){ int w=txt_n(R->name)*6+10; int bx=(SW-w)/2; draw_box(bx,16,w,14); txt(R->name,bx+5,19,C_PALEY,1); }
    if(tr){ int h=(trc*8+11)/12; for(int y=0;y<SH;y+=8) rect(0,y,SW,h,C_VOID); }
    if(dl_on&&!tr) dialog_draw(pl_y>84);
}

/* ---------------- menus ---------------- */
static void osd_time(char*b){ int s=t/60; int m=(s/60)%60; s%=60; b[0]='0';b[1]=':';b[2]='0'+m/10;b[3]='0'+m%10;b[4]=':';b[5]='0'+s/10;b[6]='0'+s%10;b[7]=0; }
static void static_noise(int n){ for(int i=0;i<n;i++) px(rnd()%SW,rnd()%SH,(rnd()&1)?C_WHITE:C_GRAY); }

static void title_frame(void){
    if(kp&(K_UP|K_DOWN)){ menu_sel^=1; sfx(SF_BLIP); }
    if(kp&K_A){ sfx(SF_SEL); if(menu_sel==0){ mode=M_SEL; gender=0; } else mode=M_CRED; }
    rect(0,0,SW,SH,C_VOID);
    for(int i=0;i<40;i++) rect(rnd()%SW,rnd()%SH,8+rnd()%30,1,C_PUR);
    for(int y=0;y<SH;y+=16) rect(0,y,SW,1,C_INK);
    int j=((t%120)<4)?2:0;
    txt("CANAL 0",31-1+j,22,C_CYAN,3); txt("CANAL 0",31+1-j,22,C_MAG,3); txt("CANAL 0",31,22,C_WHITE,3);
    txt("NOCHE DE LLUVIA",32,50,C_LAV,2);
    const char*it[2]={"NUEVO JUEGO","CRÉDITOS"};
    for(int i=0;i<2;i++){ txt_shadow(it[i],36,90+i*16,i==menu_sel?C_YEL:C_LAV,1); if(i==menu_sel&&((t>>4)&1)) txt_shadow(">",24,90+i*16,C_YEL,1); }
    hero_blit(150,80,0,0,0,2); hero_blit(190,80,1,0,0,2);
    char b[8]; osd_time(b);
    txt_shadow(">PLAY",8,6,C_WHITE,1); txt_shadow("SP",172,6,C_WHITE,1); txt_shadow(b,188,6,C_WHITE,1);
    txt_shadow("A: ACEPTAR",8,148,C_GRAY,1);
}
static void cred_frame(void){
    if(kp&(K_A|K_B)) mode=M_TITLE;
    rect(0,0,SW,SH,C_VOID); for(int i=0;i<30;i++) rect(rnd()%SW,rnd()%SH,8+rnd()%30,1,C_PUR);
    txtc("CANAL 0: NOCHE DE LLUVIA",20,C_WHITE,1);
    txtc("UN JUEGO DREAMCORE PARA GBA",40,C_LAV,1);
    txtc("CHIPTUNE: PSG DE LA GBA",56,C_LAV,1);
    txtc("MIRA DIRECTO A LA PANTALLA",72,C_LAV,1);
    txtc("NO CAMBIES DE CANAL",88,C_CYAN,1);
    txtc("B: VOLVER",140,C_GRAY,1);
}
static void sel_frame(void){
    if(kp&(K_LEFT|K_RIGHT)){ gender^=1; sfx(SF_BLIP); }
    if(kp&K_B){ mode=M_TITLE; }
    if(kp&K_A){ sfx(SF_SEL); mode=M_NAME; nlen=0; name_cur=0; }
    rect(0,0,SW,SH,C_VOID); for(int i=0;i<30;i++) rect(rnd()%SW,rnd()%SH,8+rnd()%30,1,C_PUR);
    txtc("ELIGE A TU PERSONAJE",10,C_WHITE,1);
    for(int g=0;g<2;g++){
        int x=g?148:44, sel=(g==gender);
        if(sel){ frame(x-8,38,48,86,C_YEL); frame(x-7,39,46,84,C_YELD); }
        hero_blit(x,48+(sel?bobt[(t>>2)&15]:0),g,sel?0:0,sel?((t>>3)&1)*0:0,2);
        txtc("",0,0,1);
        txt_shadow(g?"CHICA":"CHICO",x+(g?4:4),128,sel?C_YEL:C_GRAY,1);
    }
    txtc("< >  ELEGIR    A: OK",146,C_LAV,1);
}
static const u16 GRID_SPEC=0;
static u16 grid_code(int i){
    if(i<26) return (u16)('A'+i);
    if(i==26) return (u16)('N'|(2<<8));
    return (u16)(0xF000+i);   /* 27 borrar, 28 fin, 29 espacio */
}
static void name_frame(void){
    int cols=10, row=name_cur/cols, col=name_cur%cols;
    if(kp&K_LEFT){ col=(col+cols-1)%cols; }
    if(kp&K_RIGHT){ col=(col+1)%cols; }
    if(kp&K_UP){ row=(row+2)%3; }
    if(kp&K_DOWN){ row=(row+1)%3; }
    name_cur=row*cols+col;
    if(kp&(K_LEFT|K_RIGHT|K_UP|K_DOWN)) sfx(SF_BLIP);
    if(kp&K_B){ if(nlen>0){ nlen--; } else mode=M_SEL; }
    int done=0;
    if(kp&K_A){ u16 c=grid_code(name_cur);
        if(c==0xF000+27){ if(nlen>0) nlen--; }
        else if(c==0xF000+28) done=1;
        else if(nlen<8){ pname[nlen++]= (c==0xF000+29)?' ':c; sfx(SF_SEL); } }
    if(kp&K_START) done=1;
    if(done){
        if(nlen==0){ const char*d=gender?"LUNA":"NICO"; for(int i=0;i<4;i++) pname[nlen++]=(u16)d[i]; }
        new_game(); return; }
    rect(0,0,SW,SH,C_VOID); for(int i=0;i<30;i++) rect(rnd()%SW,rnd()%SH,8+rnd()%30,1,C_PUR);
    txtc("¿CÓMO TE LLAMAS?",10,C_WHITE,1);
    hero_blit(112,22,gender,0,0,1);
    for(int i=0;i<8;i++){ int x=80+i*10; rect(x,62,8,1,C_LAV); if(i<nlen) gl(pname[i],x+1,52,C_YEL,1); }
    for(int i=0;i<30;i++){ int r=i/cols,c=i%cols; int x=20+c*20,y=76+r*16; u16 g=grid_code(i);
        if(i==name_cur) rect(x-2,y-3,20,13,C_PUR2);
        if(g<0xF000) gl(g,x+5,y-1,i==name_cur?C_YEL:C_WHITE,1);
        else if(g==0xF000+27) txt("DEL",x,y-1,i==name_cur?C_YEL:C_LAV,1);
        else if(g==0xF000+28) txt("FIN",x,y-1,i==name_cur?C_YEL:C_CYAN,1);
        else txt("_",x+5,y-1,i==name_cur?C_YEL:C_LAV,1); }
    txtc("A: ELEGIR  B: BORRAR  START: FIN",138,C_GRAY,1);
}
static void over_frame(void){
    if(t>60&&(kp&K_A)){ pl_hp=6; pl_inv=0; mode=M_PLAY; for(int i=0;i<5;i++) ending=0; load_room(1,16,82); tr=2; trc=12; }
    rect(0,0,SW,SH,C_VOID); static_noise(300);
    txtc("LA CINTA SE ATASCÓ",56,C_RED,2);
    if(t>60) txtc("A: REBOBINAR",100,C_WHITE,1);
}
static void end_frame(void){
    if(t>240&&(kp&K_A)){ mode=M_TITLE; menu_sel=0; music_set(SG_TITLE); }
    rect(0,0,SW,SH,C_VOID); int n=t<90?900-t*8:60; static_noise(n>0?n:60);
    if(t>90){ txtc("FIN",44,C_WHITE,3); }
    if(t>150){ char b[24]; (void)b; txtc("GRACIAS POR JUGAR",90,C_LAV,1); }
    if(t>210){ txtc("O QUIZÁS NO...",108,C_GRAY,1); }
    if(t>240&&((t>>4)&1)) txtc("A: VOLVER AL TÍTULO",140,C_GRAY,1);
}
static void pause_frame(void){
    if(kp&K_START){ mode=M_PLAY; sfx(SF_SEL); }
    if(kp&K_SEL){ mode=M_TITLE; music_set(SG_TITLE); }
    render_play();
    draw_box(50,40,140,78);
    txtc("PAUSA",48,C_YEL,1);
    txt("NOMBRE:",60,64,C_LAV,1); for(int i=0;i<nlen;i++) gl(pname[i],110+i*6,64,C_WHITE,1);
    txt("LUGAR:",60,76,C_LAV,1); txt(ROOMS[room].name,102,76,C_WHITE,1);
    txt("ESPADA:",60,88,C_LAV,1); txt(has_sword?"SI":"NO",108,88,C_WHITE,1);
    txtc("START: SEGUIR  SELECT: TITULO",104,C_GRAY,1);
}

/* ---------------- efecto VHS + salida a pantalla ---------------- */
static int wpage=1;
FAST static void present(void){
    int gy=-100,gh=0,gs=0;
    if(gl_t>0){ gy=gl_y+(12-gl_t)*3; gh=6; gs=(gl_t&1)?1:2; }
    const u32*src=(const u32*)fb;
    volatile u32*dst=VRAM+(wpage?0x2800:0);
    for(int y=0;y<SH;y++){
        u32 add=(y&1)?0x40404040u:0;
        const u32*s=src+y*60; volatile u32*d=dst+y*60;
        if(y>=gy&&y<gy+gh){ for(int i=0;i<60;i++){ int k=i+gs; if(k>=60)k-=60; d[i]=s[k]+add; } }
        else { for(int i=0;i<60;i++) d[i]=s[i]+add; }
    }
}
static void vsync(void){
#ifndef HOST
    while(REG_VCOUNT>=160); while(REG_VCOUNT<160);
#endif
}

void game_init(void){
    for(int i=0;i<128;i++) fidx[i]=-1;
    for(int i=0;FCH[i];i++) fidx[(int)FCH[i]]=(s8)i;
    pal_init(); snd_init();
    REG_DISPCNT=0x0404;
    mode=M_TITLE; menu_sel=0; t=0; gender=0; keys=0; kp=0;
    music_set(SG_TITLE);
    (void)GRID_SPEC;
}
void game_frame(void){
    t++; poll();
    switch(mode){
     case M_TITLE: title_frame(); break;
     case M_CRED: cred_frame(); break;
     case M_SEL: sel_frame(); break;
     case M_NAME: name_frame(); break;
     case M_PLAY: play_update(); if(mode==M_PLAY) render_play(); else if(mode==M_PAUSE) pause_frame(); break;
     case M_PAUSE: pause_frame(); break;
     case M_OVER: over_frame(); break;
     case M_END: end_frame(); break;
    }
    if(mode==M_TITLE&&cursong!=SG_TITLE) music_set(SG_TITLE);
    music_tick();
    /* VHS: motas y banda de tracking */
    if(mode!=M_END&&mode!=M_OVER) for(int i=0;i<6;i++) px(rnd()%SW,rnd()%SH,C_GRAY);
    if(gl_t>0) gl_t--; else if(rnd()%420==0){ gl_t=12; gl_y=rnd()%140; }
    present();
    vsync();
    REG_DISPCNT=(u16)(0x0404|(wpage?0x10:0));
    wpage^=1;
}
