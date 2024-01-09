// ===================================================================================================================================
//
// Keyboard Replacement (Touchscreen) for rebuild PDP - Programmer to program Philips/Simoco FM1100 Transceiver
// Display is 240x400
//
// Last Build on Arduino IDE 2.2.1
// 
// -----------------------------------------------------------------------------------------------------------------------------------
// Ideas for Improvements 
// - may be usage of canvas for the touch screen could improve screen handling and separate Text from buttons (low priority) 
//   more information: https://cdn-learn.adafruit.com/downloads/pdf/adafruit-gfx-graphics-library.pdf
// -----------------------------------------------------------------------------------------------------------------------------------
//
// Change History:
// Who    When        What
// ------+------------+---------------------------------------------------------------------------------------------------------------
// Claus  2024-01-09  Resync if command is not known (1 nibble offset in receivebuffer), much more stable, now
// Claus  2024-01-05  improvements 
// Claus  2024-01-02  first stable version with display emulation
// Claus  2023-12-21  first steps with display emulation
// Claus  2023-12-14  Keyboard emululation is running via touch display
// Claus  2023-11-12  first steps with Arduino Mega 2560 and Touch display (previously pdp-FM1000)
// 
// -----------------------------------------------------------------------------------------------------------------------------------
// 
// Some hint in regard to the Display emulation
// - Display starts in 8 Bit Mode after Power On
// - If 4 Bit mode is set the first time only one nibble is written for this command (becaue still in 8 Bit Mode)
//
// It seems the 8031 Routine is always writing in 4 Bit Mode, but I guess this does not matter because the second 
// write always set the 4 Pins to low in 8 Bit Mode this means 0b00000000 which is not defined as command
// whils start up of 8031 there are invalid information at the data display data bus, therefore we wait for the first "set 8 Bit Mode"
// Additional all display information will be discarded if RS and RW lines are different when E goes high and afterwards low. 
//
// Only "write data" and "write command" instructions will will be writte to ring buffer
// After instruction is written to ring buffer Arduino Routine is set to busy until instuction is processed in main routine
//
// While Reset port are high-Z. That means PullUp or PullDown resistors will be needed if defined level is prerequisite
// 


#include <Arduino.h>
#include "PinDefinition.h"
using namespace PinDefinition;          // for fast Pin handling
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;

#include <TouchScreen.h>
#define MINPRESSURE 200
#define MAXPRESSURE 1000
#define ORIENTATION 1                   //change screen rotation 0=Portrait, 1=Landscape

const int msg_border_horiz = 8;         // horizontal Border of text box   
const int msg_border_vert  = 21;        // vertical Border of text box 

#define TextSize 4                      // 1..4; 
#define PixelPerChar 24                 // TextSize = 4 -> 24Pixel width (cursor x-position = n*24)


const int button_width_divider = 4;     // how mouch of the TFT width want we use for Keyboard buttons

int button_hight = 40;                  // default value set for 240x400 in Portrait     
int button_width = 60;     

byte DispMirror[] = {' ', ' ', ' ', ' ', ' ',' ', ' ', ' ', ' ', ' ',' ', ' ', ' ', ' ', ' ', ' ',  ' ','7'};      // Mirror DD-RAM (16 char, to rebuild cursor)

uint16_t ID ;
int Shift = 0;


// =========================================================================================================
// Pin mapping for Mega2560
//
//
// Pin Definition for Keyboard Pins
// --------------------------------
//
// ProMicro PL0..PL3  = D49...D46      -> 8031 P1.0..P1.3            (Datenleitungen)
// ProMicro PL4       = D45            -> 8031 /INT0 = P3.2 = Pin12  (DataReady)
#define P10_8031 49                   // PL0
#define P11_8031 48                   // PL1  
#define P12_8031 47                   // PL2
#define P13_8031 46                   // PL3
#define DataReady 45                  // PL4

// Pin Definition for emulated LCD pins
// -------------------------------------
//
#define D_Strobe  21                 // INT0/PD0/SCL: Interrupt Input for Display_EE (from PDP, goes low if dislay data availalbe) 
InputPin<21> D_StrobeRD  ;           // for fast toggle test use "PinDefinition.h"           

#define D_RS      27                 // PA5: Display_RS/8031_P3.4 (from PDP)     

#define D_RW      26                 // PA4: Display_RW/8031_P3.3 (from PDP)     
InputPin<26> D_RW_RD  ;              // for fast toggle test use "PinDefinition.h"           

#define D_D7      25                 // PA3: Display_D7/8031_P1.7 (from PDP) 
#define D_D6      24                 // PA2: Display_D6/8031_P1.6 (from PDP) 
#define D_D5      23                 // PA1: Display_D5/8031_P1.5 (from PDP) 
#define D_D4      22                 // PA0: Display_D4/8031_P1.4 (from PDP) 

//#define DebugPin   29                 // PA7: Pin for Debuggin     
OutputPin<29> DebugPin  ;             // for fast toggle test use "PinDefinition.h"           


//OutputPin<25> D_D7  ;             // for fast toggle test use "PinDefinition.h"           

#define DisplPort PINA
#define DisplMask  0b00111111
#define Displ8BitM 0b11000000
#define Displ4BitM 0b10000000
#define DisplOutMsk 0b00001111            // If data to be written to 8031 Display Port (PA0...PA3)
#define DisplInMsk 0b11110000
#define DisplRsRwMsk 0b00110000           // Mask to filter RW and RS Pins 
byte D_RsRwMem;                           // Last RS and RW Status after D_Strobe (E) went high 
byte D_RsRwD7654;                         // Variable to buffer Display Pins at begin of interrupt

