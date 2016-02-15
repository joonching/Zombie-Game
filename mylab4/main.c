////////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        Lab 4
// File name:       main.c
//Modified by Joshua Chung
// Written by:      CDP (and modified by ALA)
// Last modified:   9/1/2014
#define _PLIB_DISABLE_LEGACY
#include <plib.h>
#include "PmodOLED.h"
#include "OledChar.h"
#include "OledGrph.h"
#include "delay.h"
#include "ADXL345.h"
#include "Communication.h"
#include <stdbool.h>


// Digilent board configuration
#pragma config FPLLMUL  = MUL_20        // PLL Multiplier
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider
#pragma config FPLLODIV = DIV_1         // PLL Output Divider
#pragma config FPBDIV   = DIV_8         // Peripheral Clock divisor
#pragma config FWDTEN   = OFF           // Watchdog Timer
#pragma config WDTPS    = PS1           // Watchdog Timer Postscale
#pragma config FCKSM    = CSDCMD        // Clock Switching & Fail Safe Clock Monitor
#pragma config OSCIOFNC = OFF           // CLKO Enable
#pragma config POSCMOD  = HS            // Primary Oscillator
#pragma config IESO     = OFF           // Internal/External Switch-over
#pragma config FSOSCEN  = OFF           // Secondary Oscillator Enable
#pragma config FNOSC    = PRIPLL        // Oscillator Selection
#pragma config CP       = OFF           // Code Protect
#pragma config BWP      = OFF           // Boot Flash Write Protect
#pragma config PWP      = OFF           // Program Flash Write Protect
#pragma config ICESEL   = ICS_PGx1      // ICE/ICD Comm Channel Select
#pragma config DEBUG    = OFF           // Debugger Disabled for Starter Kit

// Intrumentation for the logic analyzer (or oscilliscope)
#define MASK_DBG1  0x1;
#define MASK_DBG2  0x2;
#define MASK_DBG3  0x4;
#define DBG_ON(a)  LATESET = a
#define DBG_OFF(a) LATECLR = a
#define DBG_INIT() TRISECLR = 0x7

// Definitions for the ADC averaging. How many samples (should be a power
// of 2, and the log2 of this number to be able to shift right instead of
// divide to get the average.
#define NUM_ADC_SAMPLES 32
#define LOG2_NUM_ADC_SAMPLES 5

// Global variables
unsigned int sec1000 = 0; // This is updated 1000 times per second by the interrupt handler
unsigned int sec100 = 0;
unsigned int sec10 = 0;
signed short x = 0;
signed short y = 0;
signed short z = 0;

unsigned char Button1History = 0x00;    // Last eight samples of BTN1
unsigned char Button2History = 0x00;    // Last eight samples of BTN1
unsigned char Button3History = 0x00;

int xyconstant = 90;
int xy2constant = 50;

int score = 0;
int speed_counter = 0;
int p_time = 0;
bool delay_me;

void __ISR(_TIMER_2_VECTOR, IPL3AUTO) _Timer2Handler(void)
{
   if( INTGetFlag(INT_T2) )             // Verify source of interrupt
   {

       delay_me = false;
       if(sec10 > 10)
       {
            ADXL345_GetXyz(&x,&y,&z);
            sec10 = 0;
       }
       if(sec100 >= 980)
       {
           score++;
           sec100 = 0;
       }

       Button1History <<= 1;           // Update button history

        if(PORTG & 0x40)                // Read current position of BTN1
            Button1History |= 0x01;

        Button2History <<= 1;           // Update button history

        if(PORTG & 0x80)                // Read current position of BTN2
            Button2History |= 0x01;

        Button3History <<=1;
        if(PORTA & 0x1) //btn3
            Button3History |= 0x01;

      sec100++;// Update global variable
      sec10++;
      sec1000++;
      speed_counter++;
      INTClearFlag(INT_T2);             // Acknowledge interrupt
   }
   p_time++;
   return;
}

typedef struct trace_me
{
    int x;
    int y;
    int dir;
}trace;

