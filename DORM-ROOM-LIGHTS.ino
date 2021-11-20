#include <Key.h>
#include <Keypad.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <Adafruit_NeoPixel.h>

#define SERIAL_BAUD_RATE 9600

#define KEY_R1 2
#define KEY_R2 3
#define KEY_R3 4
#define KEY_R4 5
#define KEY_C1 6
#define KEY_C2 7
#define KEY_C3 8
#define KEY_C4 9
#define DOOR_SENSE_PIN 10
#define LED_DATA_PIN 11
#define ERROR_PIN 13
#define BRIGHT_LEVEL_PIN A0

#define KEY_ROWS 4
#define KEY_COLS 4
#define LED_COUNT 300
#define LCD_LEN 16
#define LCD_HEIGHT 2
#define LCD_ADDRESS 0x27

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

#define ANIMATION_MAX_NUM 12
#define RED 0
#define GREEN 1
#define BLUE 2

#define DFLT 0
#define SETTING_ALARM 1
#define ALARM_SET 2
#define ARMING_DOOR 3
#define DOOR_ARMED 4

#define COLOR_WHEEL_OFFTOR 0
#define COLOR_WHEEL_RTOG 1
#define COLOR_WHEEL_GTOB 2
#define COLOR_WHEEL_BTOR 3

#define MAX_INPUT_LEN 4

#define DOOR_TIMER 10
#define LED_TIME 10

#define ANI_FRAME_LAT_DIM 4
#define ANI_FRAME_LAT_STARS 7

char keys[KEY_ROWS][KEY_COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
  };

uint8_t row_pins[KEY_ROWS] = {KEY_R1, KEY_R2, KEY_R3, KEY_R4};
uint8_t col_pins[KEY_COLS] = {KEY_C1, KEY_C2, KEY_C3, KEY_C4};

Keypad keypad1 = Keypad(makeKeymap(keys), row_pins, col_pins, KEY_ROWS, KEY_COLS);
Adafruit_NeoPixel strip1(LED_COUNT, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_LEN, LCD_HEIGHT);
RTC_DS1307 RTC;

uint8_t matrix[LED_COUNT][3] = {0};
uint8_t rgb_red[3] = {255, 0, 0};
uint8_t rgb_green[3] = {0, 255, 0};
uint8_t rgb_blue[3] = {0, 0, 255};
uint8_t rgb_cyan[3] = {0, 255, 255};
uint8_t rgb_magenta[3] = {255, 0, 255};
uint8_t rgb_yellow[3] = {255, 255, 0};
uint8_t rgb_white[3] = {175, 255, 255};


uint8_t current_animation = ANI_STAT_RED, system_mode = DFLT, alarm_hour = 8, alarm_minute = 0, err = 0, changing_animation = 0;

uint64_t timer = 0, prev_timer_led = 0, prev_timer_door = 0, prev_timer_error = 0;

void init_IO() {
  Serial.begin(SERIAL_BAUD_RATE);
  Wire.begin();
  RTC.begin();
  lcd.begin();
  lcd.backlight();
  strip1.begin();
  strip1.clear();
  strip1.show();

  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  
  pinMode(BRIGHT_LEVEL_PIN, INPUT);
  pinMode(ERROR_PIN, OUTPUT);
  pinMode(DOOR_SENSE_PIN, INPUT_PULLUP);
  }

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
    Serial.print(mult);
    Serial.print("*");
    Serial.print(dig);
    Serial.print("=");
    int test = dig * mult;
    Serial.print(test);
    Serial.println("");
    }
  return ret;
  }

void set_str_to_zeros(char* str, int len) {
  int i;

  for (i = 0; i < len; i++) {
    *(str + i) = '0';
    }
  }

uint8_t get_brightness() {
  int brightness_unscaled = analogRead(BRIGHT_LEVEL_PIN);
  uint8_t brightness = map(brightness_unscaled, 0, 1023, 0, 255);

  return brightness;
  }

