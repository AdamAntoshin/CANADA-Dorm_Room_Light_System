//-------------LIBRARIES--------------

#include <Key.h>
#include <Keypad.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <FastLED.h>

//----------COMM PARAMETERS-----------

#define SERIAL_BAUD_RATE 9600

//----------PINOUT CONFIG-------------

#define KEY_R1 2
#define KEY_R2 3
#define KEY_R3 4
#define KEY_R4 5
#define KEY_C1 9
#define KEY_C2 8
#define KEY_C3 7
#define KEY_C4 6
#define DOOR_SENSE_PIN 10
#define LED_DATA_PIN 11
#define ERROR_PIN 13
#define BRIGHT_LEVEL_PIN A0

//-----------OTHER IO CONFIG----------

#define KEY_ROWS 4
#define KEY_COLS 4
#define LED_COUNT 300
#define LCD_LEN 16
#define LCD_HEIGHT 2
#define LCD_ADDRESS 0x27
#define MAX_INPUT_LEN 4

//-----------ANIMATIONS ID-------------

#define ANI_STAT_RED 0
#define ANI_STAT_GREEN 1
#define ANI_STAT_BLUE 2
#define ANI_STAT_YELLOW 3
#define ANI_STAT_MAGENTA 4
#define ANI_STAT_CYAN 5
#define ANI_STAT_WHITE 6
#define ANI_DIM_RED 7
#define ANI_DIM_GREEN 8
#define ANI_DIM_BLUE 9
#define ANI_DIM_YELLOW 10
#define ANI_DIM_MAGENTA 11
#define ANI_DIM_CYAN 12
#define ANI_DIM_WHITE 13
#define ANI_COLOR_WHEEL 14
#define ANI_SHOOTING_STARS 15
#define ANI_DOOR_OPEN 200
#define ANI_WAKE_UP 201
#define ANI_ERROR 202
#define ANI_OFF 203

//-----------ANIMATION PARAMETERS/CODES-----------

#define ANIMATION_MAX_NUM 15
#define RED 0
#define GREEN 1
#define BLUE 2
#define COLOR_WHEEL_OFFTOR 0
#define COLOR_WHEEL_RTOG 1
#define COLOR_WHEEL_GTOB 2
#define COLOR_WHEEL_BTOR 3
#define ANI_FRAME_LAT_DIM 1
#define ANI_FRAME_LAT_STARS 1
#define DOOR_TIMER 10
#define LED_TIME 3

//------------SYSTEM MODES-----------
#define DFLT 0
#define SETTING_ALARM 1
#define ALARM_SET 2
#define ARMING_DOOR 3
#define DOOR_ARMED 4

//------------KEYPAD CONFIG-----------

char keys[KEY_ROWS][KEY_COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
  };

uint8_t row_pins[KEY_ROWS] = {KEY_R1, KEY_R2, KEY_R3, KEY_R4};
uint8_t col_pins[KEY_COLS] = {KEY_C1, KEY_C2, KEY_C3, KEY_C4};

//-------------OBJECT DEFINITIONS-------------

Keypad keypad1 = Keypad(makeKeymap(keys), row_pins, col_pins, KEY_ROWS, KEY_COLS);
CRGB strip1[LED_COUNT];
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_LEN, LCD_HEIGHT);
RTC_DS1307 RTC;


//------------RGB COLORS (STORED IN FLASH)-----------

const uint8_t rgb_red[3] PROGMEM = {255, 0, 0};
const uint8_t rgb_green[3] PROGMEM = {0, 255, 0};
const uint8_t rgb_blue[3] PROGMEM = {0, 0, 255};
const uint8_t rgb_cyan[3] PROGMEM = {0, 255, 255};
const uint8_t rgb_magenta[3] PROGMEM = {255, 0, 255};
const uint8_t rgb_yellow[3] PROGMEM = {255, 255, 0};
const uint8_t rgb_white[3] PROGMEM = {175, 255, 255};


//-------------GLOBAL VARIABLES------------

uint8_t current_animation = ANI_STAT_RED, system_mode = DFLT, alarm_hour = 8, alarm_minute = 0, err = 0, changing_animation = 0;

uint32_t timer = 0, prev_timer_led = 0, prev_timer_door = 0, prev_timer_error = 0;


//----------------FUNCTION DEFINITIONS-------------