//************************my functions******************************
//******************************************************************
void get_human_position(int *hx, int *hy, int *dir, int *speed);
int check_digital(int d1, int d2);
void get_human_position2(int *hx, int *hy, int *dir, int choosex, int choosey, int *speed);
void get_zombie_position(trace *z, int count, int *hx, int *hy);
int check_points(trace *z, int count, int hx, int hy);
void intro_ani(char rh, char h, char b);
void invert_ani(char lh, char hl, char b);
void print_all(char h, char z, char b, int hx, int hy, int zx, int zy);
void do_physicsx(int typex, int *speed, int *hx, int *hy);
int btn_history();
void draw_balloons(int d, int t, char tb, char bb, char s, char s_1);
void draw_balloons2(int d, int t, char tb, char bb, char s, char s_1, char b);
void print_zombies(trace *z, trace *pz, int z_count);
bool xyrand(trace *h, trace *z);
bool check_heart(trace *h, int hx, int hy, int i);
char get_head();
char get_righth();
char get_blank();
char get_heart();
char get_headr();
char get_lefth();
//******************************************************************
void initTimer2() {
    // Configure Timer 2 to request a real-time interrupt once per millisecond.
    // The period of Timer 2 is (16 * 625)/(10 MHz) = 1 s.
    OpenTimer2(T2_ON | T2_IDLE_CON | T2_SOURCE_INT | T2_PS_1_16 | T2_GATE_OFF, 624);
    INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_3);
    INTClearFlag(INT_T2);
    INTEnable(INT_T2, INT_ENABLED);
}