void handle_LED() {
  //strip1.setBrightness(get_brightness());
  static uint8_t init1 = 1, init2 = 1;
  
  switch (current_animation) {
    case ANI_STAT_RED:
      ani_stat_color(matrix, rgb_red);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_STAT_GREEN:
      ani_stat_color(matrix, rgb_green);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_STAT_BLUE:
      ani_stat_color(matrix, rgb_blue);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_STAT_YELLOW:
      ani_stat_color(matrix, rgb_yellow);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_STAT_MAGENTA:
      ani_stat_color(matrix, rgb_magenta);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_STAT_CYAN:
      ani_stat_color(matrix, rgb_cyan);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_STAT_WHITE:
      ani_stat_color(matrix, rgb_white);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_DIM_RED:
      ani_dim_color(matrix, rgb_red, ANI_FRAME_LAT_DIM);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_DIM_GREEN:
      ani_dim_color(matrix, rgb_green, ANI_FRAME_LAT_DIM);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_DIM_BLUE:
      ani_dim_color(matrix, rgb_blue, ANI_FRAME_LAT_DIM);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_DIM_YELLOW:
      ani_dim_color(matrix, rgb_yellow, ANI_FRAME_LAT_DIM);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_DIM_MAGENTA:
      ani_dim_color(matrix, rgb_magenta, ANI_FRAME_LAT_DIM);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_DIM_CYAN:
      ani_dim_color(matrix, rgb_cyan, ANI_FRAME_LAT_DIM);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_DIM_WHITE:
      ani_dim_color(matrix, rgb_white, ANI_FRAME_LAT_DIM);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_COLOR_WHEEL:
      ani_color_wheel_step(matrix, init2);
      init1 = 1;
      init2 = 0;
      break;

    case ANI_SHOOTING_STARS:
      ani_shooting_stars(matrix, rgb_cyan, 15, init1, ANI_FRAME_LAT_STARS);
      init1 = 0;
      init2 = 1;
      break;

    case ANI_OFF:
      ani_off(matrix);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_WAKE_UP:
      ani_wake_up(matrix, 5);
      init1 = 1;
      init2 = 1;
      break;

    case ANI_DOOR_OPEN:
      ani_door_open(matrix, 5);
      init1 = 1;
      init2 = 1;
      break;
    
    default:
      ;
      
    }
  update_strip(matrix, get_brightness());
  }

void update_strip(uint8_t matrix[LED_COUNT][3], uint8_t brightness) {

  strip1.setBrightness(brightness);

  for (int i = 0; i < LED_COUNT; i++) {

    strip1.setPixelColor(i, matrix[i][0], matrix[i][1], matrix[i][2]);

  }

  strip1.show();

}

void ani_off(uint8_t matrix[LED_COUNT][3]) {
  for (int i = 0; i < LED_COUNT; i++) {
    matrix[i][RED] = 0;
    matrix[i][GREEN] = 0;
    matrix[i][BLUE] = 0;
  }
}

void ani_stat_color(uint8_t matrix[LED_COUNT][3], uint8_t color[3]) {
  for (int i = 0; i < LED_COUNT; i++) {
    matrix[i][RED] = color[RED];
    matrix[i][GREEN] = color[GREEN];
    matrix[i][BLUE] = color[BLUE];
  }
}