byte Disp_Buffer[256];                    // Buffer for Display Data written by PDP
byte Disp_InPtr = 0;                      // Pointer for last Byte written to buffer + 1
byte Disp_OutPtr = 0;                     // Pointer for last Byte written to buffer + 1

#define DD_RAM 0                          
#define CG_RAM 1
byte AktivD_RAM = DD_RAM;                 // Contains information which Display RAM is active

byte D_Adr = 0;                           // To send current Adress to 8031 (DD_RAM or CG_RAM)
byte D_CgAdr = 0;                         // Current CG-RAM address
byte D_DdAdr = 0;                         // Current DD-RAM address
byte D_BufFul = 0;                        // shows whether Display ringbuffer is full

byte D_4B_1_2 = 1;                        // Signals for 4 bit mode whether nibble 1 or 2 to be handled next                            
word D_Instruction=0;                     // Variable for instruction written to display from 8031

 
#define D_Init 0
#define D_8Bit 0b00000011
#define D_4detect 0b10101010              // 4 Bit command detected but still one byte will arrive in 8 Bit Mode                                
#define D_4Bit 0b00000010
byte D_State = D_Init;                    // Status Display Interrupt Routine
#define D_Read HIGH
#define D_Write LOW

#define D_Timeout 400000                  // If D_Strobe = E = high for more than 400msec -> Start from scratch                   
unsigned long D_Timer;


#define KeybPort  PORTL
#define KeybMask  0b11110000
#define KeybShift 0                       // how many shifts-right needed to print out digital value 

#define KeyVal_0  0b00001110
#define KeyVal_1  0b00000001
#define KeyVal_2  0b00000010
#define KeyVal_3  0b00000011
#define KeyVal_4  0b00000101
#define KeyVal_5  0b00000110
#define KeyVal_6  0b00000111
#define KeyVal_7  0b00001001
#define KeyVal_8  0b00001010
#define KeyVal_9  0b00001011
#define KeyVal_l  0b00000100
#define KeyVal_r  0b00001000
#define KeyVal_d  0b00001101
#define KeyVal_e  0b00001111
#define KeyVal_c  0b00000000
#define KeyVal_s  0b00001100


#define Baud_Rate   115200


// ALL Touch panels and wiring is DIFFERENT
// copy-paste results from TouchScreen_Calibr_native.ino
/* Claus
  const int XP = 6, XM = A2, YP = A1, YM = 7; //ID=0x9341
  const int TS_LEFT = 907, TS_RT = 136, TS_TOP = 942, TS_BOT = 139;
*/





// Claus: folgende Zeilen durch Calibrierung erstellt
const int XP = 8, XM = A2, YP = A3, YM = 9; //240x400 ID=0x7793
const int TS_LEFT = 902, TS_RT = 114, TS_TOP = 72, TS_BOT = 936;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// forward declarations
extern bool update_button(Adafruit_GFX_Button *b, bool down);
extern bool update_button_list(Adafruit_GFX_Button **pb);

Adafruit_GFX_Button Shift_btn, Delete_btn, N0_btn, Enter_btn, Right_btn, N7_btn, N8_btn, N9_btn, Left_btn, N4_btn, N5_btn, N6_btn, Lock_btn, N1_btn, N2_btn, N3_btn;

int pixel_x, pixel_y;     //Touch_getXY() updates global vars
bool Touch_getXY(void)
{
  TSPoint p = ts.getPoint();
  pinMode(YP, OUTPUT);      //restore shared pins
  pinMode(XM, OUTPUT);      //because TFT control pins
  bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
  if (pressed) {
    switch (tft.getRotation() & 3) {
      // map raw ADC values to pixel coordinates
      // most apps only use a fixed rotation e.g omit unused rotations
      case 0:      //PORTRAIT
        pixel_x = map(p.x, TS_LEFT, TS_RT, 0, tft.width());
        pixel_y = map(p.y, TS_TOP, TS_BOT, 0, tft.height());
        break;
      case 1:      //LANDSCAPE
        pixel_x = map(p.y, TS_TOP, TS_BOT, 0, tft.width());
        pixel_y = map(p.x, TS_RT, TS_LEFT, 0, tft.height());
        break;
      case 2:      //PORTRAIT REV
        pixel_x = map(p.x, TS_RT, TS_LEFT, 0, tft.width());
        pixel_y = map(p.y, TS_BOT, TS_TOP, 0, tft.height());
        break;
      case 3:      //LANDSCAPE REV
        pixel_x = map(p.y, TS_BOT, TS_TOP, 0, tft.width());
        pixel_y = map(p.x, TS_LEFT, TS_RT, 0, tft.height());
        break;
    }
  }
  return pressed;
}

#define BLACK     0x0000
#define BLUE      0x001F
#define RED       0xF800
#define GREEN     0x07E0
#define CYAN      0x07FF
#define DARKCYAN  0x041F
#define MAGENTA   0xF81F
#define YELLOW    0xFFE0
#define WHITE     0xFFFF