int main()
{
     ////////////////////VARIABLES && BTNCONFIG/////////////////////
   //////////////////////////////////////////////////////////////
   char buf[17];        // Temp string for OLED display
   trace human[36];
   trace zombie[36];
   trace prev_zombie[36];
   trace heart_pos[36];
   int speed = 600;
   int move_z = 0;
   int led = 12;
   int cursor_pos = 0;
   int print_score = 0;
   int btn_hist = 0;
   int diff = 0;
   int heart_count = 0;
   int count = 5;
   int dis_heart = 0;
   int telexy = 113;
   int tele_count = 3;
   int power = 5;
   bool tap_go = false;
   bool cur_heart = false;
   bool ate_heart = false;
   bool power_time = false;
   bool reset_me = false;
   int print_mu = 0;
   int print_go = 0;
   int print_pa = 0;
   int c_mu = 0;
   int c_go = 0;
   int c_pa = 0;
   int thousand = 0;

   // Initialize GPIO for BTN1 and LED1
   DDPCONbits.JTAGEN = 0; // JTAG controller disable to use BTN3
   TRISGSET = 0x40;     // For BTN1: configure PortG bit for input
   TRISGSET = 0x80;     // For BTN2: configure PORTG bit for input
   TRISASET = 0x1;  ///btn3

   TRISGCLR = 0xf000;   // For LED1: configure PortG pin for output
   ODCGCLR  = 0xf000;   // For LED1: configure as normal output (not open drain)
   LATGCLR = 0xf000;    //Initialize LEDs to 0000

   //////////////////////////////////////////////////////////////
   //////////////////////////////////////////////////////////////

   /////////////////ADXL initialization && STATES//////////////
   ////////////////////////////////////////////////////////////
   DelayInit();
   OledInit();
   initTimer2();
   enum States{init, difficulty, play, print_play, splash, chase, reset,game_over, btn_down,
                choose_player, load, draw_heart, draw_power_state, power_state, balloon};
   enum States state = init;
   int chnl = ADXL345_Init();

   if (chnl == -1)
   {
       state = game_over;
   }
   SpiMasterInit(chnl);

   ////////////////////////////////////////////////////////////
   ////////////////////////////////////////////////////////////

   //********************TIMER2 && INT CONFIGs***********
   //****************************************************

   INTSetVectorPriority(INT_EXTERNAL_1_VECTOR, INT_PRIORITY_LEVEL_2);
   INTClearFlag(INT_INT1);
   INTEnable(INT_INT1, INT_ENABLED);
   INTSetVectorPriority(INT_EXTERNAL_2_VECTOR, INT_PRIORITY_LEVEL_2);
   INTClearFlag(INT_INT2);
   INTEnable(INT_INT2, INT_ENABLED);

   INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
   INTEnableInterrupts();
   //****************************************************
   //****************************************************
   while (1)
   {
       if(ADXL345_SingleTapDetected() && tap_go)
           state = reset;
       switch(state)
       {
           case init:
               OledClearBuffer();
               human[0].x = rand()%11;
               human[0].y = rand()%3;
               zombie[0].x = rand()%11;
               zombie[0].y = rand()%3;
               zombie[1].x = rand()%11;
               zombie[1].y = rand()%3;
               zombie[2].x = rand()%11;
               zombie[2].y = rand()%3;

               if((human[0].x == zombie[0].x && human[0].y == zombie[0].y) ||
                       (human[0].x == zombie[1].x && human[0].y == zombie[1].y) ||
                       (human[0].x == zombie[2].x && human[0].y == zombie[2].y))
                   state = init;
               else if(reset_me)
                   state = difficulty;
                else
                   state = splash;
                    break;

           case splash:
               reset_me = false;
               OledSetCursor(0,0);
               OledPutString("WELCOME TO ");
               OledSetCursor(0,1);
               OledPutString("ZOMBIELAND");
               OledSetCursor(0,2);
               OledPutString("Joshua Chung");
               intro_ani(get_righth(), get_headr(), get_blank());
               OledUpdate();

               if(sec1000 > 2000)
               {
                   OledClearBuffer();
                   state = difficulty;
                   sec1000 = 0;
               }

               break;

           case difficulty:
               OledSetCursor(0,0);
               OledPutString("Difficulty");
               OledSetCursor(0,1);
               OledPutString("BTN1: DIFF++");

               OledSetCursor(0,2);
               OledPutString("BTN2: DIFF--");

               OledSetCursor(0,3);
               OledPutString("BTN3: PLAY");
               if(sec1000 >= 200)
                    btn_hist = btn_history();
               diff = 1;
               if(btn_hist != 0)
                   state = btn_down;
               break;
           case choose_player:
               if(sec1000 >= 200)
               {
                   OledSetCursor(0,0);
                   OledPutString("Choose Player");
                   OledSetCursor(0,1);
                   OledPutString("MU");
                   OledSetCursor(5,1);
                   OledPutString("GO");
                   OledSetCursor(10,1);
                   OledPutString("PA");
                   OledSetCursor(cursor_pos, 2);
                   OledPutString("  ");
                   if(y < -xyconstant && cursor_pos != 10)
                       cursor_pos = cursor_pos+5;


                   else if(y > xyconstant && cursor_pos != 0)
                       cursor_pos = cursor_pos-5;

                   OledSetCursor(cursor_pos, 2);
                   OledPutString("--");
                   OledSetCursor(0,4);
                   OledPutString("BTN3 to play");
                   OledUpdate();
                   diff = 0;
                   sec1000 = 0;
                   btn_hist = btn_history();
                   if(btn_hist == 3)
                   {
                       state = btn_down;
                       score = 0;
                   }
               }
               break;
           case btn_down:

               //used to delay the movement of the zombies
               if(led == 15)
               {
                   count = 7;
                   power = 3;
               }
               if(led == 16)
               {
                   count = 9;
                   power = 2;
               }

               if(btn_hist == 3 && Button3History == 0x00)
               {
                   OledClearBuffer();
                   if(diff == 1)
                        state = choose_player;
                   else
                   {
                       sec1000 = 0;
                       state = load;
                   }
                   score = 0;
                   break;
               }
               if(btn_hist == 2 && Button2History == 0x00)
               {
                   if(led != 12)
                   {
                       LATGCLR = 1 << led;
                       led--;
                   }
                   state = difficulty;
               }

               else if(btn_hist == 1 && Button1History == 0x00)
               {
                   if(led != 16)
                   {
                       LATGSET = 1 << led;
                       led++;
                   }
                   state = difficulty;
               }

               break;

           case load:

               OledSetCursor(0, 2);
               OledPutString("  Loading ...");

               int i;
               for(i = 0; i<1; i++)
               {
                   intro_ani(get_righth(), get_headr(), get_blank());
                   invert_ani(get_lefth(), get_head(), get_blank());
                   OledUpdate();
               }
               intro_ani(get_righth(), get_headr(), get_blank());
               invert_ani(get_lefth(), get_head(), get_blank());
               OledUpdate();

               if(sec1000 > 1000)
               {
                   OledClearBuffer();
                   state = print_play;
               }

               break;
           case print_play:
               tap_go = true;
           {
               if(ADXL345_SingleTapDetected())
                   {
                       state = reset;
                       break;
                   }
               tap_go = true;
               print_score = score;
                if(speed_counter >= speed) //my delay
                {
                    if(score > 1000)
                    {
                        score = 2;
                        thousand++;
                    }
                   OledSetCursor(12,0);
                   sprintf(buf, "%3d", score);
                   OledPutString(buf);
                   OledSetCursor(12,1);
                   sprintf(buf, "H:%d", heart_count);
                   OledPutString(buf);
                   OledSetCursor(12,2);
                   sprintf(buf, "T:%d", tele_count);
                   OledPutString(buf);
                }
           }
           if(!power_time)
               state = play;
           else
           {
               p_time = 0;
               state = draw_power_state;
           }
           break;
           case play:
           {
               tap_go = true;
               dis_heart++;
               if(speed_counter >= speed) //my delay
               {
                   if(cur_heart && check_heart(heart_pos, human[0].x, human[0].y,0))
                   {
                       heart_count++;
                       score = score*1.5;
                       ate_heart = true;
                   }

                   if(heart_count == power)
                   {
                       OledClearBuffer();
                       power_time = true;
                       heart_count = 0;
                       state = print_play;
                       break;
                   }

                   //my delay method for zombie
                   move_z++;
                   if(ADXL345_SingleTapDetected())
                   {
                       state = reset;
                       break;
                   }

                   if(check_points(zombie, led, human[0].x, human[0].y) == 1)
                   {
                       OledClearBuffer();
                       state = game_over;
                       print_score = score;
                       sec1000 = 0;
                       break;
                   }
                   int i;


                   int prev_hx = human[0].x;
                   int prev_hy = human[0].y;
                   get_human_position(&human[0].x, &human[0].y, &human[0].dir, &speed);
                   if(move_z > count)
                   {
                       for(i = 0; i < 3; i++)
                            prev_zombie[i] = zombie[i];
                       get_zombie_position(zombie, led, &human[0].x, &human[0].y);
                       move_z = 0;
                   }

                   if(btn_history() == 3 && tele_count != 0)
                   {
                       human[0].x = telexy/10;
                       human[0].y = telexy%10;
                       tele_count--;
                   }

                   OledSetCursor(prev_hx, prev_hy);
                   OledDrawGlyph(get_blank());

                   OledSetCursor(human[0].x, human[0].y);
                   if(human[0].dir == 0)
                       OledDrawGlyph(get_head());
                   if(human[0].dir == 1)
                       OledDrawGlyph(get_headr());

                   print_zombies(zombie, prev_zombie, led);
                   OledUpdate();
                   speed_counter = 0;
               }
               print_score = score;
               state = draw_heart;

               break;
           }

           case draw_heart:
           {
               if(ADXL345_SingleTapDetected())
                   {
                       state = reset;
                       break;
                   }
               if(speed_counter >= speed)
               {
                   if(ADXL345_SingleTapDetected())
                   {
                       state = reset;
                       break;
                   }
                   int choose = rand()%10;
                   if((cur_heart && dis_heart > 150000) || (cur_heart && ate_heart))
                   {
                       dis_heart = 0;
                       cur_heart = false;
                       ate_heart = false;
                       OledSetCursor(heart_pos[0].x, heart_pos[0].y);
                       OledDrawGlyph(get_blank());
                   }

                   else if((choose%3 == 0 && !cur_heart) || (!cur_heart && ate_heart))
                   {
                       dis_heart = 0;
                       heart_pos[0].x = rand()%12;
                       heart_pos[0].y = rand()%3;
                       OledSetCursor(heart_pos[0].x, heart_pos[0].y);
                       OledDrawGlyph(get_heart());
                       cur_heart = true;
                       ate_heart = false;
                   }
                }

               state = print_play;
             }
           break;

           case draw_power_state:
           {
               if(ADXL345_SingleTapDetected())
                   {
                       state = reset;
                       break;
                   }

               OledSetCursor(12,3);
               OledPutString(" ");
                int i;
                for(i = 1; i < 30; i++)
                {
                    heart_pos[i].x = rand()%11;
                    heart_pos[i].y = rand()%3;
                    OledSetCursor(heart_pos[i].x, heart_pos[i].y);
                    OledDrawGlyph(get_heart());
                }
                p_time = 0;
                state = power_state;
           }

               break;

           case power_state:
           {
               if(ADXL345_SingleTapDetected())
                   {
                       state = reset;
                       break;
                   }

               if(p_time < 3000)
               {
                   OledSetCursor(12,3);
                   OledPutString(" ");
                   int i;
                   int prev_hx = human[0].x;
                   int prev_hy = human[0].y;
                   get_human_position(&human[0].x, &human[0].y, &human[0].dir, &speed);
                   for(i = 1; i < 30; i++)
                   {
                       if(check_heart(heart_pos, human[0].x, human[0].y, i))
                       {
                           OledSetCursor(heart_pos[i].x, heart_pos[i].y);
                           OledDrawGlyph(get_blank());
                           score = score*1.5;
                           heart_pos[i].x = 50;
                           heart_pos[i].y = 50;
                       }
                   }

                   OledSetCursor(prev_hx, prev_hy);
                   OledDrawGlyph(get_blank());

                   OledSetCursor(human[0].x, human[0].y);
                   if(human[0].dir == 0)
                       OledDrawGlyph(get_head());
               }
               else
               {
                   OledClearBuffer();
                   state = print_play;
                   power_time = false;
               }
           }
           break;

           case reset:
               LATGCLR = 0xf000;    //Initialize LEDs to 0000
               move_z = 0;
               led = 12;
               speed = 300;
               score = 0;
               cursor_pos = 0;
               print_score = 0;
               btn_hist = 0;
               diff = 0;
               heart_count = 0;
               count = 5;
               dis_heart = 0;
               telexy = 113;
               tele_count = 3;
               power = 5;
               tap_go = false;
               cur_heart = false;
               ate_heart = false;
               thousand = 0;
               reset_me = true;
               sec1000 = 0;
                              state = init;
                           break;

           case game_over:
           {
               if(ADXL345_SingleTapDetected())
                   {
                       state = reset;
                       break;
                   }
               int mu_thou = 0;
               int go_thou = 0;
               int pa_thou = 0;
               if(cursor_pos == 0)
               {
                   print_mu = print_score;
                   mu_thou = thousand;
                   if(print_mu + 1000*mu_thou > c_mu)
                       c_mu = print_mu + 1000*mu_thou;
               }
               if(cursor_pos == 5)
               {
                   print_go = print_score;
                   go_thou = thousand;

                   if(print_go + 1000*go_thou > c_go)
                       c_go = print_go + 1000*go_thou;
               }
               if(cursor_pos == 10)
               {
                   print_pa = print_score;
                   pa_thou = thousand;
                   if(print_pa + 1000*pa_thou > c_pa)
                       c_pa = print_pa + 1000*pa_thou;
               }
               
               OledSetCursor(0,0);
               OledPutString("Score Table:\n");
               OledSetCursor(0,1);
               sprintf(buf, "MU: %d", c_mu);
               OledPutString(buf);
               OledSetCursor(0,2);
               sprintf(buf, "GO: %d", c_go);
               OledPutString(buf);
               OledSetCursor(0,3);
               sprintf(buf, "PA: %d", c_pa);
               OledPutString(buf);

               if(sec1000 >= 3000)
               {
                   state = balloon;
                   sec1000 = 0;
               }


               OledUpdate();
               if(btn_history() == 3)
                   state = reset;

           }
               break;

           case balloon:
           {
                BYTE top_balloon[8] = {0x00, 0x80, 0x40,0x40,0x40,0x40, 0x80, 0x00};
                char t_b = 0x06;

                BYTE bot_balloon[8] = {0x00, 0x07,0x08,0xB8,0x48,0x04,0x03,0x00};
                char b_b = 0x07;

                BYTE balloon_str[8] = {0x00,0x00,0x00, 0xE3, 0x14, 0x08, 0x00, 0x00};
                char s1 = 0x08;
                BYTE balloon_str2[8] = {0x00, 0x08, 0x54, 0x23, 0x00,0x00,0x00,0x00};
                char s2 = 0x09;
                OledDefUserChar(t_b, top_balloon);
                OledDefUserChar(b_b, bot_balloon);
                OledDefUserChar(s1, balloon_str);
                OledDefUserChar(s2, balloon_str2);
               OledClearBuffer();
               int t;
                    for (t = 3; t>=0; t--)
                    {
                        if(t != 3)
                        {
                            draw_balloons(2, t+1, t_b, b_b, s1, s2);
                            draw_balloons(7, t+1, t_b, b_b, s1, s2);
                            draw_balloons(12, t+1, t_b, b_b, s1, s2);
                            draw_balloons(15, t+1, t_b, b_b, s1, s2);
                        }
                        draw_balloons(0, t, t_b, b_b, s1, s2);
                        draw_balloons(5, t, t_b, b_b, s1, s2);
                        draw_balloons(10, t, t_b, b_b, s1, s2);
                        draw_balloons(13, t, t_b, b_b, s1, s2);
                        sprintf(buf, "               ");
                        OledSetCursor(0, t);
                        OledPutString(buf);

                    }

                    for (t = 3; t>=0; t--)
                    {
                        if(t != 3)
                        {
                            draw_balloons2(2, t+1, t_b, b_b, s1, s2,get_blank());
                            draw_balloons2(7, t+1, t_b, b_b, s1, s2,get_blank());
                            draw_balloons2(12, t+1, t_b, b_b, s1, s2,get_blank());
                            draw_balloons2(15, t+1, t_b, b_b, s1, s2,get_blank());
                        }

                        draw_balloons2(0, t, t_b, b_b, s1, s2,get_blank());
                        draw_balloons2(5, t, t_b, b_b, s1, s2,get_blank());
                        draw_balloons2(10, t, t_b, b_b, s1, s2, get_blank());
                        draw_balloons2(13, t, t_b, b_b, s1, s2, get_blank());
                    }

           }
           OledClearBuffer();
               state = game_over;
               break;

           default:
               break;
       }
   }

      return 0;
}

