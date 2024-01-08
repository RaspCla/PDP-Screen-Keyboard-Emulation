#pragma once

namespace PinDefinition 
{ 
      using Register = volatile byte *;

      
// Die Arrays sind automatisch generierter Code, bitte nicht haendisch aendern
// Der Index ist jeweils die Arduino Pinnummer

// UNO, Nano, Pro Mini usw.
#if defined(__AVR_ATmega328P__) | defined(__AVR_ATmega328__) | defined(__AVR_ATmega168P__) | defined(__AVR_ATmega168__)  
 constexpr byte pinMaske[]     = {0b1,0b10,0b100,0b1000,0b10000,0b100000,0b1000000,0b10000000,0b1,0b10,0b100,0b1000,0b10000,0b100000,0b1,0b10,0b100,0b1000,0b10000,0b100000,};
 constexpr Register portList[] = {&PORTD,&PORTD,&PORTD,&PORTD,&PORTD,&PORTD,&PORTD,&PORTD,&PORTB,&PORTB,&PORTB,&PORTB,&PORTB,&PORTB,&PORTC,&PORTC,&PORTC,&PORTC,&PORTC,&PORTC,};
 constexpr Register ddrList[]  = {&DDRD,&DDRD,&DDRD,&DDRD,&DDRD,&DDRD,&DDRD,&DDRD,&DDRB,&DDRB,&DDRB,&DDRB,&DDRB,&DDRB,&DDRC,&DDRC,&DDRC,&DDRC,&DDRC,&DDRC,};
 constexpr Register pinList[]  = {&PIND,&PIND,&PIND,&PIND,&PIND,&PIND,&PIND,&PIND,&PINB,&PINB,&PINB,&PINB,&PINB,&PINB,&PINC,&PINC,&PINC,&PINC,&PINC,&PINC,};
#endif 

// Leonardo, Micro, usw.
#if defined(__AVR_ATmega32U4__)
 constexpr byte pinMaske[]     = {0b100,0b1000,0b10,0b1,0b10000,0b1000000,0b10000000,0b1000000,0b10000,0b100000,0b1000000,0b10000000,0b1000000,0b10000000,0b1000,0b10,0b100,0b1,0b10000000,0b1000000,0b100000,0b10000,0b10,0b1,0b10000,0b10000000,0b10000,0b100000,0b1000000,0b1000000,0b100000,};
 constexpr Register portList[] = {&PORTD,&PORTD,&PORTD,&PORTD,&PORTD,&PORTC,&PORTD,&PORTE,&PORTB,&PORTB,&PORTB,&PORTB,&PORTD,&PORTC,&PORTB,&PORTB,&PORTB,&PORTB,&PORTF,&PORTF,&PORTF,&PORTF,&PORTF,&PORTF,&PORTD,&PORTD,&PORTB,&PORTB,&PORTB,&PORTD,&PORTD,};
 constexpr Register ddrList[]  = {&DDRD,&DDRD,&DDRD,&DDRD,&DDRD,&DDRC,&DDRD,&DDRE,&DDRB,&DDRB,&DDRB,&DDRB,&DDRD,&DDRC,&DDRB,&DDRB,&DDRB,&DDRB,&DDRF,&DDRF,&DDRF,&DDRF,&DDRF,&DDRF,&DDRD,&DDRD,&DDRB,&DDRB,&DDRB,&DDRD,&DDRD,};
 constexpr Register pinList[]  = {&PIND,&PIND,&PIND,&PIND,&PIND,&PINC,&PIND,&PINE,&PINB,&PINB,&PINB,&PINB,&PIND,&PINC,&PINB,&PINB,&PINB,&PINB,&PINF,&PINF,&PINF,&PINF,&PINF,&PINF,&PIND,&PIND,&PINB,&PINB,&PINB,&PIND,&PIND,};
#endif   

// Mega
#if defined(__AVR_ATmega2560__) | defined(__AVR_ATmega1280__)
 constexpr byte pinMaske[]     = {0b1,0b10,0b10000,0b100000,0b100000,0b1000,0b1000,0b10000,0b100000,0b1000000,0b10000,0b100000,0b1000000,0b10000000,0b10,0b1,0b10,0b1,0b1000,0b100,0b10,0b1,0b1,0b10,0b100,0b1000,0b10000,0b100000,0b1000000,0b10000000,0b10000000,0b1000000,0b100000,0b10000,0b1000,0b100,0b10,0b1,0b10000000,0b100,0b10,0b1,0b10000000,0b1000000,0b100000,0b10000,0b1000,0b100,0b10,0b1,0b1000,0b100,0b10,0b1,0b1,0b10,0b100,0b1000,0b10000,0b100000,0b1000000,0b10000000,0b1,0b10,0b100,0b1000,0b10000,0b100000,0b1000000,0b10000000,};
 constexpr Register portList[] = {&PORTE,&PORTE,&PORTE,&PORTE,&PORTG,&PORTE,&PORTH,&PORTH,&PORTH,&PORTH,&PORTB,&PORTB,&PORTB,&PORTB,&PORTJ,&PORTJ,&PORTH,&PORTH,&PORTD,&PORTD,&PORTD,&PORTD,&PORTA,&PORTA,&PORTA,&PORTA,&PORTA,&PORTA,&PORTA,&PORTA,&PORTC,&PORTC,&PORTC,&PORTC,&PORTC,&PORTC,&PORTC,&PORTC,&PORTD,&PORTG,&PORTG,&PORTG,&PORTL,&PORTL,&PORTL,&PORTL,&PORTL,&PORTL,&PORTL,&PORTL,&PORTB,&PORTB,&PORTB,&PORTB,&PORTF,&PORTF,&PORTF,&PORTF,&PORTF,&PORTF,&PORTF,&PORTF,&PORTK,&PORTK,&PORTK,&PORTK,&PORTK,&PORTK,&PORTK,&PORTK,};
 constexpr Register ddrList[]  = {&DDRE,&DDRE,&DDRE,&DDRE,&DDRG,&DDRE,&DDRH,&DDRH,&DDRH,&DDRH,&DDRB,&DDRB,&DDRB,&DDRB,&DDRJ,&DDRJ,&DDRH,&DDRH,&DDRD,&DDRD,&DDRD,&DDRD,&DDRA,&DDRA,&DDRA,&DDRA,&DDRA,&DDRA,&DDRA,&DDRA,&DDRC,&DDRC,&DDRC,&DDRC,&DDRC,&DDRC,&DDRC,&DDRC,&DDRD,&DDRG,&DDRG,&DDRG,&DDRL,&DDRL,&DDRL,&DDRL,&DDRL,&DDRL,&DDRL,&DDRL,&DDRB,&DDRB,&DDRB,&DDRB,&DDRF,&DDRF,&DDRF,&DDRF,&DDRF,&DDRF,&DDRF,&DDRF,&DDRK,&DDRK,&DDRK,&DDRK,&DDRK,&DDRK,&DDRK,&DDRK,};
 constexpr Register pinList[]  = {&PINE,&PINE,&PINE,&PINE,&PING,&PINE,&PINH,&PINH,&PINH,&PINH,&PINB,&PINB,&PINB,&PINB,&PINJ,&PINJ,&PINH,&PINH,&PIND,&PIND,&PIND,&PIND,&PINA,&PINA,&PINA,&PINA,&PINA,&PINA,&PINA,&PINA,&PINC,&PINC,&PINC,&PINC,&PINC,&PINC,&PINC,&PINC,&PIND,&PING,&PING,&PING,&PINL,&PINL,&PINL,&PINL,&PINL,&PINL,&PINL,&PINL,&PINB,&PINB,&PINB,&PINB,&PINF,&PINF,&PINF,&PINF,&PINF,&PINF,&PINF,&PINF,&PINK,&PINK,&PINK,&PINK,&PINK,&PINK,&PINK,&PINK,};
#endif  
    