//
// write command to Keyboard board
//
void writeKeyboard(int ser)
{
  int KeyVal, wrongVal = 0;

  switch (ser)
  {
    case '0':
      KeyVal = KeyVal_0;
      break;

    case '1':
      KeyVal = KeyVal_1;
      break;

    case '2':
      KeyVal = KeyVal_2;
      break;

    case '3':
      KeyVal = KeyVal_3;
      break;

    case '4':
      KeyVal = KeyVal_4;
      break;

    case '5':
      KeyVal = KeyVal_5;
      break;

    case '6':
      KeyVal = KeyVal_6;
      break;

    case '7':
      KeyVal = KeyVal_7;
      break;

    case '8':
      KeyVal = KeyVal_8;
      break;

    case '9':
      KeyVal = KeyVal_9;
      break;

    case 'l':
      KeyVal = KeyVal_l;
      break;

    case 'r':
      KeyVal = KeyVal_r;
      break;

    case 'd':
      KeyVal = KeyVal_d;
      break;

    case 'e':
      KeyVal = KeyVal_e;
      break;

    case 'c':
      KeyVal = KeyVal_c;
      break;

    case 's':
      KeyVal = KeyVal_s;
      break;

    default:
      wrongVal = 1;
      break;
  }

  if (wrongVal == 0)
  {
    KeybPort &= KeybMask;
    KeybPort |= KeyVal;
    delay(1);
    digitalWrite(DataReady, LOW);               // Create neg. pulse on DataReady Pin
    delay(100);
    digitalWrite(DataReady, HIGH);

    Serial.print("Keyboard: ");
    Serial.print(char(ser));
    Serial.print(' ');
    Serial.println(KeyVal >> KeybShift, BIN);
  }
  else wrongVal = 0;
}


// Interrupt to handle data for Display (read data pins)
// =====================================================
void DiplayInt() 
{
D_RsRwD7654 = DisplPort&DisplMask;                     // Read Display pins as early as possible 
if (D_StrobeRD.isHigh())                               // D_Strobe(Diplay-E) went "high"
{

 D_RsRwMem = D_RsRwD7654&DisplRsRwMsk;                 // Remember Status of RW and RS Pins for next E = Low Cycle
 if (D_State!=D_Init)                                  // Do nothing if wait for first command (8031 inits port) -> wait for next low edge
 {
  if (D_RW_RD.isHigh())                                // Next Command is read command, display port = output and set data to port (fist step: only busy flag)
  {                                                    // --------------------------------------------------------------------------
   if ((Disp_InPtr-Disp_OutPtr)<=240)
   {
    DDRA |= DisplOutMsk;                                // we have to write data to 8031 display pins
    PORTA &= 0b11110000;                                // If buffer not nearly full: write all Pins 0 -> not busy and Adr. = 0b (else pullups stay active -> = busy)
   }                                                    // (8031/PDP does not evaluate the address informations, as far I could understand the firmware)

   DebugPin=LOW;
   DebugPin=HIGH;
   DebugPin=LOW;
   DebugPin=HIGH;
  }
  else                                                  // Next Command is write (from 8031) -> set display ports to input
  {                                                     // ---------------------------------------------------------------
   DDRA &= DisplInMsk;                                  // we have to read data from 8031 display pins
   //PORTA |= DisplOutMsk;                              // -> Input with Pullups 
   PORTA &= DisplInMsk;                                 // -> Input without Pullups
  DebugPin=LOW;
  DebugPin=HIGH;
  }

}

DebugPin=LOW;
//D_D7=LOW;                                            // for fast toggle test use "PinDefinition.h"   

//  pinMode(D_D7, INPUT);                                              // 8031 Diplay_D7 (data lines)



}

else                                                  // D_Strobe(Diplay-E) went "low"
{                                                     // -----------------------------
if  ((D_RsRwD7654&DisplRsRwMsk) == D_RsRwMem)          // Only process information if RW and RS Pins have same status as they had if D_Strobe = E  was high !
{                                                     // else discard because no valid informatio from 8031
 if (!D_RW_RD.isHigh())                               // Process Command / Data only in Write Mode(RD Pin=Low)
 {
  if (D_State==D_Init)                                // If in init phase we check whether set 8-bit or set 4-bit commands arrived (else do nothing)
                                                      // we do this on any nibble, even if "set 4-bit" command has been processed completely. This doesn't 
                                                      // make trouble because low-nibble never contains a "set x-Bit" command  
  {                                                   // State: D_Init
   if (D_RsRwD7654==D_8Bit)                           // ------------
   {
    Disp_Buffer[Disp_InPtr]=D_RsRwD7654;              // set Bit7 in Disp_Buffer to indicate 8 Bit Mode (ignore second Byte then)
    Disp_Buffer[Disp_InPtr++]|=Displ8BitM;     
    D_State=D_8Bit;                                   // Next sequence should be init to 8 Bit Mode  
    DebugPin=HIGH;
    DebugPin=LOW;
    DebugPin=HIGH;
    DebugPin=LOW;
    DebugPin=HIGH;
    DebugPin=LOW;
   }
  }
  else if ((D_State==D_8Bit)||(D_State==D_4detect))     // State: 8-Bit mode
  {                                                     // -----------------
   Disp_Buffer[Disp_InPtr]=D_RsRwD7654;                 // set Bit7 in Disp_Buffer to indicate 8 Bit Mode (ignore second Byte then)

   if (D_State==D_4detect)
   {
    D_State=D_4Bit;
   }
   else if (Disp_Buffer[Disp_InPtr]==(D_4Bit))
   {
    D_State=D_4detect;
   }

   Disp_Buffer[Disp_InPtr++]|=Displ8BitM;               // if 4 Bit mode detected set 4 Bit mode but still process next nibble in 8 Bit mode    

   DebugPin=HIGH;
   DebugPin=LOW;
   DebugPin=HIGH;
   DebugPin=LOW;
  }
  else                                                  // State: 4 Bit mode, dont set Bit7 in Disp_Buffer to indicate 4 Bit Mode (second byte will arrive)
  {                                                     // ------------------------------------------------------------------------------------------------
                                                        // Dont check any content anymore else we have to differ between High- and Low Nibble ! 
   Disp_Buffer[Disp_InPtr]=D_RsRwD7654;                 // clr Bit7 in Disp_Buffer to indicate 4 Bit Mode
   Disp_Buffer[Disp_InPtr++]|=Displ4BitM;               // signal 4 Bit Mode    
   DebugPin=HIGH;
   DebugPin=LOW;
  }

// Disp_Buffer[Disp_InPtr++]=DisplPort;                 // Buffer for Display Data written by PDP, Needs about 1usec

// D_D7=HIGH;                                           // for fast toggle test use "PinDefinition.h"   

 }
}

DebugPin=HIGH;
 
}

}


