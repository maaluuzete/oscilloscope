#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_MCP23017.h>
#include <hardware/adc.h>
#include <math.h>

#define tft_cs 5
#define tft_dc 6
#define tft_rst 7

Adafruit_ILI9341 tft(tft_cs,tft_dc,tft_rst);
Adafruit_MCP23017 mcp;

#define signal_pin 26

int screen_w=320;
int screen_h=240;
int grid_top=20;
int grid_h=200;
int samples=320;

uint16_t raw[320];
float volts[320];

float volts_divs[]={0.1,0.2,0.5,1.0};
int volts_idx=3;

int time_divs[]={10,20,50,100,200,500};
int time_idx=3;

float vert_offset=0.0;
int horiz_offset=0;

float trigger_lvl=1.65;
bool trigger_rise=true;
bool running=true;

float meas_vpp=0.0;
float meas_vrms=0.0;
float meas_freq=0.0;

uint16_t pastel_pink=0xf3bf;
uint16_t grid_pink=0xf1ae;
uint16_t trace_cyan=0x07ff;

void splash()
{
 tft.fillScreen(pastel_pink);
 tft.setTextColor(0xffff);
 tft.setTextSize(2);
 tft.setCursor(40,90);
 tft.print("made with love");
 tft.setCursor(60,120);
 tft.print("& solder by");
 tft.setTextSize(3);
 tft.setCursor(60,155);
 tft.print("maluzete");
 delay(2500);
}

void draw_grid()
{
 tft.fillRect(0,grid_top,screen_w,grid_h,0x0000);
 for(int i=0;i<=10;i++)
 {
  int x=i*(screen_w/10);
  tft.drawFastVLine(x,grid_top,grid_h,grid_pink);
 }
 for(int i=0;i<=8;i++)
 {
  int y=grid_top+i*(grid_h/8);
  tft.drawFastHLine(0,y,screen_w,grid_pink);
 }
}

void acquire()
{
 int delay_us=(time_divs[time_idx]*10)/samples;
 for(int i=0;i<samples;i++)
 {
  raw[i]=analogRead(signal_pin);
  delayMicroseconds(delay_us);
 }
 for(int i=0;i<samples;i++)
 {
  volts[i]=(raw[i]/4095.0)*3.3;
 }
}

int find_trigger()
{
 for(int i=1;i<samples;i++)
 {
  if(trigger_rise)
  {
   if(volts[i-1]<trigger_lvl && volts[i]>=trigger_lvl){return i;}
  }
  else
  {
   if(volts[i-1]>trigger_lvl && volts[i]<=trigger_lvl){return i;}
  }
 }
 return 0;
}

void compute_measures()
{
 float vmin=10.0;
 float vmax=0.0;
 float sumsq=0.0;
 int crossings=0;
 float last_cross=0.0;
 float period_sum=0.0;

 for(int i=0;i<samples;i++)
 {
  if(volts[i]<vmin){vmin=volts[i];}
  if(volts[i]>vmax){vmax=volts[i];}
  sumsq+=volts[i]*volts[i];
 }

 for(int i=1;i<samples;i++)
 {
  if(volts[i-1]<trigger_lvl && volts[i]>=trigger_lvl)
  {
   float t=i*(time_divs[time_idx]*10e-6/samples);
   if(crossings>0){period_sum+=t-last_cross;}
   last_cross=t;
   crossings++;
  }
 }

 meas_vpp=vmax-vmin;
 meas_vrms=sqrt(sumsq/samples);
 if(crossings>1){meas_freq=1.0/(period_sum/(crossings-1));}
 else{meas_freq=0.0;}
}

void draw_wave()
{
 int trig=find_trigger();
 for(int i=1;i<screen_w;i++)
 {
  int i1=trig+i-1+horiz_offset;
  int i2=trig+i+horiz_offset;
  if(i2>=samples){break;}
  float v1=volts[i1]-vert_offset;
  float v2=volts[i2]-vert_offset;
  int y1=grid_top+grid_h/2-(v1/volts_divs[volts_idx])*(grid_h/8);
  int y2=grid_top+grid_h/2-(v2/volts_divs[volts_idx])*(grid_h/8);
  tft.drawLine(i-1,y1,i,y2,trace_cyan);
 }
}

void draw_ui()
{
 tft.fillRect(0,0,screen_w,20,0x0000);
 tft.setTextColor(0xffff);
 tft.setTextSize(1);
 tft.setCursor(5,5);
 tft.print(volts_divs[volts_idx]);
 tft.print("v/div ");
 tft.print(time_divs[time_idx]);
 tft.print("us/div");
 tft.setCursor(170,5);
 tft.print("vpp:");
 tft.print(meas_vpp,2);
 tft.setCursor(240,5);
 tft.print("hz:");
 tft.print(meas_freq,1);
}

void setup()
{
 adc_init();
 adc_gpio_init(signal_pin);
 tft.begin();
 tft.setRotation(1);
 mcp.begin();
 for(int i=0;i<16;i++)
 {
  mcp.pinMode(i,INPUT);
  mcp.pullUp(i,HIGH);
 }
 splash();
 draw_grid();
}

void loop()
{
 if(running)
 {
  acquire();
  compute_measures();
  draw_grid();
  draw_wave();
  draw_ui();
 }
 if(!mcp.digitalRead(14))
 {
  running=!running;
  delay(300);
 }
}
