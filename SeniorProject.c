/*
Read from controllers with pad_poll().
We also demonstrate sprite animation by cycling among
several different metasprites, which are defined using macros.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h> 
// include NESLIB header
#include "neslib.h"

// include CC65 NES Header (PPU)
#include <nes.h>
#define TILE 0xd4 
#define TILE1 0xd0 
#define ATTR 3
#define ATTR1 1
//cup game
#define TILEC 0xc4 //cup
#define TILEB 0xc8 //ball
#define TILEA 0x1f//arrow
#define TILEE 0x00//empty
#define TILERR 0xf4//door
#define ATTRC 0
#define ATTRC2 3

// link the pattern table into CHR ROM
//#link "chr_generic.s"

// number of actors (4 h/w sprites each)
#define NUM_ACTORS 14

#include "apu.h"
//#link "apu.c"

// actor x/y positions
byte actor_x[NUM_ACTORS];
byte actor_y[NUM_ACTORS];
// actor x/y deltas per frame (signed)
sbyte actor_dx[NUM_ACTORS];
sbyte actor_dy[NUM_ACTORS];
int test=0;

///// METASPRITES
  const unsigned char metasprite[]={    
        0,      0,      TILE+0,   ATTR, 
        0,      8,      TILE+1,   ATTR, 
        8,      0,      TILE+2,   ATTR, 
        8,      8,      TILE+3,   ATTR, 
        128};
  const unsigned char metasprite2[]={    
        0,      0,      TILE1+0,   ATTR1, 
        0,      8,      TILE1+1,   ATTR1, 
        8,      0,      TILE1+2,   ATTR1, 
        8,      8,      TILE1+3,   ATTR1, 
        128};
//cup game
  const unsigned char metaspriteC[]={    //cups
        0,      0,      TILEC+0,   ATTRC2, 
        0,      8,      TILEC+1,   ATTRC2, 
        8,      0,      TILEC+2,   ATTRC2, 
        8,      8,      TILEC+3,   ATTRC2, 
        128};
  
  const unsigned char metaspriteB[]={   //ball
        0,      0,      TILEB+0,   ATTRC, 
        0,      8,      TILEB+1,   ATTRC, 
        8,      0,      TILEB+2,   ATTRC, 
        8,      8,      TILEB+3,   ATTRC, 
        128};
  const unsigned char metaspriteA[]={   //arrow
        0,      0,      TILEA,   ATTRC,
        0,      0,      TILEE,   ATTRC,
        0,      0,      TILEE,   ATTRC,
        0,      0,      TILEE,   ATTRC,
        128};
  const unsigned char metaspriteD[]={   //door
        0,      0,      TILERR+0,   ATTRC, 
        0,      8,      TILERR+1,   ATTRC, 
        8,      0,      TILERR+2,   ATTRC, 
        8,      8,      TILERR+3,   ATTRC, 
        128};

// define a 2x2 metasprite
#define DEF_METASPRITE_2x2(name,code,pal)\
const unsigned char name[]={\
        0,      0,      (code)+0,   pal, \
        0,      8,      (code)+1,   pal, \
        8,      0,      (code)+2,   pal, \
        8,      8,      (code)+3,   pal, \
        128};

// define a 2x2 metasprite, flipped horizontally
#define DEF_METASPRITE_2x2_FLIP(name,code,pal)\
const unsigned char name[]={\
        8,      0,      (code)+0,   (pal)|OAM_FLIP_H, \
        8,      8,      (code)+1,   (pal)|OAM_FLIP_H, \
        0,      0,      (code)+2,   (pal)|OAM_FLIP_H, \
        0,      8,      (code)+3,   (pal)|OAM_FLIP_H, \
        128};

DEF_METASPRITE_2x2(playerRStand, 0xd8, 0);
DEF_METASPRITE_2x2(playerRRun1, 0xdc, 0);
DEF_METASPRITE_2x2(playerRRun2, 0xe0, 0);
DEF_METASPRITE_2x2(playerRRun3, 0xe4, 0);
DEF_METASPRITE_2x2(playerRJump, 0xe8, 0);
DEF_METASPRITE_2x2(playerRClimb, 0xec, 0);
DEF_METASPRITE_2x2(playerRSad, 0xf0, 0);

DEF_METASPRITE_2x2_FLIP(playerLStand, 0xd8, 0);
DEF_METASPRITE_2x2_FLIP(playerLRun1, 0xdc, 0);
DEF_METASPRITE_2x2_FLIP(playerLRun2, 0xe0, 0);
DEF_METASPRITE_2x2_FLIP(playerLRun3, 0xe4, 0);
DEF_METASPRITE_2x2_FLIP(playerLJump, 0xe8, 0);
DEF_METASPRITE_2x2_FLIP(playerLClimb, 0xec, 0);
DEF_METASPRITE_2x2_FLIP(playerLSad, 0xf0, 0);

bool quit=false;
int exitvar=0;

const unsigned char* const playerRunSeq[16] = {
  playerLRun1, playerLRun2, playerLRun3, 
  playerLRun1, playerLRun2, playerLRun3, 
  playerLRun1, playerLRun2,
  playerRRun1, playerRRun2, playerRRun3, 
  playerRRun1, playerRRun2, playerRRun3, 
  playerRRun1, playerRRun2,
};

/*{pal:"nes",layout:"nes"}*/
const char PALETTE[32] = { 
  0xb1,			// screen color

  0x05,0x6,0x31,0x0,	// background palette 0
  0x05,0x20,0x2c,0x0,	// background palette 1
  0x05,0x10,0x20,0x0,	// background palette 2
  0x06,0x16,0x26,0x0,	// background palette 3

  0x02,0x32,0x24,0x0,	// sprite palette 0
  0x02,0x35,0x24,0x0,	// sprite palette 1
  0x0d,0x2d,0x3a,0x0,	// sprite palette 2
  0x05,0xa5,0xb4	// sprite palette 3
};