// Print Commandlist to Monitor Console
// ====================================
void helpscreen()
{
  if (Serial)
  {
    Serial.println("Choice:");
    Serial.println(" 1          | Shift: A");
    Serial.println(" 2          | Shift: B");
    Serial.println(" 3          | Shift: C");
    Serial.println(" 4          | Shift: D");
    Serial.println(" 5          | Shift: E");
    Serial.println(" 6          | Shift: F");
    Serial.println(" 7          | Shift: F1");
    Serial.println(" 8          | Shift: F2");
    Serial.println(" 9          | Shift: F3");
    Serial.println(" 0          | Shift: ESCAPE");
    Serial.println(" l: <-      | Shift: COPY[<- (to radio/CDP)");
    Serial.println(" r: ->      | Shift: COPY->] (from radio/CDP)");
    Serial.println(" d: Delete  | Shift: CANCEL");
    Serial.println(" e: Enter   | Shift: CHECK");
    Serial.println(" c: Lock    | Shift: Channel LOCK");
    Serial.println(" s: Shift");
    Serial.println();
    Serial.println(" h: Refresh Screen (Arduino Monitor Window)");
    Serial.println();
  }
}

//
// Reset Display Buffer Mirror 
// ===========================
//
void   ResetDisplMirror()
{
 for (byte z=0; z<=15; z++)  
 {
  DispMirror[z] = ' ';                   // Clear display mirror, also
 }
//  MirrorPtr = 0;                         // Reset pointer to Mirror
}


//
// Mark Cursor at Display 
// ======================
//
void   DisplayCursor()
{
   tft.setCursor(msg_border_horiz+(D_DdAdr*PixelPerChar), msg_border_vert+3);
   tft.setTextColor(BLACK, WHITE);                                                // Text Inverse 
   tft.print((char)DispMirror[D_DdAdr]); 
   tft.setTextColor(WHITE, BLACK);                                                // Text Normal
   tft.setCursor(msg_border_horiz+(D_DdAdr*PixelPerChar), msg_border_vert+3);

}


//
// Initialize text area of Touchscreen
// ===================================
//

void  initTextSize()
{
tft.setTextSize(TextSize);              // 1..4; 4 -> 24Pixel width (cursor x-position = n*24)
tft.setTextColor(WHITE, BLACK);         // First parameter: Text Color, second parameter: backround color
tft.setTextWrap(false);                 // true/false

}


void  initText()
{
tft.setCursor(msg_border_horiz, msg_border_vert+3);
initTextSize();
}


//
// Touchscreen Init (no Shift pressed)
// ===================================
//
void  initTouch_NoShift()
{
  Shift_btn.initButton(&tft, 0.5*button_width, (tft.height()-0.5*button_hight), button_width, button_hight, BLACK, WHITE, DARKCYAN, "Shift", 2);
  Delete_btn.initButton(&tft, 1.5*button_width, (tft.height()-0.5*button_hight), button_width, button_hight, BLACK, WHITE, BLACK, "Delete", 2);
  N0_btn.initButton(&tft, 2.5*button_width, (tft.height()-0.5*button_hight), button_width, button_hight, BLACK, WHITE, BLACK, "0", 2);
  Enter_btn.initButton(&tft, 3.5*button_width, (tft.height()-0.5*button_hight), button_width, button_hight, BLACK, WHITE, BLACK, "Enter", 2);
  Right_btn.initButton(&tft, 0.5*button_width, (tft.height()-1.5*button_hight), button_width, button_hight, BLACK, WHITE, BLACK, "->", 2);
  N7_btn.initButton(&tft, 1.5*button_width, (tft.height()-1.5*button_hight), button_width, button_hight, BLACK, WHITE, BLACK, "7", 2);
  N8_btn.initButton(&tft, 2.5*button_width, (tft.height()-1.5*button_hight), button_width, button_hight, BLACK, WHITE, BLACK, "8", 2);
  N9_btn.initButton(&tft, 3.5*button_width, (tft.height()-1.5*button_hight), button_width, button_hight, BLACK, WHITE, BLACK, "9", 2);
  Left_btn.initButton(&tft, 0.5*button_width, (tft.height()-2.5*button_hight), button_width, button_hight, BLACK, WHITE, BLACK, "<-", 2);
  N4_btn.initButton(&tft, 1.5*button_width, (tft.height()-2.5*button_hight), button_width, button_hight, BLACK, WHITE, BLACK, "4", 2);
  N5_btn.initButton(&tft, 2.5*button_width, (tft.height()-2.5*button_hight), button_width, button_hight, BLACK, WHITE, BLACK, "5", 2);
  N6_btn.initButton(&tft, 3.5*button_width, (tft.height()-2.5*button_hight), button_width, button_hight, BLACK, WHITE, BLACK, "6", 2);
  Lock_btn.initButton(&tft, 0.5*button_width, (tft.height()-3.5*button_hight), button_width, button_hight, BLACK, WHITE, BLACK, "Lock", 2);
  N1_btn.initButton(&tft, 1.5*button_width, (tft.height()-3.5*button_hight), button_width, button_hight, BLACK, WHITE, BLACK, "1", 2);
  N2_btn.initButton(&tft, 2.5*button_width, (tft.height()-3.5*button_hight), button_width, button_hight, BLACK, WHITE, BLACK, "2", 2);
  N3_btn.initButton(&tft, 3.5*button_width, (tft.height()-3.5*button_hight), button_width, button_hight, BLACK, WHITE, BLACK, "3", 2);
  Shift_btn.drawButton(false);
  Delete_btn.drawButton(false);
  N0_btn.drawButton(false);
  Enter_btn.drawButton(false);
  Right_btn.drawButton(false);
  N7_btn.drawButton(false);
  N8_btn.drawButton(false);
  N9_btn.drawButton(false);
  Left_btn.drawButton(false);
  N4_btn.drawButton(false);
  N5_btn.drawButton(false);
  N6_btn.drawButton(false);
  Lock_btn.drawButton(false);
  N1_btn.drawButton(false);
  N2_btn.drawButton(false);
  N3_btn.drawButton(false);

  initTextSize();             // texsize to be refreshed (dont know why)
}