//**********************Characters************************************
//********************************************************************
char get_head()//runnning left
{
        BYTE s_head[8] = {0x80, 0x40, 0x37, 0x48, 0x97, 0x15, 0x17, 0x00};
        char  human_head = 0x00;
        OledDefUserChar(human_head, s_head);

        return human_head;
}

char get_righth()
{
   BYTE right_head[8] = {0x3C, 0x42, 0x81, 0xB9, 0xAA, 0x6C, 0x28, 0x00};
   char zombie_h = 0x01;
   OledDefUserChar(zombie_h, right_head);

   return zombie_h;
}

char get_blank()
{
     BYTE blank[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
     char blank_char = 0x02;
     OledDefUserChar(blank_char, blank);

     return blank_char;
}

char get_heart()
{
    BYTE heart[8] = {0x04, 0x0E, 0x1F, 0x3D, 0x7C, 0x3D, 0x1F,0x0E};
    char my_heart = 0x03;
    OledDefUserChar(my_heart, heart);

    return my_heart;
}


char get_lefth()
{
    BYTE left_head[8] = {0x00, 0x28, 0x6C, 0xAA, 0xB9, 0x81, 0x42, 0x3C};
    char left_h = 0x04;
    OledDefUserChar(left_h, left_head);

    return left_h;
}

char get_headr()
{
    BYTE run_right[8] = {0x00,0x17, 0x15, 0x97, 0x48, 0x37, 0x40, 0x80};
    char rr = 0x05;
    OledDefUserChar(rr, run_right);

    return rr;

}

//********************************************************************
//********************************************************************

const int my_speed1 = 150;
const int my_speed2 = 450;
const int temp = 60;
void get_speed(int *speed)
{
    if(x > xyconstant +temp)
        (*speed) = my_speed1;
    else if(x < -xyconstant - temp)
        (*speed) = my_speed1;
    else
        (*speed) = my_speed2;

    if(y < -xyconstant - temp)
        (*speed) = my_speed1;
    else if(y > xyconstant + temp)
        (*speed) = my_speed1;
    else
        (*speed) = my_speed2;
}

void do_physicsx(int typex, int *speed, int *hx, int *hy)
{
    static int count = 0;
    if(*speed == 300)
        count = 3;
    if(*speed == 150)
        count = 2;

    int i;
    for(i = 0; i < count; i++)
    {
        DelayMs(600);
        OledSetCursor(*hx, *hy);
        OledDrawGlyph(get_blank());

        if(typex == 1)
            OledSetCursor((*hx)--, (*hy));
        if(typex == 0)
            OledSetCursor((*hx)++, (*hy));
        OledDrawGlyph(get_head());
    }
    count--;
}
void get_human_position2(int *hx, int *hy, int *dir, int choosex, int choosey, int *speed)
{
    if(choosex == 1) //if 1 it means that im going right
    {
        if((*hx) < 11)
        {
            (*hx)++;
            (*dir) = 1;
        }
    }
    else if(choosex == 0) //if 0 it means im going left
    {
        if((*hx) > 0)
        {
            (*hx)--;
            (*dir) = 0;
        }
    }
    if(choosey == 1) //if 1 it means i'm going down
    {
        if((*hy) < 3 )
            (*hy)++;
    }
    else if(choosey == 0) //if  0 it means im going up
    {
          if((*hy) > 0)
            (*hy)--;
    }


}
//choosex == 1 means that im going to move right
//choosey == 0 means im going down
void get_human_position(int *hx, int *hy, int *dir, int *speed)
{
    bool ran = false;
    get_speed(&(*speed));
    if(x > xyconstant && y > xyconstant)
    {
        get_human_position2(&(*hx), &(*hy), &(*dir), 0, 0, &(*speed));
        ran = true;
    }

    else if(x > xyconstant && y < -xyconstant)
    {
        get_human_position2(&(*hx), &(*hy), &(*dir),1 , 0 ,&(*speed));
        ran = true;
    }

    else if(x < -xyconstant && y > xyconstant)
    {
        get_human_position2(&(*hx), &(*hy), &(*dir), 0, 1 ,&(*speed));
        ran = true;
    }

    else if(x < -xyconstant && y < -xyconstant)
    {
        get_human_position2(&(*hx), &(*hy), &(*dir), 1, 1,&(*speed));
        ran = true;
    }
    //choosex == 1 means that im going to move right
//choosey == 1 means im going down
    if(!ran)
    {
        if(x > xyconstant) //human moves up
        {
            get_human_position2(&(*hx), &(*hy), &(*dir),3, 0,&(*speed));
        }
        if(x < -xyconstant)//human moves down
        {
            get_human_position2(&(*hx), &(*hy), &(*dir),3, 1,&(*speed));
        }

        if(y < -xyconstant)//human goes right
        {
            get_human_position2(&(*hx), &(*hy),&(*dir),1, 3,&(*speed));
        }

        if(y > xyconstant) //human goes left
        {
            get_human_position2(&(*hx), &(*hy), &(*dir),0, 3,&(*speed));
        }
    }
}

void get_zombie_position2(trace *z, int m_count)
{
    if(m_count > 1)
    {
        if(z[0].x == z[1].x && z[0].y == z[1].y)
        {
            if(z[0].x < 11)
                z[0].x++;
            else if(z[0].x > 0)
                z[0].x++;
            if(z[0].y > 0)
                z[0].y--;
            else if(z[0].y < 3)
                z[0].y++;
        }
        if (m_count > 2)
        {
            if(z[0].x == z[2].x && z[0].y == z[2].y)
            {
                if(z[0].x < 11)
                    z[0].x++;
                else if(z[0].x > 0)
                    z[0].x++;
                if(z[0].y > 0)
                    z[0].y--;
                else if(z[0].y < 3)
                    z[0].y++;
            }
            if(z[1].x == z[2].x && z[1].y == z[2].y)
            {
                if(z[1].x < 11)
                    z[1].x++;
                else if(z[1].x > 0)
                    z[1].x++;
                if(z[1].y > 0)
                    z[1].y--;
                else if(z[1].y < 3)
                    z[1].y++;
            }
        }
    }
}

void get_zombie_position(trace *z, int count, int *hx, int *hy)
{
    int max = 1;
    if (count == 15)
        max = 2;
    if (count == 16)
        max = 3;

    int i;
    for(i = 0; i < max; i++)
    {
        if(z[i].x >= (*hx) && z[i].x > 0)
        {
            z[i].x--;
            z[i].dir = 1; //1 is left
        }

        else if(z[i].x <= (*hx) && z[i].x < 11)
        {
            z[i].x++;
            z[i].dir = 0;//0 is right
        }

        if(z[i].y >= (*hy) && z[i].y > 0)
            z[i].y--;

        else if(z[i].y <= (*hy) && z[i].y < 3)
            z[i].y++;
    }
    get_zombie_position2(z,max);
}


int check_points(trace *z, int count, int hx, int hy)
{
    int max = 1;
    if(count == 15)
        max = 2;
    if(count == 16)
        max = 3;
    int i;
    for(i = 0; i < max; i++)
    {
        if(z[i].x == hx && z[i].y == hy)
            return 1;
    }
        return 0;
}
void invert_ani(char lh, char hl, char b)
{
    int i;
    for(i = 16; i > 0; i--)
    {
        DelayMs(70);
        if(i)
        {
            OledSetCursor(i+1, 3);
            OledDrawGlyph(b);
            OledSetCursor(i+3, 3);
            OledDrawGlyph(b);

        }

        OledSetCursor(i, 3);
        OledDrawGlyph(hl);

        OledSetCursor(i+2, 3);
        OledDrawGlyph(lh);
        OledUpdate();
    }

    DelayMs(70);

     OledSetCursor(i+3, 3);
     OledDrawGlyph(b);
     OledSetCursor(i, 3);
     OledDrawGlyph(hl);

     OledSetCursor(i+2, 3);
     OledDrawGlyph(lh);
     OledUpdate();
DelayMs(70);
     OledSetCursor(i+2, 3);
     OledDrawGlyph(b);
     OledSetCursor(i+1, 3);
     OledDrawGlyph(lh);
     OledUpdate();
DelayMs(70);
     OledSetCursor(i+1, 3);
     OledDrawGlyph(b);
     OledSetCursor(i, 3);
     OledDrawGlyph(lh);
     DelayMs(70);
     OledSetCursor(i, 3);
     OledDrawGlyph(b);
     OledUpdate();
}

void intro_ani(char rh, char h, char b)
{
    int i;
    for( i = 0; i < 16; i++)
    {
        DelayMs(70);
        if(i)
        {
            OledSetCursor(i-1, 3);
            OledDrawGlyph(b);
            OledSetCursor(i-3, 3);
            OledDrawGlyph(b);

        }

        OledSetCursor(i, 3);
        OledDrawGlyph(h);

        OledSetCursor(i-2, 3);
        OledDrawGlyph(rh);
        OledUpdate();
    }
    DelayMs(70);

     OledSetCursor(i-3, 3);
     OledDrawGlyph(b);
     OledSetCursor(i, 3);
     OledDrawGlyph(h);

     OledSetCursor(i-2, 3);
     OledDrawGlyph(rh);
     OledUpdate();
DelayMs(70);
     OledSetCursor(i-2, 3);
     OledDrawGlyph(b);
     OledSetCursor(i-1, 3);
     OledDrawGlyph(rh);
     OledUpdate();
DelayMs(70);
     OledSetCursor(i-1, 3);
     OledDrawGlyph(b);
     OledSetCursor(i, 3);
     OledDrawGlyph(rh);
     DelayMs(200);
     OledSetCursor(i, 3);
     OledDrawGlyph(b);
     OledUpdate();

}

int btn_history()
{
    if(Button1History == 0xFF)
        return 1;
    else if(Button2History == 0xFF)
        return 2;
    else if(Button3History == 0xFF)
        return 3;
    else
        return 0;
}

void print_zombies(trace *z, trace *pz, int z_count)
{
    int max = 1;
    if(z_count == 15)
        max = 2;
    if(z_count == 16)
        max = 3;
    

    int i;
    for( i = 0; i < max; i++)
    {
        OledSetCursor(pz[i].x, pz[i].y);
        OledDrawGlyph(get_blank());

        OledSetCursor(z[i].x, z[i].y);
        if(z[i].dir == 0)
            OledDrawGlyph(get_righth());
        if(z[i].dir == 1)
            OledDrawGlyph(get_lefth());
    }
}

bool check_heart(trace *h, int hx, int hy, int i)
{
    if(hx == h[i].x && hy == h[i].y)
        return true;
    return false;
}

void draw_balloons(int d, int t, char tb, char bb, char s, char s_1)
{
    OledSetCursor(d, t);
    OledDrawGlyph(tb);
    OledSetCursor(d, t+1);
    OledDrawGlyph(bb);
    OledSetCursor(d, t+2);
    OledDrawGlyph(s);
    OledSetCursor(d, t+3);
    OledDrawGlyph(s_1);
    OledUpdate();
}
//the second part of my balloons animation
void draw_balloons2(int d, int t, char tb, char bb, char s, char s_1, char b)
{
    OledSetCursor(d, t-3);
    OledDrawGlyph(tb);
    OledSetCursor(d, t-2);
    OledDrawGlyph(bb);
    OledSetCursor(d, t-1);
    OledDrawGlyph(s);
    OledSetCursor(d, t);
    OledDrawGlyph(s_1);
    OledSetCursor(d, t+1);
    OledDrawGlyph(b);
    OledSetCursor(d, t+2);
    OledDrawGlyph(b);
    OledUpdate();
}