void win_chime() 
{
  APU_ENABLE(ENABLE_PULSE0);
  APU_PULSE_DECAY(0, 782, 0, 13, 8);
  APU_PULSE_SWEEP(0, 1, 4, 1);
  APU_PULSE_DECAY(1, 993, 64, 6, 136);
  APU_PULSE_SWEEP(1, 7, 1, 0);
}
void start_ding() 
{
  APU_ENABLE(ENABLE_PULSE1);
  APU_PULSE_DECAY(1, 190, 64, 10, 18);
  APU_PULSE_SWEEP(1, 2, 3, 0);
}
void lose_static() 
{
  APU_ENABLE(ENABLE_NOISE);
  APU_NOISE_DECAY(8|7, 13, 10);
}
void mus1() 
{
  APU_ENABLE(ENABLE_PULSE1);
  APU_PULSE_DECAY(1, 1828, 128, 7, 6);
  APU_PULSE_SWEEP(1, 7, 5, 1);
}
void mus2() 
{
  APU_ENABLE(ENABLE_PULSE0);
  APU_PULSE_DECAY(0, 291, 128, 8, 3);
  APU_PULSE_SWEEP(0, 7, 1, 1);
}
void mus3() 
{
  APU_ENABLE(ENABLE_PULSE1);
  APU_PULSE_DECAY(1, 86, 192, 6, 14);
  APU_PULSE_SWEEP(1, 2, 2, 0);
}


// setup PPU and tables
void setup_graphics() {
  // clear sprites
  oam_hide_rest(0);
  // set palette colors
  pal_all(PALETTE);
  // turn on PPU
  ppu_on_all();
}

int seed;
void display_cups()
{
  int i;
  char oam_id;
  setup_graphics();
 
  actor_x[8]=20;
  actor_y[8]=80;
  actor_x[9]=40;
  actor_y[9]=180; 
  actor_x[10]=100;
  actor_y[10]=180;
  actor_x[11]=160;
  actor_y[11]=180;
  
  oam_id = oam_meta_spr(actor_x[8], actor_y[8], oam_id, metaspriteA);
  for(i=9;i<12;i++)
  {
    oam_id = oam_meta_spr(actor_x[i], actor_y[i], oam_id, metaspriteC);
  }
}