void ani_dim_color(uint8_t matrix[LED_COUNT][3], uint8_t color[3], uint8_t frame_count) {
  static float strength = 1;
  static bool up = false;
  static uint8_t frame = 1;

  for (int i = 0; i < LED_COUNT; i++) {
    matrix[i][RED] = (int)((float)color[RED] * strength);
    matrix[i][GREEN] = (int)((float)color[GREEN] * strength);
    matrix[i][BLUE] = (int)((float)color[BLUE] * strength);
  }
  if (frame >= frame_count) {
    frame = 1;
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

void ani_shooting_stars(uint8_t matrix[LED_COUNT][3], uint8_t color[3], uint8_t len, uint8_t init_flag, uint8_t frame_count) {
  static int counter = 0;
  int tail_pos = 0;
  static uint8_t frame = 1;
  
  if (init_flag) {
    for (int i = LED_COUNT - 1; i >= 0; i--) {
      matrix[i][RED] = color[RED] / pow(2, tail_pos);
      matrix[i][GREEN] = color[GREEN] / pow(2, tail_pos);
      matrix[i][BLUE] = color[BLUE] / pow(2, tail_pos);

      tail_pos++;
      if (tail_pos == len)
        tail_pos = 0;
    }

    init_flag = 0;
  }

  if (frame >= frame_count) {
    frame = 1;
    for (int i = LED_COUNT - 1; i > 0; i--) {
      matrix[i][RED] = matrix[i - 1][RED];
      matrix[i][GREEN] = matrix[i - 1][GREEN];
      matrix[i][BLUE] = matrix[i - 1][BLUE];
    }
  
    if (counter != 0) {
      matrix[0][RED] = matrix[0][RED] / 2;
      matrix[0][GREEN] = matrix[0][GREEN] / 2;
      matrix[0][BLUE] = matrix[0][BLUE] / 2;
  
      counter++;
      if (counter == len)
        counter = 0;
    }
  
    else {
      matrix[0][RED] = color[RED];
      matrix[0][GREEN] = color[GREEN];
      matrix[0][BLUE] = color[BLUE];
      counter++;
    }
  }

  else {
    frame++;
    }
}

void ani_color_wheel_step(uint8_t matrix[LED_COUNT][3], uint8_t init_flag) {
  static int status = COLOR_WHEEL_OFFTOR;

  if (init_flag == 1) {
    for (int i = 0; i < LED_COUNT; i++) {
      matrix[i][GREEN] = 0;
      matrix[i][BLUE] = 0;
      matrix[i][RED] = 0;
      }
    init_flag = 0;
    }
  
  for (int i = 0; i < LED_COUNT; i++) {
    switch (status) {

      case COLOR_WHEEL_OFFTOR:
        matrix[i][GREEN] = 0;
        matrix[i][BLUE] = 0;
        matrix[i][RED]++;
        if (i == LED_COUNT - 1 && matrix[i][RED] == 255)
          status = COLOR_WHEEL_RTOG;
        break;

      case COLOR_WHEEL_RTOG:
        matrix[i][RED]--;
        matrix[i][GREEN]++;
        if (i == LED_COUNT - 1 && matrix[i][GREEN] == 255)
          status = COLOR_WHEEL_GTOB;
        break;

      case COLOR_WHEEL_GTOB:
        matrix[i][GREEN]--;
        matrix[i][BLUE]++;
        if (i == LED_COUNT - 1 && matrix[i][BLUE] == 255)
          status = COLOR_WHEEL_BTOR;
        break;

      case COLOR_WHEEL_BTOR:
        matrix[i][BLUE]--;
        matrix[i][RED]++;
        if (i == LED_COUNT - 1 && matrix[i][RED] == 255)
          status = COLOR_WHEEL_RTOG;
        break;
    }
  }
}

void ani_wake_up(uint8_t matrix[LED_COUNT][3], uint8_t frame_count) {
  static uint8_t frame = 1;
  int i;
  if (frame >= frame_count) {
    frame = 1;

    for (i = 0; i < LED_COUNT; i++) {
      matrix[i][RED]++;
      matrix[i][GREEN]++;
      }
    if (matrix[0][RED] == 255) {
      current_animation = ANI_DIM_YELLOW;
      }
    }
  else {
    frame++;
    }
  }

void ani_door_open(uint8_t matrix[LED_COUNT][3], uint8_t frame_count) {
  static uint8_t frame = 1, current_length = 0;
  if (frame >= frame_count) {
    frame = 1;

    matrix[current_length][RED] = 255;
    matrix[current_length][GREEN] = 0;
    matrix[current_length][BLUE] = 255;

    matrix[LED_COUNT - current_length - 1][RED] = 255;
    matrix[LED_COUNT - current_length - 1][GREEN] = 0;
    matrix[LED_COUNT - current_length - 1][BLUE] = 255;
    
    current_length++;

    if (LED_COUNT - current_length - 1 >= current_length) {
      current_animation = ANI_DIM_MAGENTA;
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

void handle_error() {

  if (err == 1 && millis() - prev_timer_error > 1000) {
    digitalWrite(ERROR_PIN, LOW);
    err = 0;
    }
  }

void handle_lcd(char* current_input) {
  switch (system_mode) {
    case DFLT:
      //lcd.clear();
      lcd.backlight();
      lcd.setCursor(0, 0);
      lcd.print("Enter Animation:");
      lcd.setCursor(0, 1);
      lcd.print(current_input);
      lcd.setCursor(11, 1);
      lcd.print("*-SET");
      break;

    case SETTING_ALARM:
      //lcd.clear();
      lcd.backlight();
      lcd.setCursor(0, 0);
      lcd.print("Set Alarm To:");
      lcd.setCursor(0, 1);
      lcd.print(current_input);
      lcd.setCursor(11, 1);
      lcd.print("*-SET");
      break;

    case ALARM_SET:
    case DOOR_ARMED:
      lcd.noBacklight();
      //lcd.clear();
      set_str_to_zeros(current_input, MAX_INPUT_LEN);
      break;

    case ARMING_DOOR:
      int door_time = (millis() - prev_timer_door) / 1000;
      char time_txt[20]; 
      sprintf(time_txt, "%2d seconds", DOOR_TIMER - door_time);
      //lcd.clear();
      lcd.backlight();
      lcd.setCursor(0, 0);
      lcd.print("Arming door");
      lcd.setCursor(0, 1);
      lcd.print(time_txt);

      if (DOOR_TIMER - door_time <= 0) {
        system_mode = DOOR_ARMED;
        }
      break;
      }
      
      
    }
  

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

void handle_alarm() {
  DateTime now = RTC.now(); 
  char hour_txt[4];
  char alarm_txt[4];
  sprintf(hour_txt, "%02d:%02d", now.hour(), now.minute());
  sprintf(alarm_txt, "%02d:%02d", alarm_hour, alarm_minute);
  lcd.setCursor(0, 0);
  lcd.print("Alrm:");
  lcd.print(alarm_txt);
  lcd.print(" Time: ");
  lcd.setCursor(0, 1);
  lcd.print(hour_txt);
  lcd.setCursor(8, 1);
  lcd.print("#-CANCEL");
  
  if (now.hour() == alarm_hour && now.minute() == alarm_minute) {
    current_animation = ANI_WAKE_UP;
    system_mode = DFLT;
    lcd.clear();
    }

  }

void handle_input() {
  static char current_input[MAX_INPUT_LEN + 1] = "0000";
  int i;

  handle_lcd(current_input);
 
  char current_key = keypad1.getKey();
  if (current_key) {
    Serial.println(current_key);
    if (current_key == '#' && (system_mode == ALARM_SET || system_mode == DOOR_ARMED)) {
      system_mode = DFLT;
      set_str_to_zeros(current_input, MAX_INPUT_LEN);
      lcd.clear();
      }
    else if (system_mode == ALARM_SET && current_key != '#') {
      lcd.backlight();
      delay(3000);
      lcd.noBacklight();
      }
      
    switch (current_key) {
      case 'A':
        Serial.println("Doing A");
        if (system_mode != SETTING_ALARM) {
          system_mode = SETTING_ALARM;
          set_str_to_zeros(current_input, MAX_INPUT_LEN);
          lcd.clear();
          }
        break;
      case 'D':
        Serial.println("Doing D");
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
        Serial.println("Doing num");
        for (i = 0; i < MAX_INPUT_LEN - 1; i++) {
          current_input[i] = current_input[i+1];
          }
        current_input[i] = current_key;
        break;
        
      case '#':
        Serial.println("hi");
        system_mode = DFLT;
        lcd.clear();
        set_str_to_zeros(current_input, MAX_INPUT_LEN);
          
        break;
        
      case '*':
        Serial.println("Doing *");
        int input_int = string_to_int(current_input, MAX_INPUT_LEN);
        if (system_mode == DFLT) {
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
                Serial.println("Alarm set!");
                lcd.clear();
                }
          else {
            system_mode = DFLT;
            lcd.clear();
            Serial.println("Alarm not set!");
            raise_error();
            set_str_to_zeros(current_input, MAX_INPUT_LEN);
            }
          }
        break;
   
      }
    }
    
  }

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