void  initTouch_Shift()
{
  Shift_btn.initButton(&tft, 0.5*button_width, (tft.height()-0.5*button_hight), button_width, button_hight, BLACK, WHITE, BLACK, "Shift", 2);
  Delete_btn.initButton(&tft, 1.5*button_width, (tft.height()-0.5*button_hight), button_width, button_hight, BLACK, WHITE, DARKCYAN, "Cancel", 2);
  N0_btn.initButton(&tft, 2.5*button_width, (tft.height()-0.5*button_hight), button_width, button_hight, BLACK, WHITE, DARKCYAN, "Escape", 2);
  Enter_btn.initButton(&tft, 3.5*button_width, (tft.height()-0.5*button_hight), button_width, button_hight, BLACK, WHITE, DARKCYAN, "Check", 2);
  Right_btn.initButton(&tft, 0.5*button_width, (tft.height()-1.5*button_hight), button_width, button_hight, BLACK, WHITE, DARKCYAN, "Copy  In", 2);
  N7_btn.initButton(&tft, 1.5*button_width, (tft.height()-1.5*button_hight), button_width, button_hight, BLACK, WHITE, DARKCYAN, "F1", 2);
  N8_btn.initButton(&tft, 2.5*button_width, (tft.height()-1.5*button_hight), button_width, button_hight, BLACK, WHITE, DARKCYAN, "F2", 2);
  N9_btn.initButton(&tft, 3.5*button_width, (tft.height()-1.5*button_hight), button_width, button_hight, BLACK, WHITE, DARKCYAN, "F3", 2);
  Left_btn.initButton(&tft, 0.5*button_width, (tft.height()-2.5*button_hight), button_width, button_hight, BLACK, WHITE, DARKCYAN, "Copy Out", 2);
  N4_btn.initButton(&tft, 1.5*button_width, (tft.height()-2.5*button_hight), button_width, button_hight, BLACK, WHITE, DARKCYAN, "D", 2);
  N5_btn.initButton(&tft, 2.5*button_width, (tft.height()-2.5*button_hight), button_width, button_hight, BLACK, WHITE, DARKCYAN, "E", 2);
  N6_btn.initButton(&tft, 3.5*button_width, (tft.height()-2.5*button_hight), button_width, button_hight, BLACK, WHITE, DARKCYAN, "F", 2);
  Lock_btn.initButton(&tft, 0.5*button_width, (tft.height()-3.5*button_hight), button_width, button_hight, BLACK, WHITE, DARKCYAN, "Chn Lock", 2);
  N1_btn.initButton(&tft, 1.5*button_width, (tft.height()-3.5*button_hight), button_width, button_hight, BLACK, WHITE, DARKCYAN, "A", 2);
  N2_btn.initButton(&tft, 2.5*button_width, (tft.height()-3.5*button_hight), button_width, button_hight, BLACK, WHITE, DARKCYAN, "B", 2);
  N3_btn.initButton(&tft, 3.5*button_width, (tft.height()-3.5*button_hight), button_width, button_hight, BLACK, WHITE, DARKCYAN, "C", 2);
  Shift_btn.drawButton(false);
  Delete_btn.drawButton(false);
  N0_btn.drawButton(false);
  Enter_btn.drawButton(false);
  Right_btn.drawButton(false);
  N7_btn.drawButton(false);
  N8_btn.drawButton(false);
  N9_btn.drawButton(false);
  Left_btn.drawButton(false);
  N4_btn.drawButton(false);
  N5_btn.drawButton(false);
  N6_btn.drawButton(false);
  Lock_btn.drawButton(false);
  N1_btn.drawButton(false);
  N2_btn.drawButton(false);
  N3_btn.drawButton(false);

  initTextSize();             // texsize to be refreshed (dont know why)
}