      constexpr Register getOutPort(const byte pin)
      {
        return portList[pin];
      };
      
      constexpr Register getDdrPort(const byte pin)
      {
        return ddrList[pin];
      };
      
      constexpr Register getInPort(const byte pin)
      {
        return pinList[pin];
      };
      
      
      constexpr byte getMaske(const byte pin)
      {
        return pinMaske[pin];
      };



  template<byte arduinoPin>
  class Pin
  {
      protected:
      void inline __attribute__((always_inline)) initInput()
      {
          *getDdrPort(arduinoPin) &= ~getMaske(arduinoPin);
      }
      
      void inline __attribute__((always_inline)) initPullup()
      { 
          *getDdrPort(arduinoPin) &= ~getMaske(arduinoPin);
          *getOutPort(arduinoPin) |=  getMaske(arduinoPin);
      }
      
      void inline __attribute__((always_inline)) initOutput()
      {
        
          *getDdrPort(arduinoPin) |= getMaske(arduinoPin);
      }
  
      bool inline __attribute__((always_inline)) isHigh()
      {
        return *getInPort(arduinoPin)  &  getMaske(arduinoPin);
      }
  
      void inline __attribute__((always_inline)) setHigh()
      {
        *getOutPort(arduinoPin)  |=  getMaske(arduinoPin);
      }
      
