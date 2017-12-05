// Infrared remote library for Arduino: send and receive infrared signals with multiple protocols
// http://z3t0.github.io/Arduino-IRremote
#include <IRremote.h>

// IR
const byte IR_RECV_PIN = 3;

IRrecv irrecv(IR_RECV_PIN);

decode_results results;

// SSD
#define SEG_SIZE 8
#define DIG_SIZE 4

const byte SEG_PIN[SEG_SIZE] = {5, 12, 11, 10, 9, 8, 7, 6};
const byte COM_PIN[DIG_SIZE] = {14, 15, 16, 17};

byte seg_pin_value[][SEG_SIZE] =
{
 //h  a  b  c  d  e  f  g     
  {0, 1, 1, 1, 1, 1, 1, 0},  //0
  {0, 0, 1, 1, 0, 0, 0, 0},  //1
  {0, 1, 1, 0, 1, 1, 0, 1},  //2
  {0, 1, 1, 1, 1, 0, 0, 1},  //3
  {0, 0, 1, 1, 0, 0, 1, 1},  //4
  {0, 1, 0, 1, 1, 0, 1, 1},  //5
  {0, 1, 0, 1, 1, 1, 1, 1},  //6
  {0, 1, 1, 1, 0, 0, 0, 0},  //7
  {0, 1, 1, 1, 1, 1, 1, 1},  //8
  {0, 1, 1, 1, 1, 0, 1, 1},  //9
  {0, 1, 1, 1, 0, 1, 1, 1},  //A
  {0, 0, 0, 1, 1, 1, 1, 1},  //b
  {0, 1, 0, 0, 1, 1, 1, 0},  //C
  {0, 0, 1, 1, 1, 1, 0, 1},  //d
  {0, 1, 0, 0, 1, 1, 1, 1},  //E
  {0, 1, 0, 0, 0, 1, 1, 1},  //F
  {1, 0, 0, 0, 0, 0, 0, 0},  //.
  {0, 0, 0, 0, 0, 0, 0, 0},  //null
};

byte digits[DIG_SIZE] = {0, 0, 0, 0};

volatile boolean invert = false;
volatile boolean mirror = false;
volatile boolean pause = false;
volatile boolean column = false;
volatile byte speed = 5;
volatile int inc_base = 1;
volatile int count = 0;

void setup()
{
  Serial.begin(9600);

  irrecv.enableIRIn(); // Start the receiver

  attachInterrupt(digitalPinToInterrupt(IR_RECV_PIN), recv_ir, CHANGE);

  set_pinmode(SEG_PIN, SEG_SIZE, OUTPUT);
  set_pinmode(COM_PIN, DIG_SIZE, OUTPUT);
}

void loop()
{  
  if (count > 9999) count = 0;
  if (count < 0) count = 9999;
  get_digits(count);
  all_digits_on(digits, speed, 2);
  if (! pause) count += inc_base;
}

void recv_ir()
{  
  if (irrecv.decode(&results)) {
    switch(results.value)
    {  
        case 0xFF40BF: // down
          invert = true;
          break;
        case 0xFF2AD5: // up
          invert = false;
          break;
        case 0xFF807F: // ++ count
          if (inc_base < 0) inc_base *= -1;
          break;
        case 0xFF30CF: // -- count
          if (inc_base > 0) inc_base *= -1;
          break;
        case 0xFF4AB5: // fast
          if (speed > 5) speed -= 5;
          break;
        case 0xFF8A75: // slow
          if (speed < 250) speed += 5;
          break;
        case 0xFF12ED: // pause
          pause = ! pause;
          break;
        case 0xFFC03F: // stop
          count = 0;
          inc_base = 1;
          speed = 5;
          pause = true;
          break;

        default: break;
    }   
    irrecv.resume(); // Receive the next value
  }
}

void set_pinmode(byte pin[], int size, int mode)
{
  for (int i = 0; i < size; ++i) {
    pinMode(pin[i], mode);
  }
}

void set_seg(byte pin_value[])
{
  for (int i = 0; i < SEG_SIZE; ++i)
  {
    digitalWrite(SEG_PIN[i], pin_value[i]);
  }
}

void set_com(byte n)
{  
  for (int i = 0; i < DIG_SIZE; ++i)
  {
    digitalWrite(COM_PIN[i], HIGH);
  }
  digitalWrite(COM_PIN[n-1], LOW);
}

void set_digit_on(byte com, byte n)
{  
    // clear: set as null
    set_seg(seg_pin_value[17]);
    // select digit: 1-4
    set_com(com);
    // pin value pre-wash: normal, inverted, mirrored
    byte value[SEG_SIZE];
    memcpy(value, seg_pin_value[n], sizeof(seg_pin_value[n][0])*SEG_SIZE);
    if (invert) invert_digit(value);
    if (mirror) mirror_digit(value);
    // set value to segment pins
    set_seg(value);
    // show column
    if (column) digitalWrite(SEG_PIN[0], 1);
}

void all_digits_on(byte digits[], int period, unsigned long refresh)
{
  // digit duration equal to period * refresh * DIG_SIZE
  // e.g 125 * 2 * 4 = 1s
  for(int i = 0; i < period; ++i)  
  {  
    for (int j = 0; j < DIG_SIZE; j++)
    {
      set_digit_on(j + 1, digits[j]);
      // feel blink when refresh > 10 ms, takes 2 as normal
      delay(refresh);
    }
  }
}

void invert_digit(byte pin_value[])
{
  for (int i = 1; i <= 3; ++i)
  {
    pin_value[i] = pin_value[i] ^ pin_value[i + 3];
    pin_value[i + 3] = pin_value[i + 3] ^ pin_value[i];
    pin_value[i] = pin_value[i] ^ pin_value[i + 3];
  }
}

void mirror_digit(byte pin_value[])
{
  int size = SEG_SIZE - 1;
  for (int i = 2; i <= 3; ++i)
  {
    int d = i * i;
    if (d > size) d -= size;
    pin_value[i] = pin_value[i] ^ pin_value[i + d];
    pin_value[i + d] = pin_value[i + d] ^ pin_value[i];
    pin_value[i] = pin_value[i] ^ pin_value[i + d];
  }
}

void get_digits(int num)
{
  int idx = DIG_SIZE;
  int inc = -1;
  if (invert || mirror)
  {
    idx = -1;
    inc = 1;
  }
  digits[idx += inc] = num % 10;
  digits[idx += inc] = num / 10 % 10;
  digits[idx += inc] = num / 100 % 10;
  digits[idx += inc] = num / 1000;
}