// Setup
//======================================================================================================
void setup(void)
{
#if defined(__arm__) || defined(ESP32) //default to 12-bit ADC
  analogReadResolution(10); //Adafruit TouchScreen.h expects 10-bit
#endif

digitalWrite(DataReady, HIGH);                                    // DataReady shall become High, directly
pinMode(DataReady, OUTPUT);                                       //
pinMode(P10_8031, OUTPUT);
pinMode(P11_8031, OUTPUT);
pinMode(P12_8031, OUTPUT);
pinMode(P13_8031, OUTPUT);

pinMode(D_RW, INPUT);                                              // 8031 Diplay_RW -> Low if data shall be written
D_RW_RD.init();
pinMode(D_RS, INPUT);                                              // 8031 Diplay_RS -> Low for instruction- / high for data- access
//  pinMode(D_D7, INPUT);                                               // 8031 Diplay_D7 (data lines)
//  pinMode(D_D7, OUTPUT);                                              // 8031 Diplay_D7 (data lines)
//  D_D7.init();                                                        // for fast toggle test use "PinDefinition.h"   
pinMode(D_D7, INPUT);                                              // 8031 Diplay_D6 
pinMode(D_D6, INPUT);                                              // 8031 Diplay_D6 
pinMode(D_D5, INPUT);                                              // 8031 Diplay_D5 
pinMode(D_D4, INPUT);                                              // 8031 Diplay_D4 

//  pinMode(DebugPin, OUTPUT);                                          //  
DebugPin.init();                      // for fast toggle test use "PinDefinition.h"   


pinMode(D_Strobe, INPUT_PULLUP);                                   // 8031 Diplay_E  -> If goes low than Interrupt
attachInterrupt(digitalPinToInterrupt(D_Strobe), DiplayInt, CHANGE);   // CHANGE, FALLING, RISING, HIGH, LOW
D_StrobeRD.init();                      // for fast toggle test use "PinDefinition.h"   

Serial.begin(Baud_Rate);
ID = tft.readID();

if (ID == 0xD3D3) ID = 0x9486; // write-only shield
tft.begin(ID);
tft.setRotation(ORIENTATION);   // try different rotations
tft.fillScreen(BLACK);

Serial.print("TFT ID = 0x");
Serial.println(ID, HEX);
Serial.print("TFT-width:");
Serial.print(tft.width());

Serial.print("; TFT-height:");
Serial.println(tft.height());
Serial.println();

button_width = tft.width()/button_width_divider;
//  button_hight = 40;

  
initTouch_NoShift();                    // Init Button area of touch display 
initText();                             // Text arrea on touch display initialize

tft.print(" PDP for FM1000 "); 


helpscreen();                   // Menu anzeigen (Monitor Window)

} 

// SETUP Ende
// ====================

  
/*
   updating multiple buttons from a list

   anything more than two buttons gets repetitive

   you can place button addresses in separate lists
   e.g. for separate menu screens
*/

// Array of button addresses to behave like a list
Adafruit_GFX_Button *buttons[] = {&Shift_btn, &Delete_btn, &N0_btn, &Enter_btn, &Right_btn, &N7_btn, &N8_btn, &N9_btn, &Left_btn, &N4_btn, &N5_btn, &N6_btn, &Lock_btn, &N1_btn, &N2_btn, &N3_btn, NULL};

/* update the state of a button and redraw as reqd

   main program can use isPressed(), justPressed() etc
*/
bool update_button(Adafruit_GFX_Button *b, bool down)
{
  b->press(down && b->contains(pixel_x, pixel_y));
  if (b->justReleased())
  {
    b->drawButton(false);
    initText();                         // Claus it seems that after new initialization of touch screen (buttons) we also have to newly initialize the text area
                                        // else the text is written afterwards in default style 
  }
  if (b->justPressed())
  {
    b->drawButton(true);
    initText();                         // Claus it seems that after new initialization of touch screen (buttons) we also have to newly initialize the text area
                                        // else the text is written afterwards in default style 
  }
  return down;
}

/* most screens have different sets of buttons
   life is easier if you process whole list in one go
*/
bool update_button_list(Adafruit_GFX_Button **pb)
{
  bool down = Touch_getXY();
  for (int i = 0 ; pb[i] != NULL; i++) 
  {
    update_button(pb[i], down);
  }
  return down;
}




// Main
// =============================================================================