void shuffle()
{
  char oam_id;
  int j;
  int i;
  setup_graphics();
  oam_id = oam_meta_spr(actor_x[8], actor_y[8], oam_id, metaspriteA);
  oam_id = oam_meta_spr(actor_x[10], actor_y[10], oam_id, metaspriteB);

  for(i=0;i<74;i++)
  {
    ppu_wait_frame();
  }
  
  for(i=0;i<4;i++)
  {
    for(j=0;j<12;j++)
    {
      oam_id = oam_meta_spr(actor_x[8], actor_y[8], oam_id, metaspriteA);
      oam_id = oam_meta_spr(actor_x[9], actor_y[9], oam_id, metaspriteC);
      actor_x[9] += 5;
      oam_id = oam_meta_spr(actor_x[10], actor_y[10], oam_id, metaspriteC);
      oam_id = oam_meta_spr(actor_x[11], actor_y[11], oam_id, metaspriteC);
      actor_x[11] -= 5;
      ppu_wait_frame();
      //oam_clear();
    }
    for(j=0;j<15;j++)
    {
      oam_id = oam_meta_spr(actor_x[8], actor_y[8], oam_id, metaspriteA);
      oam_id = oam_meta_spr(actor_x[9], actor_y[9], oam_id, metaspriteC);
      actor_x[9] -= 4;
      oam_id = oam_meta_spr(actor_x[10], actor_y[10], oam_id, metaspriteC);
      oam_id = oam_meta_spr(actor_x[11], actor_y[11], oam_id, metaspriteC);
      actor_x[11] += 4;
      ppu_wait_frame();
      //oam_clear();
    }
  }
}

void display_ball(int x)
{
  int i;
  char oam_id;
  setup_graphics();
 
  oam_id = oam_meta_spr(actor_x[8], actor_y[8], oam_id, metaspriteA); //shows arrow
  oam_id = oam_meta_spr(actor_x[x], actor_y[x], oam_id, metaspriteB); //shows ball
  for(i=9;i<12;i++)
  {
    if(i==x)
    {
      continue;   
    }
    oam_id = oam_meta_spr(actor_x[i], actor_y[i], oam_id, metaspriteC); //shows empty cups
  }
  
  for(i=0;i<74;i++)
  {
    ppu_wait_frame();
  }
}