void init_IO();
int pow_int (int base, int exponent);
int string_to_int (char* str, int len);
void set_str_to_zeros(char* str, int len);
uint8_t get_brightness();
uint8_t get_red(uint32_t color);
uint8_t get_green(uint32_t color);
uint8_t get_blue(uint32_t color);
void handle_LED();
void update_strip(uint8_t brightness);
void ani_off();
void ani_stat_color(const uint8_t (&color)[3]);
void ani_dim_color(const uint8_t (&color)[3], uint8_t frame_count);
void ani_shooting_stars(const uint8_t (&color)[3], uint8_t len, uint8_t init_flag, uint8_t frame_count);
void ani_color_wheel_step(uint8_t init_flag);
void ani_wake_up(uint8_t frame_count);
void ani_door_open(uint8_t frame_count);
void raise_error();
void handle_error();
void handle_lcd(char* current_input);
void handle_door();
void handle_alarm();
void handle_input();

void setup() {
  init_IO();
  prev_timer_led = millis();
  prev_timer_door = millis();
}

void loop() {
  handle_input();
  handle_error();

  if (system_mode == DOOR_ARMED) {
    handle_door();
    }
  if (system_mode == ALARM_SET) {
    handle_alarm();
    }

  if (millis() - prev_timer_led >= LED_TIME) {
    prev_timer_led = millis();
    handle_LED();
    }
}