/* compare the simplicity of update_button_list()
*/
void loop(void)
{
  int ser, Shift_;

// Display Buffer
// --------------


while (Disp_InPtr!=Disp_OutPtr)
{

 Serial.println(Disp_Buffer[Disp_OutPtr], BIN); 

 if ( D_4B_1_2==1)                                                      // current nibble = nibble 1 = high nibble
 {                                                                      // ---------------------------------------
  D_4B_1_2=2;                                                           // Next nibble is nibble 2 
  D_Instruction=Disp_Buffer[Disp_OutPtr++]<<4;                          // High Nibble + RW&RS Pins .. -> D_Instruction         
 }
 else                                                                   // current nibble = nibble 2 = low nibble
 {                                                                      // --------------------------------------
  D_4B_1_2=1;                                                           // Next nibble is nibble 1 
  D_Instruction|=(Disp_Buffer[Disp_OutPtr++]&DisplOutMsk);              // Low nibble -> D_Instruction
  Serial.print(D_Instruction, BIN); 

                                                                        // display command handler
                                                                        // ^^^^^^^^^^^^^^^^^^^^^^^
    if ((D_Instruction&0b101111111111)==0b100000110000)                 // Set 8 Bit Mode  (and operation: accept command also in 4bit mode) 
      {                                                                 // --------------
        Serial.println("   Set 8-Bit Mode"); 
      }

    else if ((D_Instruction&0b101111111111)==0b100000101000)           // Set 4 Bit Mode  (and operation: accept command also in 4bit mode) 
      Serial.println("   Set 4-Bit Mode");                             // -------------- 

    else if (D_Instruction==0b100000001000)                            // Display Off, Cursor Off, Cursor Position Off
      {                                                                // --------------------------------------------
       Serial.println("   Set Display=off, Cursor=off, Cursor Position=off"); 
      }

    else if (D_Instruction==0b100000001110)                            // Display On, Cursor On, Cursor Position Off
      {                                                                // ------------------------------------------
        Serial.println("   Set Display=on, Cursor=on, Cursor Position=off"); 
      }

    else if (D_Instruction==0b100000000001)                           // Clear Display
      {                                                               // --------------
        AktivD_RAM = DD_RAM;
        D_DdAdr = 0;
        D_Adr = D_DdAdr;
        tft.setCursor(msg_border_horiz, msg_border_vert+3);
        ResetDisplMirror();
        DisplayCursor();                                              // print character at cursor location inverse


//        tft.print("                "); 
//        tft.setCursor(msg_border_horiz, msg_border_vert+3);
//        tft.println("");

        Serial.println("   Clear Display, Set DD-RAM: 0"); 
      }


    else if (D_Instruction==0b100000000010)                           // Cursor to Home Position
      {                                                               // -----------------------
        tft.setCursor(msg_border_horiz, msg_border_vert+3);
//        tft.print("                "); 
//        tft.setCursor(msg_border_horiz, msg_border_vert+3);
        Serial.println("   Return Cursor to Home position");          
      }


    else if (D_Instruction==0b100000000100)                           // No Shift operation
      {                                                               // ------------------
        Serial.println("   Entry Mode: no shift orperation "); 
      }


    else if ((D_Instruction&0b111111000000)==0b100001000000)          // Set CG-RAM address
      {                                                               // ------------------
        Serial.print("   CG-RAM Adr.= "); 
        Serial.println(D_Instruction&0b00111111,HEX);
         D_CgAdr = (D_Instruction&0b00111111);                         // Current CG-RAM address
         D_Adr = D_CgAdr;
         AktivD_RAM = CG_RAM;                                           
      }

    else if ((D_Instruction&0b111110000000)==0b100010000000)          // Set DD-RAM address
      {                                                               // ------------------
        D_DdAdr = (D_Instruction&0b01111111);                         // Current CG-RAM address
        Serial.print("   DD-RAM Adr.= "); 
        Serial.println(D_DdAdr,HEX);
        AktivD_RAM = DD_RAM;                                      
        if ((D_Instruction&0b00111111)<= 15)                          // command only valid if address <= 15
        {
          D_Adr = D_DdAdr;
//        MirrorPtr = D_DdAdr;                                        // set pointer to display mirror, also
          DisplayCursor();                                            // print character at cursor location inverse
        }
        else
        {
         Serial.println("!!!!Error, DD-RAM Adr. > 0x0F !!!");
        }
       }


    else if ((D_Instruction&0b111100000000)==0b101000000000)          // Write to CG- or DD-RAM
      {                                                               // ----------------------
        if (AktivD_RAM == DD_RAM)
        {
         if (D_DdAdr <= 15)                                          // command only valid if address <= 15
         {
          DispMirror[D_DdAdr]=(D_Instruction&0x0FF);                 // write character to display mirro
          tft.print((char)(DispMirror[D_DdAdr]));                    // send character to TFT and increment Pointer to mirror!

          Serial.print("   Write Data to DD-RAM Adr: ");               
          Serial.print(D_Adr,HEX);
          Serial.print(", Data: ");
          Serial.println((char)(D_Instruction&0x0FF));

          D_DdAdr++;
          D_Adr = D_DdAdr;
         }
        else
         {
         Serial.println("!!!!Error, DD-RAM Adr. > 0x0F !!!");
         }

        }
        else
        {
          Serial.print("   Write Data to CG-RAM: "); 
          Serial.print(D_Adr,HEX);
          Serial.print(", Data: ");
          Serial.println(D_Instruction&0x0FF,HEX);
          D_CgAdr++; 
          D_Adr = D_CgAdr;
        }

   
      }

         
    else                                                                        // In case of unknown command do not write but resync -> OutPtr with offset 1
      {                                                                         // --------------------------------------------------------------------------
        Serial.println("   !!!  display instruction not defined, Resync  !!!"); 
        Disp_OutPtr--;                                                          // resync makes system much more stable !
        D_4B_1_2=2;                                                             // Next nibble is nibble 2 
        D_Instruction=Disp_Buffer[Disp_OutPtr++]<<4;                            // High Nibble + RW&RS Pins .. -> D_Instruction         
      }
  }
  
}

  
  
 // Touchscreen
 // ----------- 
  Shift_ = Shift;

  update_button_list(buttons);  //use helper function

  if (Lock_btn.justPressed()) {
//    tft.fillRect(msg_border_horiz, msg_border_vert, tft.width()-2*msg_border_horiz, tft.height()-(4*button_hight+2*msg_border_vert), BLUE);       // Start-X, Start-Y, X, Y
    ser = 'c';
    writeKeyboard(ser);
    Shift = 0;
  }

  if (N1_btn.justPressed()) {
//    tft.fillRect(msg_border_horiz, msg_border_vert, tft.width()-2*msg_border_horiz, tft.height()-(4*button_hight+2*msg_border_vert), WHITE);       // Start-X, Start-Y, X, Y
    ser = '1';
    writeKeyboard(ser);
    Shift = 0;
  }

  if (N2_btn.justPressed()) {
//    tft.fillRect(msg_border_horiz, msg_border_vert, tft.width()-2*msg_border_horiz, tft.height()-(4*button_hight+2*msg_border_vert), BLACK);       // Start-X, Start-Y, X, Y
    ser = '2';
    writeKeyboard(ser);
    Shift = 0;
  }

  if (N3_btn.justPressed()) {
//    tft.fillRect(msg_border_horiz, msg_border_vert, tft.width()-2*msg_border_horiz, tft.height()-(4*button_hight+2*msg_border_vert), RED);       // Start-X, Start-Y, X, Y
    ser = '3';
    writeKeyboard(ser);
    Shift = 0;
  }

  if (Left_btn.justPressed()) {
//    tft.fillRect(msg_border_horiz, msg_border_vert, tft.width()-2*msg_border_horiz, tft.height()-(4*button_hight+2*msg_border_vert), GREEN);       // Start-X, Start-Y, X, Y
    ser = 'l';
    writeKeyboard(ser);
    Shift = 0;
  }

  if (N4_btn.justPressed()) {
//    tft.fillRect(msg_border_horiz, msg_border_vert, tft.width()-2*msg_border_horiz, tft.height()-(4*button_hight+2*msg_border_vert), BLUE);       // Start-X, Start-Y, X, Y
    ser = '4';
    writeKeyboard(ser);
    Shift = 0;
  }

  if (N5_btn.justPressed()) {
//    tft.fillRect(msg_border_horiz, msg_border_vert, tft.width()-2*msg_border_horiz, tft.height()-(4*button_hight+2*msg_border_vert), WHITE);       // Start-X, Start-Y, X, Y
    ser = '5';
    writeKeyboard(ser);
    Shift = 0;
  }

  if (N6_btn.justPressed()) {
//    tft.fillRect(msg_border_horiz, msg_border_vert, tft.width()-2*msg_border_horiz, tft.height()-(4*button_hight+2*msg_border_vert), BLACK);       // Start-X, Start-Y, X, Y
    ser = '6';
    writeKeyboard(ser);
    Shift = 0;
  }

  if (Right_btn.justPressed()) {
//    tft.fillRect(msg_border_horiz, msg_border_vert, tft.width()-2*msg_border_horiz, tft.height()-(4*button_hight+2*msg_border_vert), BLACK);       // Start-X, Start-Y, X, Y
    ser = 'r';
    writeKeyboard(ser);
    Shift = 0;
  }

  if (N7_btn.justPressed()) {
//    tft.fillRect(msg_border_horiz, msg_border_vert, tft.width()-2*msg_border_horiz, tft.height()-(4*button_hight+2*msg_border_vert), BLUE);       // Start-X, Start-Y, X, Y
    ser = '7';
    writeKeyboard(ser);
    Shift = 0;
  }

  if (N8_btn.justPressed()) {
//    tft.fillRect(msg_border_horiz, msg_border_vert, tft.width()-2*msg_border_horiz, tft.height()-(4*button_hight+2*msg_border_vert), RED);       // Start-X, Start-Y, X, Y
    ser = '8';
    writeKeyboard(ser);
    Shift = 0;
  }

  if (N9_btn.justPressed()) {
//    tft.fillRect(msg_border_horiz, msg_border_vert, tft.width()-2*msg_border_horiz, tft.height()-(4*button_hight+2*msg_border_vert), GREEN);       // Start-X, Start-Y, X, Y
    ser = '9';
    writeKeyboard(ser);
    Shift = 0;
  }

  if (Shift_btn.justPressed()) {
//    tft.fillRect(msg_border_horiz, msg_border_vert, tft.width()-2*msg_border_horiz, tft.height()-(4*button_hight+2*msg_border_vert), WHITE);       // Start-X, Start-Y, X, Y
    ser = 's';
    writeKeyboard(ser);
    if (Shift == 0)
    {
      Shift = 1;
    }
    else
    {
      Shift = 0;
    }
  }

  if (Delete_btn.justPressed()) {
//    tft.fillRect(msg_border_horiz, msg_border_vert, tft.width()-2*msg_border_horiz, tft.height()-(4*button_hight+2*msg_border_vert), BLUE);       // Start-X, Start-Y, X, Y
    ser = 'd';
    writeKeyboard(ser);
    Shift = 0;
  }

  if (N0_btn.justPressed()) {
//    tft.fillRect(msg_border_horiz, msg_border_vert, tft.width()-2*msg_border_horiz, tft.height()-(4*button_hight+2*msg_border_vert), GREEN);       // Start-X, Start-Y, X, Y
    ser = '0';
    writeKeyboard(ser);
    Shift = 0;
  }

  if (Enter_btn.justPressed()) {
//    tft.fillRect(msg_border_horiz, msg_border_vert, tft.width()-2*msg_border_horiz, tft.height()-(4*button_hight+2*msg_border_vert), RED);       // Start-X, Start-Y, X, Y
    ser = 'e';
    writeKeyboard(ser);
    Shift = 0;
  }



  if (Shift_ != Shift)
  {
    if (Shift == 1) initTouch_Shift();
    else initTouch_NoShift();
  }


  // Also read input from serial Monitor
  if (Serial) {
    if (Serial.available() > 0) {
      ser = Serial.read();
      if (ser == 'h')
      { 
        // Hilfe Text ausgeben
        helpscreen();
      }
      else
      {
        writeKeyboard(ser);
      }
      Serial.setTimeout(500);
    }
  }


}
/*
  #endif
*/