int title_screenCG() 
{ 
  
  int num; //winning cup number
  //int num = ((seed+rand()) % 3) +1; //winning cup number
  int score;
  int total;
  int i;
  int cup=1;  //user selected cup number
  char oam_id;
  char scr[]="";
  char str[] ="";
  char tot[] ="";
  
  ppu_off();
  vram_adr(NTADR_A(0,1));	
  vram_write("                                ", 32);
  vram_adr(NTADR_A(0,5));	
  vram_write("                                ", 32);
     vram_adr(NTADR_A(7,6));	
  vram_write("                ",16);
     vram_adr(NTADR_A(7,7));	
  vram_write("                ",16);
   vram_adr(NTADR_A(7,8));	
  vram_write("                ",16);
     vram_adr(NTADR_A(7,9));	
  vram_write("                    ",20);
  vram_adr(NTADR_A(12,6));	
  vram_write("Cup Game", 8);
  
  vram_adr(NTADR_A(6,21));	
  vram_write("Press start to begin", 20);
  
  vram_adr(NTADR_A(0,54));	
  vram_write("                                ", 32);
  vram_adr(NTADR_A(0,59));	
  vram_write("                                ", 32);
  
  ppu_on_all();
   seed=rand() % 100;
   while(1)
   {
     if(pad_trigger(0)&PAD_START)
     {    
       start_ding();   
       test=2;
       while(1)
       {


  sprintf(str, "%d", num); 
   
  ppu_off();
  
  vram_adr(NTADR_A(8,2));		// set address
  vram_write("CUP GUESSING GAME!", 17);// write bytes to video RAM
 
  vram_adr(NTADR_A(2,6));
  vram_write("Please choose a cup using \x1c\x1d",28);
  
  vram_adr(NTADR_A(2,7));
  vram_write("start=play select=main menu",27);
  
  vram_adr(NTADR_A(6,10));
  vram_write("Cup 1",5);
  
  vram_adr(NTADR_A(6,12));
  vram_write("Cup 2",5);
 
  vram_adr(NTADR_A(6,14));
  vram_write("Cup 3",5);
  
  vram_adr(NTADR_A(6,22));	
  vram_write("                   ", 20);
  vram_adr(NTADR_A(0,21)); 	
  vram_write("                                ", 32);
        vram_adr(NTADR_A(13,58)); 
        vram_write("  ",2);

  ppu_on_all();
  
  //guess();

  
  display_cups();
  shuffle();
  display_cups();
  
  while(1)
  {
    num = ((seed+rand()) % 3) +9;
    if(pad_trigger(0)&PAD_DOWN && (actor_y[8] >=80 && actor_y[8] < 110))
    {
      oam_clear();
      
      actor_y[8]+=15;
      //if(cup<3)
      cup++;
      
      oam_id = oam_meta_spr(actor_x[8], actor_y[8], oam_id, metaspriteA);
      for(i=9;i<12;i++)
      {
        oam_id = oam_meta_spr(actor_x[i], actor_y[i], oam_id, metaspriteC);
      }
    }  
    
    if(pad_trigger(0)&PAD_UP && (actor_y[8] >80 && actor_y[8] <= 110))
    {
      oam_clear();
     
      actor_y[8]-=15;
        if(cup>1)
      {
        cup--;
      } 
      oam_id = oam_meta_spr(actor_x[8], actor_y[8], oam_id, metaspriteA);
      for(i=9;i<12;i++)
      {
       oam_id = oam_meta_spr(actor_x[i], actor_y[i], oam_id, metaspriteC);
      } 
    }
    
    if(pad_trigger(0)&PAD_START)
    {
      //shuffle();
      if((cup+8)==num)
      {
        ppu_off();
        win_chime();//sound
        score=score+1;
        total=total+1;
        
        vram_adr(NTADR_A(6,18)); 	
        vram_write("WIN!", 4);
       
        //vram_adr(NTADR_A(19,19)); 
        //vram_write(str,1);
        //vram_adr(NTADR_A(6,19)); 	
        //vram_write("Winning cup:", 12);
  
        sprintf(scr,"%d",score);
        sprintf(tot,"%d",total);
        vram_adr(NTADR_A(6,20)); 	
        vram_write("SCORE:     ATTEMPTS:", 22);
        
        vram_adr(NTADR_A(13,20)); 
        vram_write(scr,2);

        vram_adr(NTADR_A(26,20)); 
        vram_write(tot,2);
        
        ppu_on_all();  
        display_ball(num);
        cup=1;
        break;
      }
    else
    {
      ppu_off();
      total=total+1;
      lose_static(); //sound
      vram_adr(NTADR_A(6,18)); 	
      vram_write("LOSE", 4);

      //vram_adr(NTADR_A(19,19)); 
      //vram_write(str,1);
      //vram_adr(NTADR_A(6,19)); 	
      //vram_write("Winning cup:", 12);
      vram_adr(NTADR_A(6,20)); 	
      vram_write("SCORE:     ATTEMPTS:", 22);
      sprintf(scr,"%d",score);
      sprintf(tot,"%d",total);
      vram_adr(NTADR_A(13,20)); 
      vram_write(scr,2);

      vram_adr(NTADR_A(26,20)); 
      vram_write(tot,2);
      
      ppu_on_all();
      display_ball(num);
      cup=1;
      break;
     }
    }
      if(pad_trigger(0)&PAD_SELECT)
    {
        score=0;
        total=0;
        ppu_off();
      vram_adr(NTADR_A(8,2));		// set address
  	vram_write("                 ", 17);
          vram_adr(NTADR_A(2,6));
  vram_write("                                ",32);
  
  vram_adr(NTADR_A(2,7));
  vram_write("                                ",32);
  
  vram_adr(NTADR_A(6,10));
  vram_write("                                ",32);
  
  vram_adr(NTADR_A(6,12));
  vram_write("     ",5);
 
  vram_adr(NTADR_A(6,14));
  vram_write("     ",5);
        vram_adr(NTADR_A(6,18)); 	
        vram_write("    ", 4);
        vram_adr(NTADR_A(6,20)); 	
      vram_write("                      ", 22);
        ppu_on_all();
	return 1;
        //break;
        //continue;
       // title;
      }
  }
        
        //break;
  
      }
      
    }
  }
}