//Initialization process
void init_IO() {
  Serial.begin(SERIAL_BAUD_RATE);
  Wire.begin();
  RTC.begin();
  lcd.init();
  lcd.backlight();

  FastLED.addLeds<NEOPIXEL, LED_DATA_PIN>(strip1, LED_COUNT);

  
  if (! RTC.isrunning()) {
    Serial.println(F("RTC is NOT running!"));
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  
  pinMode(BRIGHT_LEVEL_PIN, INPUT);
  pinMode(ERROR_PIN, OUTPUT);
  pinMode(DOOR_SENSE_PIN, INPUT_PULLUP);
  }

//Return base^exponent (with ints instead of floats)
int pow_int (int base, int exponent) {
  int ret = 1;
  while (exponent > 0) {
    ret *= base;
    exponent--;
    }
  return ret;
  }

int string_to_int (char* str, int len) {
  int i, ret = 0;
  for (i = 0; i < len; i++) {
    int mult = pow_int(10, len - i - 1);
    int dig = (int)(*(str + i) - '0');
    ret += dig * mult;
    }
  return ret;
  }

//Change string to "0000..." (for clearing input)
void set_str_to_zeros(char* str, int len) {
  int i;

  for (i = 0; i < len; i++) {
    *(str + i) = '0';
    }
  }

//Get desired brightness from potentiometer
uint8_t get_brightness() {
  int brightness_unscaled = analogRead(BRIGHT_LEVEL_PIN);
  uint8_t brightness = map(brightness_unscaled, 0, 1023, 0, 255);

  return brightness;
  }

//Update LED colors in memory according to chosen animation and push update to strip
void handle_LED() {
  //Some animations need to know when they're called for the first time
  static uint8_t init1 = 1, init2 = 1;
  
  switch (current_animation) {
    case ANI_STAT_RED:
      ani_stat_color(rgb_red);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_STAT_GREEN:
      ani_stat_color(rgb_green);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_STAT_BLUE:
      ani_stat_color(rgb_blue);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_STAT_YELLOW:
      ani_stat_color(rgb_yellow);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_STAT_MAGENTA:
      ani_stat_color(rgb_magenta);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_STAT_CYAN:
      ani_stat_color(rgb_cyan);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_STAT_WHITE:
      ani_stat_color(rgb_white);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_DIM_RED:
      ani_dim_color(rgb_red, ANI_FRAME_LAT_DIM);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_DIM_GREEN:
      ani_dim_color(rgb_green, ANI_FRAME_LAT_DIM);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_DIM_BLUE:
      ani_dim_color(rgb_blue, ANI_FRAME_LAT_DIM);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_DIM_YELLOW:
      ani_dim_color(rgb_yellow, ANI_FRAME_LAT_DIM);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_DIM_MAGENTA:
      ani_dim_color(rgb_magenta, ANI_FRAME_LAT_DIM);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_DIM_CYAN:
      ani_dim_color(rgb_cyan, ANI_FRAME_LAT_DIM);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_DIM_WHITE:
      ani_dim_color(rgb_white, ANI_FRAME_LAT_DIM);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_COLOR_WHEEL:
      ani_color_wheel_step(init2);
      init1 = 1;
      init2 = 0;
      break;

    case ANI_SHOOTING_STARS:
      ani_shooting_stars(rgb_cyan, 15, init1, ANI_FRAME_LAT_STARS);
      init1 = 0;
      init2 = 1;
      break;

    case ANI_OFF:
      ani_off();
      init1 = 1;
      init2 = 1;
      break;

    case ANI_WAKE_UP:
      ani_wake_up(2);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_DOOR_OPEN:
      ani_door_open(1);
      init1 = 1;
      init2 = 1;
      break;
    
    default:
      ;
      
    }
  update_strip(get_brightness());
  }

void update_strip(uint8_t brightness) {

  FastLED.setBrightness(brightness);
  /*
  for (int i = 0; i < LED_COUNT; i++) {

    strip1.setPixelColor(i, matrix[i][0], matrix[i][1], matrix[i][2]);

  }
  */
  FastLED.show();

  //strip1.setBrightness(255);
}

/*
 *--------------------ANIMATIONS---------------------
 *
 * Functions update the RGB data in memory without pushing data
 * to strip. Dynamic animations update the data in "frames":
 * each call progresses the animation further depending on
 * current data and state.
 *
 *
 */
 
void ani_off() {
  for (int i = 0; i < LED_COUNT; i++) {
    strip1[i].setRGB(0, 0, 0);
  }
}

void ani_stat_color(const uint8_t (&color)[3]) {
  uint8_t red = pgm_read_byte(&color[RED]);
  uint8_t green = pgm_read_byte(&color[GREEN]);
  uint8_t blue = pgm_read_byte(&color[BLUE]);
  
  for (int i = 0; i < LED_COUNT; i++) {
    strip1[i].setRGB(red, green, blue);
  }
}

// Slowly brighten and dim the color of the strip
void ani_dim_color(const uint8_t (&color)[3], uint8_t frame_count) {
  static float strength = 1;
  static bool up = false; //Do the lights currently brighten or dim?
  static uint8_t frame = 1;

 
  
  //Update after a certain amount of calls
  if (frame >= frame_count) {
    frame = 1;

    uint8_t red = (uint8_t)((float)pgm_read_byte(&color[RED]) * strength);
    uint8_t green = (uint8_t)((float)pgm_read_byte(&color[GREEN]) * strength);
    uint8_t blue = (uint8_t)((float)pgm_read_byte(&color[BLUE]) * strength);
    
    for (int i = 0; i < LED_COUNT; i++) {
      strip1[i].setRGB(red, green, blue);
    }
    if (up) {
      strength += 0.01;
      if (strength >= 1) {
        strength = 1;
        up = false;
      }
    }
    else {
      strength -= 0.01;
      if (strength <= 0.3) {
        strength = 0.3;
        up = true;
      }
    }
  }
  else {
    frame++;
    }
}


void ani_shooting_stars(const uint8_t (&color)[3], uint8_t len, uint8_t init_flag, uint8_t frame_count) {
  static int counter = 0;
  int tail_pos = 0;
  static uint8_t frame = 1;

  // Generate shooting stars at initialization
  if (init_flag) {
    for (int i = LED_COUNT - 1; i >= 0; i--) {
      uint8_t red = pgm_read_byte(&color[RED]) / pow(2, tail_pos);
      uint8_t green = pgm_read_byte(&color[GREEN]) / pow(2, tail_pos);
      uint8_t blue = pgm_read_byte(&color[BLUE]) / pow(2, tail_pos);
      
      strip1[i].setRGB(red, green, blue);

      tail_pos++;
      if (tail_pos == len)
        tail_pos = 0;
    }

    init_flag = 0;
  }

  //Move the shooting stars further
  if (frame >= frame_count) {
    frame = 1;
    for (int i = LED_COUNT - 1; i > 0; i--) {
      strip1[i] = strip1[i - 1];
    }

    // Set the first pixel, depending on if it's part of a shooting star
    // tail or the head of a new shooting star
    if (counter != 0) {

      strip1[0].setRGB(strip1[0].r / 2, strip1[0].g / 2, strip1[0].b / 2);
  
      counter++;
      if (counter == len)
        counter = 0;
    }
  
    else {
      uint8_t red = pgm_read_byte(&color[RED]);
      uint8_t green = pgm_read_byte(&color[GREEN]);
      uint8_t blue = pgm_read_byte(&color[BLUE]);
      strip1[0].setRGB(red, green, blue);
      counter++;
    }
  }

  else {
    frame++;
    }
}

// Smoothly cycle throught different colors
void ani_color_wheel_step(uint8_t init_flag) {
  static int status = COLOR_WHEEL_OFFTOR;

  if (init_flag == 1) {
    for (int i = 0; i < LED_COUNT; i++) {
      strip1[i].setRGB(0, 0, 0);
      }
    init_flag = 0;
    }
  
  for (int i = 0; i < LED_COUNT; i++) {
    switch (status) {

      case COLOR_WHEEL_OFFTOR:
        strip1[i].setRGB(min(strip1[i].r + 2, 255), 0, 0);
        if (i == LED_COUNT - 1 && strip1[i].r + 2 >= 255)
          status = COLOR_WHEEL_RTOG;
        break;

      case COLOR_WHEEL_RTOG:
        strip1[i].setRGB(max(strip1[i].r - 2, 0), min(strip1[i].g + 2, 255), 0);

        if (i == LED_COUNT - 1 && strip1[i].g + 2 >= 255)
          status = COLOR_WHEEL_GTOB;
        break;

      case COLOR_WHEEL_GTOB:
        strip1[i].setRGB(0, max(strip1[i].g - 2, 0), min(strip1[i].b + 2, 255));
      
        if (i == LED_COUNT - 1 && strip1[i].b + 2 >= 255)
          status = COLOR_WHEEL_BTOR;
        break;

      case COLOR_WHEEL_BTOR:
        strip1[i].setRGB(min(strip1[i].r + 2, 255), 0, max(strip1[i].b - 2, 0));
      
        if (i == LED_COUNT - 1 && strip1[i].r + 2 >= 255)
          status = COLOR_WHEEL_RTOG;
        break;
    }
  }
}

// Alarm animation
void ani_wake_up(uint8_t frame_count) {
  static uint8_t frame = 1;
  int i;
  
  if (frame >= frame_count) {
    frame = 1;
    //Serial.println(get_red(strip1.getPixelColor(0)));
    for (i = 0; i < LED_COUNT; i++) {
      strip1[i].setRGB(min(strip1[i].r + 3, 255), min(strip1[i].g + 3, 255), 0);
      }
    //Serial.println(get_red(strip1.getPixelColor(0)));
    if (strip1[i].r >= 255) {
      current_animation = ANI_DIM_YELLOW;
      }
    //Serial.println(get_red(strip1.getPixelColor(0)));
    }
  else {
    frame++;
    }
  }

//Door open animation
void ani_door_open(uint8_t frame_count) {
  static uint8_t frame = 1, current_length = 0;
  if (frame >= frame_count) {
    frame = 1;

    strip1[current_length].setRGB(255, 0, 255);
    strip1[current_length + 1].setRGB(255, 0, 255);
    strip1[current_length + 2].setRGB(255, 0, 255);
    //strip1.setPixelColor(current_length + 3, 255, 0, 255);
    strip1[LED_COUNT - current_length - 1].setRGB(255, 0, 255);
    strip1[LED_COUNT - current_length - 2].setRGB(255, 0, 255);
    strip1[LED_COUNT - current_length - 3].setRGB(255, 0, 255);
    //strip1.setPixelColor(LED_COUNT - current_length - 4, 255, 0, 255);
    
    current_length += 3;
    //Serial.println(current_length);
    if (LED_COUNT - current_length - 3 <= current_length) {
      current_animation = ANI_DIM_MAGENTA;
      current_length = 0;
      }
    }
  else {
    frame++;
    }
  }

void raise_error() {
  err = 1;
  digitalWrite(ERROR_PIN, HIGH);
  prev_timer_error = millis();
  }

// Raise error for one second
void handle_error() {

  if (err == 1 && millis() - prev_timer_error > 1000) {
    digitalWrite(ERROR_PIN, LOW);
    err = 0;
    }
  }

// Print text to lcd displat depending on current state
void handle_lcd(char* current_input) {
  switch (system_mode) {
    case DFLT:
      //lcd.clear();
      lcd.backlight();
      lcd.setCursor(0, 0);
      lcd.print(F("Enter Animation:"));
      lcd.setCursor(0, 1);
      lcd.print(current_input);
      lcd.setCursor(11, 1);
      lcd.print(F("*-SET"));
      break;

    case SETTING_ALARM:
      //lcd.clear();
      lcd.backlight();
      lcd.setCursor(0, 0);
      lcd.print(F("Set Alarm To:"));
      lcd.setCursor(0, 1);
      lcd.print(current_input);
      lcd.setCursor(11, 1);
      lcd.print(F("*-SET"));
      break;

    case ALARM_SET:
    case DOOR_ARMED:
      lcd.noBacklight();
      //lcd.clear();
      set_str_to_zeros(current_input, MAX_INPUT_LEN);
      current_animation = ANI_OFF;
      break;

    case ARMING_DOOR:
      int door_time = (millis() - prev_timer_door) / 1000;
      char time_txt[20]; 
      sprintf(time_txt, "%2d seconds", DOOR_TIMER - door_time);
      //lcd.clear();
      lcd.backlight();
      lcd.setCursor(0, 0);
      lcd.print(F("Arming door"));
      lcd.setCursor(0, 1);
      lcd.print(time_txt);

      if (DOOR_TIMER - door_time <= 0) {
        system_mode = DOOR_ARMED;
        }
      break;
      }
      
      
    }

// When door armed, wait for door to open
void handle_door() {
  if (digitalRead(DOOR_SENSE_PIN) == HIGH) {
    current_animation = ANI_DOOR_OPEN;
    system_mode = DFLT;
    lcd.clear();
    }
  else {
    current_animation = ANI_OFF;
    }
  }

//When alarm set, wait for set time to arrive
void handle_alarm() {
  DateTime now = RTC.now(); 
  char hour_txt[4];
  char alarm_txt[4];
  sprintf(hour_txt, "%02d:%02d", now.hour(), now.minute());
  sprintf(alarm_txt, "%02d:%02d", alarm_hour, alarm_minute);
  lcd.setCursor(0, 0);
  lcd.print(F("Alrm:"));
  lcd.print(alarm_txt);
  lcd.print(F(" Time: "));
  lcd.setCursor(0, 1);
  lcd.print(hour_txt);
  lcd.setCursor(8, 1);
  lcd.print(F("#-CANCEL"));
  
  if (now.hour() == alarm_hour && now.minute() == alarm_minute) {
    ani_off();
    current_animation = ANI_WAKE_UP;
    system_mode = DFLT;
    lcd.clear();
    }

  }

// Handle input from keypad
void handle_input() {
  static char current_input[MAX_INPUT_LEN + 1] = "0000";
  int i;

  handle_lcd(current_input);
 
  char current_key = keypad1.getKey();
  if (current_key) {
    //Serial.println(current_key);

    //'#' cancels current operation
    if (current_key == '#' && (system_mode == ALARM_SET || system_mode == DOOR_ARMED)) {
      system_mode = DFLT;
      set_str_to_zeros(current_input, MAX_INPUT_LEN);
      lcd.clear();
      }

    //Key press that is not CANCEL lights up display for 3 seconds
    //when alarm is set
    else if (system_mode == ALARM_SET && current_key != '#') {
      lcd.backlight();
      delay(3000);
      lcd.noBacklight();
      }
      
    switch (current_key) {
      //When 'A' is pressed, set alarm
      case 'A':
        //Serial.println(F("Doing A"));
        if (system_mode != SETTING_ALARM) {
          system_mode = SETTING_ALARM;
          set_str_to_zeros(current_input, MAX_INPUT_LEN);
          lcd.clear();
          }
        break;

      //When 'D' is pressed, start arming door
      case 'D':
        Serial.println(F("Doing D"));
        system_mode = ARMING_DOOR;
        prev_timer_door = millis();
        lcd.clear();
        break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        //Serial.println(F("Doing num"));

        //Update numerical input
        for (i = 0; i < MAX_INPUT_LEN - 1; i++) {
          current_input[i] = current_input[i+1];
          }
        current_input[i] = current_key;
        break;

      //Cancel current operation
      case '#':
        //Serial.println("hi");
        system_mode = DFLT;
        lcd.clear();
        set_str_to_zeros(current_input, MAX_INPUT_LEN);
          
        break;

      //Set animation by default, set alarm if currently setting one
      case '*':
        //Serial.println(F("Doing *"));
        int input_int = string_to_int(current_input, MAX_INPUT_LEN);
        if (system_mode == DFLT) {
          //check if input is valid
          if (input_int <= ANIMATION_MAX_NUM && input_int >= 0) {
            if (current_animation != input_int) {
              //changing_animation = 1;
              }
            current_animation = input_int;
            
            set_str_to_zeros(current_input, MAX_INPUT_LEN);
          }
          else {
            set_str_to_zeros(current_input, MAX_INPUT_LEN);
            raise_error();
            }
          }
        else if (system_mode == SETTING_ALARM) {
          int input_minute = input_int % 100;
          int input_hour = input_int / 100;
          Serial.println(input_int);
          Serial.println(input_hour);
          Serial.println(input_minute);
          if (input_minute >= 0 && input_minute < 60 &&
              input_hour >= 0 && input_hour < 24) {
                alarm_hour = input_hour;
                alarm_minute = input_minute;
                system_mode = ALARM_SET;
                Serial.println(F("Alarm set!"));
                lcd.clear();
                set_str_to_zeros(current_input, MAX_INPUT_LEN);
                }
          else {
            system_mode = DFLT;
            lcd.clear();
            Serial.println(F("Alarm not set!"));
            raise_error();
            set_str_to_zeros(current_input, MAX_INPUT_LEN);
            }
          }
        break;
   
      }
    }
    
  }