      void inline __attribute__((always_inline)) setLow()
      {
        *getOutPort(arduinoPin)  &=  ~getMaske(arduinoPin);
      }
      
      void inline __attribute__((always_inline)) toggle()
      {
        *getInPort(arduinoPin)  =  getMaske(arduinoPin);
      }
  };


  template<byte arduinoPin>
  class InputPin : protected Pin<arduinoPin> 
  {
      public:
      using Pin<arduinoPin>::initPullup;
      using Pin<arduinoPin>::isHigh;
    
      void inline __attribute__((always_inline)) init()
      {
          Pin<arduinoPin>::initInput();
      }
  };

  template<byte arduinoPin>
  class OutputPin : protected Pin<arduinoPin>
  {
      public:
      using Pin<arduinoPin>::toggle;
      using Pin<arduinoPin>::isHigh;
      using Pin<arduinoPin>::setHigh;
      using Pin<arduinoPin>::setLow;
    
      void inline __attribute__((always_inline)) init()
      {
         Pin<arduinoPin>::initOutput();
      }
     
      inline __attribute__((always_inline)) operator bool()
      {
        return  Pin<arduinoPin>::isHigh();
      }
      
      void inline __attribute__((always_inline)) set(const bool value)
      {
        if(value) setHigh(); else setLow();
      }
      
      bool inline __attribute__((always_inline)) operator = (const bool value)
      {
        set(value);
        return value;
      }
  };

   template<byte arduinoPin>
   class RelaisINV : protected OutputPin<arduinoPin>
   {
      public:
      void inline __attribute__((always_inline)) init()
      {
        // init, ohne Low Puls
        OutputPin<arduinoPin>::setHigh();
        OutputPin<arduinoPin>::initOutput();
      }
      
      void inline __attribute__((always_inline)) ein()
      {
         OutputPin<arduinoPin>::setLow();
      }
      
      void inline __attribute__((always_inline)) aus()
      {
         OutputPin<arduinoPin>::setHigh();
      }
   };

   
   template<byte arduinoPin>
   class TasterGND : protected InputPin<arduinoPin>
   {
    
      public:
      using InputPin<arduinoPin>::init;
      using InputPin<arduinoPin>::initPullup;
      bool inline __attribute__((always_inline)) pressed()
      {
        return !InputPin<arduinoPin>::isHigh();
      }
      
      bool inline __attribute__((always_inline)) released()
      {
        return InputPin<arduinoPin>::isHigh();
      }
      
      inline __attribute__((always_inline)) operator bool()
      {
        return  pressed();
      }
   };

}