bool doesplayerhitCPU(userx, usery, cpux, cpuy)
{
  if(abs(userx - cpux) < 8)
  {
    if(abs(usery - cpuy) < 8)
    {
	scroll(0,0);
        //speed=-4;
        //lose_screen (); 
      return true;
    }
  }
}
bool doesplayerhitCPU2(userx, usery, cpux, cpuy){
  if(abs(userx - cpux) < 8)
  {
    if(abs(usery - cpuy) < 8)
    {
	scroll(0,0);
        return true;
    }
  }
}
int lose_screen()
{
  setup_graphics();
  lose_static();
  ppu_off();
  vram_adr(NTADR_A(0,8));
  vram_write("                               ",32);
  vram_adr(NTADR_A(0,11));
  vram_write("                               ",32);
  vram_adr(NTADR_A(0,14));
  vram_write("                               ",32);
  vram_adr(NTADR_A(0,17));
  vram_write("                               ",32);
  vram_adr(NTADR_A(0,20));
  vram_write("                               ",32);


  
  vram_adr(NTADR_A(12,10));		// set address
  vram_write("You Lose!", 8);// write bytes to video RAM
  
  vram_adr(NTADR_A(3,15));		// set address
  vram_write("Press start to play again", 25);
  vram_adr(NTADR_A(3,19));		// set address
  vram_write("Press select for main menu", 26);
  ppu_wait_frame();
  // enable PPU rendering (turn on screen)
  ppu_on_all();

  // infinite loop
  while (1)
  {
    if(pad_trigger(0)&PAD_START)
    {
      ppu_off();
     vram_adr(NTADR_A(12,10));		// set address
     vram_write("        ", 8);// write bytes to video RAM
  
     vram_adr(NTADR_A(3,15));		// set address
     vram_write("                         ", 25);
     
     vram_adr(NTADR_A(3,19));		// set address
     vram_write("                          ", 26); 
       vram_adr(NTADR_A(13,57));
  vram_write("  ",2);

     // enable PPU rendering (turn on screen)
     ppu_on_all();
    
     break;
    }
    if(pad_trigger(0)&PAD_SELECT)
    {
     ppu_off();
     vram_adr(NTADR_A(12,10));		// set address
     vram_write("        ", 8);// write bytes to video RAM
  
     vram_adr(NTADR_A(3,15));		// set address
     vram_write("                         ", 25);
     
     vram_adr(NTADR_A(3,19));		// set address
     vram_write("                          ", 26);
      vram_adr(NTADR_A(0,55));		// set address
     vram_write("                                ", 32);
       vram_adr(NTADR_A(13,57));
  vram_write("  ",2);
     // enable PPU rendering (turn on screen)
     ppu_on_all();
     //return;
      exitvar=1;
     //title_screen();
      return 1;
     break;
    }
  }
}
void new_some()
{
  byte runseq;
  char pad;
  char oam_id;
  char i;
  actor_x[0]=8;
  actor_y[0]=22;

  pad = pad_poll(i);
  
  if (pad&PAD_LEFT && actor_x[0]>8) actor_dx[0]=-2;
      else if (pad&PAD_RIGHT && actor_x[0]<240) actor_dx[0]=2;
      else actor_dx[0]=0;
      // move actor[i] up/down
      if (pad&PAD_UP && actor_y[0]>44) actor_dy[0]=-2;
      else if (pad&PAD_DOWN && actor_y[0]<166) actor_dy[0]=2;
      else actor_dy[0]=0;
      
     
        runseq = actor_x[0] & 7;
        if (actor_dx[0] >= 0)
        runseq += 8;      
      
        oam_id = oam_meta_spr(actor_x[0], actor_y[0], oam_id, playerRunSeq[runseq]);
        actor_x[0] += actor_dx[0];
        actor_y[0] += actor_dy[0];
  
}
void scroll_demo() {
  
  char i;	// actor index
  int score=1;
  char oam_id;	// sprite ID
  byte runseq;
  char scr[]="";
  char str[] ="";
  int speed=-3;
  int w;

  int seed;
  char pad;	// controller flags
  
  
  int x = 0;   // x scroll position
  int y = 0;   // y scroll position
  int dy = 1;  // y scroll direction
  int dx = 1;
  int random;
 
  int k=0;
  exitvar=0;
  
  ppu_off();
  vram_adr(NTADR_A(0,5));
  vram_write("\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d",32);
  vram_adr(NTADR_A(0,8));
  vram_write("\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d",32);
  vram_adr(NTADR_A(0,11));
  vram_write("\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d",32);
  vram_adr(NTADR_A(0,14));
  vram_write("\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d",32);
  vram_adr(NTADR_A(0,17));
  vram_write("\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d",32);
  vram_adr(NTADR_A(0,20));
  vram_write("\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d",32);
       
  vram_adr(NTADR_A(0,55));
  vram_write("\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d\x3d",32);
  
  vram_adr(NTADR_B(0,1));
  vram_write("\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae",32);
     vram_adr(NTADR_A(13,56));
  vram_write("       ",7);
  
  vram_adr(NTADR_B(0,59));
  vram_write("\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae\xae",32);
  //sprintf(scr,"%d",score);
  vram_adr(NTADR_B(13,56)); 
  vram_write("Score: ",7);

  setup_graphics();
  ppu_on_all();
  //title_screenRR();

 
	
  // initialize actors with random values
  for (i=0; i<8; i++) 
  {
    if(i==0)
    {
      actor_x[i]=8;
      actor_y[i]=22;
      
      continue;
    }
    actor_x[i] =i*32;
    
    actor_y[i]=((rand() % 6) *24)+46;
    //actor_y[i] =(rand() % (160 - 44 + 1)) + 44;
    
  }
  /*actor_y[2]=46;
  actor_y[3]=70;
  actor_y[4]=94;
  actor_y[5]=118;
  actor_y[6]=142;
  actor_y[7]=166;
 */
 

  // infinite loop
  while (1) {
    sprintf(scr,"%d",score);
    
   if(score>2 && score<4)
   {
     speed=-4;
   }
   if(score>4 && score<6)
   {
     speed=-5;
   }
    if(score>6)
    {
      speed=-6;
    }
    seed=rand() % 1000;
    
   if (seed <50)
    {
     mus1(); 
    }

        if ( 400 > seed > 350)
   {
         mus2(); 
    }

        if (seed >950)
    {
     mus3(); 
    }
    
    // wait for next frame
    //ppu_wait_frame();
    // update y variable
  
    if (doesplayerhitCPU2(actor_x[0], actor_y[0], actor_x[1], actor_y[1]))
    {
      score++;
     
      actor_x[1]=-5;
      actor_y[1]=((random % 6) *24)+46;
        
      for(w=0;w<2;w++)
      {
      ppu_wait_frame();
      }
      vram_adr(NTADR_B(13,57)); 
      vram_write(scr,2);

    }
    if (doesplayerhitCPU(actor_x[0], actor_y[0], actor_x[2], actor_y[2]))
    {
      lose_screen();
      return;
       actor_x[0]=8;
       actor_y[0]=22;
    }
     if (doesplayerhitCPU(actor_x[0], actor_y[0], actor_x[3], actor_y[3]))
    {
       lose_screen();
       return;
       speed=-3;
       actor_x[0]=8;
       actor_y[0]=22;
    }
     if (doesplayerhitCPU(actor_x[0], actor_y[0], actor_x[4], actor_y[4]))
    {
       lose_screen();
       return;
       speed=-3;
       actor_x[0]=8;
       actor_y[0]=22;
    }
     if (doesplayerhitCPU(actor_x[0], actor_y[0], actor_x[5], actor_y[5]))
    {
       lose_screen();
       return;
       speed=-3;
       actor_x[0]=8;
       actor_y[0]=22;
    }
     if (doesplayerhitCPU(actor_x[0], actor_y[0], actor_x[6], actor_y[6]))
    {
       lose_screen();
       return;
       speed=-3;
       actor_x[0]=8;
       actor_y[0]=22;
    }
     if (doesplayerhitCPU(actor_x[0], actor_y[0], actor_x[7], actor_y[7]))
    {
       lose_screen();
       return;
       speed=-3;
       actor_x[0]=8;
       actor_y[0]=22;
    }
    
    //y += dy;
    x += dx;
    // change direction when hitting either edge of scroll area
    if(y>= 479) dy = -1;//if (y >= 479) dy = -1;
    if (y == 0) dy = 1;
    
     // random=rand(); //new random value 
    // set scroll register

    // set player 0/1 velocity based on controller
    for (i=0; i<1; i++) 
    {
      for(k=1;k<8;k++)
      {
         random=rand();
	if(actor_x[k]<1)   
        {
          actor_y[k]=((random % 6) *24)+46;
         //actor_y[k]=(rand() % (160 - 44 + 1)) +44; //actor_y[k]=(rand() % (160 - 44 + 1)) +44+(i*random%10);
        }
      }
        
    
      // poll controller i (0-1)
      pad = pad_poll(i);
      
      // move actor[i] left/right
      if (pad&PAD_LEFT && actor_x[0]>8) actor_dx[0]=-2;
      else if (pad&PAD_RIGHT && actor_x[0]<240) actor_dx[0]=2;
      else actor_dx[0]=0;
      // move actor[i] up/down
      if (pad&PAD_UP && actor_y[0]>44) actor_dy[0]=-2;
      else if (pad&PAD_DOWN && actor_y[0]<166) actor_dy[0]=2;
      else actor_dy[0]=0;
      
     
        runseq = actor_x[0] & 7;
        if (actor_dx[0] >= 0)
        runseq += 8;      
      
        oam_id = oam_meta_spr(actor_x[0], actor_y[0], oam_id, playerRunSeq[runseq]);
        
        
       //obstacles
        oam_id = oam_meta_spr(actor_x[1]-8, actor_y[1], oam_id, metasprite2);
        oam_id = oam_meta_spr(actor_x[2]-8, actor_y[2], oam_id, metasprite);
        oam_id = oam_meta_spr(actor_x[3]-8, actor_y[3], oam_id, metasprite);
        oam_id = oam_meta_spr(actor_x[4]-8, actor_y[4], oam_id, metasprite);
        oam_id = oam_meta_spr(actor_x[5]-8, actor_y[5], oam_id, metasprite);
        oam_id = oam_meta_spr(actor_x[6]-8, actor_y[6], oam_id, metasprite);
        oam_id = oam_meta_spr(actor_x[7]-8, actor_y[7], oam_id, metasprite);
      
             
        actor_x[0] += actor_dx[0];
        actor_y[0] += actor_dy[0];
      
        //change speed of each obstacle
         actor_x[1] += speed;
         actor_x[2] += speed;
         actor_x[3] += speed;
         actor_x[4] += speed;
         actor_x[5] += speed;
         actor_x[6] += speed;
         actor_x[7] += speed;
       
    }
    
    // draw and move all actors
    // hide rest of sprites
    // if we haven't wrapped oam_id around to 0

    if (oam_id!=0) oam_hide_rest(oam_id);
    // wait for next frame
    ppu_wait_frame();
    
    scroll(x, y);  
    
  }
  
}
void title_screenRR() 
{
  
  ppu_off();
       vram_adr(NTADR_A(7,6));	
  vram_write("                ",16);
     vram_adr(NTADR_A(7,7));	
  vram_write("                ",16);
   vram_adr(NTADR_A(7,8));	
  vram_write("                ",16);
     vram_adr(NTADR_A(7,9));	
  vram_write("                    ",20);

  vram_adr(NTADR_A(10,6));	
  vram_write("Road runner", 11);
  
  vram_adr(NTADR_A(6,21));	
  vram_write("Press start to begin", 20);
  
  ppu_on_all();
   
 while(1)
  {
   seed=rand() % 100;
  
    if(pad_trigger(0)&PAD_START)
    {
      start_ding(); 
      ppu_off();
        vram_adr(NTADR_A(10,6));	
  	vram_write("           ", 11);
  
  	vram_adr(NTADR_A(6,21));	
  	vram_write("                       ", 20);
      ppu_on_all();
      while(1){
      scroll_demo();
        if(exitvar==1)
          break;
      }
      break;
    }
  }
}
int title_screen() 
{
  char pad;
  char oam_id;
  char i;
  actor_x[0]=110;
  actor_y[0]=133;
  actor_x[12]=160;
  actor_y[12]=133;
  actor_x[13]=60;
  actor_y[13]=133;

  
  ppu_off();
    vram_adr(NTADR_A(0,1));	
  vram_write("                                ", 32);
  vram_adr(NTADR_A(0,5));	
  vram_write("                                ", 32);
  
  //vram_adr(NTADR_A(10,6));	
 // vram_write("Pick a game", 11);
  
  vram_adr(NTADR_A(2,15));	
  vram_write("     R.R.       CupGame",23);
  
   vram_adr(NTADR_A(2,20));	
  vram_write("      A           B",20);
   vram_adr(NTADR_A(7,6));	
  vram_write("\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f",16);
     vram_adr(NTADR_A(7,7));	
  vram_write("\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f",16);
   vram_adr(NTADR_A(7,8));	
  vram_write("\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf",16);
     vram_adr(NTADR_A(7,9));	
  vram_write("    \xb0\xb1\xb2\xb3\xb4\xb5\xbb\xbc\xb6\xb7\xb8\xb9\xba\xbd\xbe\xbf",20);
    vram_adr(NTADR_A(0,54));	
  vram_write("                                ",32);
  vram_adr(NTADR_A(0,59));	
  vram_write("                                ",32);
   vram_adr(NTADR_A(13,56));
  vram_write("       ",7);
  vram_adr(NTADR_A(0,57));
  vram_write("                                ",32);
  vram_adr(NTADR_A(13,58)); 
        vram_write("  ",2);
  setup_graphics();
  ppu_on_all();
  //ppu_off();  
  
 while(1)
  { 
   
   oam_id = 0;
    for (i=0; i<2; i++) {
      ppu_wait_frame();
      // poll controller i (0-1)
      pad = pad_poll(i);
      // move actor[i] left/right
      if (pad&PAD_LEFT && actor_x[i]>0) actor_dx[i]=-2;
      else if (pad&PAD_RIGHT && actor_x[i]<240) actor_dx[i]=2;
      else actor_dx[i]=0;
      // move actor[i] up/down
      if (pad&PAD_UP && actor_y[i]>0) actor_dy[i]=-2;
      else if (pad&PAD_DOWN && actor_y[i]<212) actor_dy[i]=2;
      else actor_dy[i]=0;
      
      if(abs(actor_x[0] - actor_x[13]) < 8)
  {
    
    if(abs(actor_y[0] - actor_y[13]) < 8)
    {
	oam_clear();
      start_ding(); 
      ppu_off();
        vram_adr(NTADR_A(10,6));	
  	vram_write("           ", 11);
  
  	vram_adr(NTADR_A(2,11));	
  	vram_write("                                 ", 27);
        	vram_adr(NTADR_A(2,15));	
  	vram_write("                                 ", 27);
        	vram_adr(NTADR_A(2,20));	
  	vram_write("                                 ", 27);
      title_screenRR();
      //ppu_on_all();
      return 1;;
    }
  }
 
  else if(abs(actor_x[0] - actor_x[12]) < 8)
  {
    if(abs(actor_y[0] - actor_y[12]) < 8)
    {
	oam_clear();
      start_ding(); 
      ppu_off();
        vram_adr(NTADR_A(10,6));	
  	vram_write("           ", 11);
  
  	vram_adr(NTADR_A(2,11));	
  	vram_write("                                 ", 27);
      
              	vram_adr(NTADR_A(2,15));	
  	vram_write("                                 ", 27);
        	vram_adr(NTADR_A(2,20));	
  	vram_write("                                 ", 27);
      
      
      //ppu_on_all();
      title_screenCG();
      return 1;
    }
  }
    }
      for (i=0; i<1; i++) 
      {
      byte runseq = actor_x[i] & 7;
        
      if (actor_dx[i] >= 0)
        runseq += 8;
      oam_id = oam_meta_spr(actor_x[i], actor_y[i], oam_id, playerRunSeq[runseq]);
      actor_x[i] += actor_dx[i];
      actor_y[i] += actor_dy[i];
    }
   
   oam_id = oam_meta_spr(actor_x[12], actor_y[12], oam_id, metaspriteD);
   oam_id = oam_meta_spr(actor_x[13], actor_y[13], oam_id, metaspriteD);


   if(pad_poll(0)&PAD_B||PAD_A)
    {
     if(pad_poll(0)&PAD_B)
     {
     oam_clear();
      start_ding(); 
      ppu_off();
        vram_adr(NTADR_A(10,6));	
  	vram_write("           ", 11);
  
  	vram_adr(NTADR_A(2,11));	
  	vram_write("                                 ", 27);
     
             	vram_adr(NTADR_A(2,15));	
  	vram_write("                                 ", 27);
        	vram_adr(NTADR_A(2,20));	
  	vram_write("                                 ", 27);
      //ppu_on_all();
      title_screenCG();
      break;
     //continue;
       
     }
     
     if(pad_poll(0)&PAD_A)
    {
      oam_clear();
      start_ding(); 
      ppu_off();
        vram_adr(NTADR_A(10,6));	
  	vram_write("           ", 11);
  
  	vram_adr(NTADR_A(2,11));	
  	vram_write("                                 ", 27);
      
              	vram_adr(NTADR_A(2,15));	
  	vram_write("                                 ", 27);
        	vram_adr(NTADR_A(2,20));	
  	vram_write("                                 ", 27);
      title_screenRR();
      //ppu_on_all();
      break;
    }
     
   }
  }
}

// main program
void main() 
{
  //setup_graphics();
  while(1)
  { // scroll_demo();
    title_screen();
  }
}