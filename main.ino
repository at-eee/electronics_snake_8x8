// Project made using 1588BS 8x8 LED matrix (red, common-anode).
/// Version with demultiplexer: Arudino Uno has very limiting pin amount 
/// as for let's say: 16 matrix display, hence the demultiplexer for the LED matrix rows was used.
/// I would preferably use demux with HIGH output state if I had it (like even CD4515BCN), because it would make things easier but I only had outputting
/// LOW (SN74LS13BN) so:  I used 3x8 demux outputting LOW state for rows (SN74LS13BN) + NOT gates (2x SN74HC04N) (Everything for steering only rows)

#include <stdlib.h>
#include <avr/wdt.h>

// 8 column pins:
#define PIN_A 6
#define PIN_B 7
#define PIN_C 8
#define PIN_D 9
#define PIN_E 10
#define PIN_F 11
#define PIN_G 12
#define PIN_H 13

// 3 row select pins:
// (3 demux input (output select) pins connected to arduino)
// (i.e: pins used to select one of the 8 columns (row pins) via demux outputs)
#define DEMUX_PIN_0 A0
#define DEMUX_PIN_1 A1
#define DEMUX_PIN_2 A2
// Where the DEMUX_PIN_0 is the least significant bit and DEMUX_PIN_2 is the most significant bit.

// Button pins:
#define LEFT_BUTTON 2
#define UP_BUTTON 3
#define RIGHT_BUTTON 4
#define DOWN_BUTTON 5

// Snake directions:
#define LEFT 1
#define UP 2
#define RIGHT 3
#define DOWN 4

// Number of rows and columns of our LED matrix display:
#define ROWS 8
#define COLS 8

#define MAX_SNAKE_LEN ROWS*COLS

//#define LETTER_INTRO_TIME 1000 // in ms, time for display of every letter in intro.
#define LETTER_SLIDE_TIME 75 // in ms, the time it takes for a letter to move by 1 cell.
#define MOVEMENT_WAIT_TIME 1000 // in ms, time game waits for player's input.
//#define // in ms, flicker rate time during game over screen

//!Experimental (as well any lines using it - the flickering of the given row is pretty annoying/misleading and shall be solved before)
//#define HEAD_BLINK_TIME 200
//#define APPLE_BLINK_TIME 800

// Snake data structure
struct snake{
  int8_t pos_x; // "row" index
  int8_t pos_y; // "column" index
  int8_t direction;
};

struct snake snake_segments[MAX_SNAKE_LEN]; // Array containing whole snake's body
int snake_len; // *Current* snake length
// Snake length is equal to the index of snake's tail as well
// Index zero (0) is equal to snake's head

// Stores states of all of the LEDs on the 8x8 matrix
byte cell_states[ROWS] = {0b00000000};
//(use these later:)
#define EMPTY_FIELD 0
#define SNAKE 1
#define TAIL  2
#define APPLE 3
//!!!Change defines to enums later?
//Making player unable to retrurn later?
bool game_over = false; // If equals to true, current game is over.
int8_t apple_pos_x = -1, apple_pos_y = -1;

// Letters:

const byte LETTER_S[8] = {0b01111110,
                          0b01000000,
                          0b01000000,
                          0b01111110,
                          0b00000010,
                          0b00000010,
                          0b00000010,
                          0b01111110};

const byte LETTER_N[8] = {0b10000001,
                          0b11000001,
                          0b10100001,
                          0b10010001,
                          0b10001001,
                          0b10000101,
                          0b10000011,
                          0b10000001};

const byte LETTER_A[8] = {0b00011000,
                          0b00100100,
                          0b00100100,
                          0b01000010,
                          0b01111110,
                          0b10000001,
                          0b10000001,
                          0b10000001};

const byte LETTER_K[8] = {0b01000100,
                          0b01001000,
                          0b01010000,
                          0b01100000,
                          0b01100000,
                          0b01010000,
                          0b01001000,
                          0b01000100};

const byte LETTER_E[8] = {0b01111110,
                          0b01000000,
                          0b01000000,
                          0b01111110,
                          0b01000000,
                          0b01000000,
                          0b01000000,
                          0b01111110};

/*
Pin layout of 1588BS LED 8x8 matrix screen:

A-H - column identifiers (Activated by negative current)
0-7 - row identifiers (Activated by positive current)

                        (counting from 1 from bottom left (from the side where the part number of the display is written))
                        |
                        v
LED matrix identifiers  physical (display) pins   Demux output pins:    pins on arduino:
0                       9                         0                     selected by demux (managed by arduino pins A0-A2)
1                       14                        1                     - || -
2                       8                         2                     - || -
3                       12                        3                     - || -
4                       1                         4                     - || -
5                       7                         5                     - || -
6                       2                         6                     - || -
7                       5                         7                     - || -
A                       13                        -                     6
B                       3                         -                     7
C                       4                         -                     8
D                       10                        -                     9
E                       6                         -                     10
F                       11                        -                     11
G                       15                        -                     12
H                       16                        -                     13

*/

// Sets the state of the singular ("cell" i.e:) LED on our LED matrix. Specifying the row, column (to pick the right LED) and state is required.
void set_state(int row, int col, byte state){
  
  if(state == APPLE || state == TAIL)
    state = 1;

  if(state)
    cell_states[row] |= (0b10000000 >> col);
  else
    cell_states[row] &= ~(0b10000000 >> col);

}

// Clears all the current column states.
void clear_cols(){ // (Doesn't reset (set to 0) the actual snake_fields, but sets the column states of the previous row to low (physical arduino
                  // pins) instead, to prevent them from influencing the displaying of the next rows).

  // Clears colums:
  for(int i = PIN_A; i <= PIN_H; i++){
    digitalWrite(i, HIGH); // HIGH means turned OFF for the columns (cathodes (HIGH is interfering with the current direction and/or the possible anode's (row's) HIGH)).
  }

} // Maybe rename to cleanup later or something?

// Resets all of the LED states (snake_fields) to 0.
void clear_states(){
  for(int i = 0; i < ROWS; i++)
    cell_states[i] = 0b00000000;
}

// Decodes passed value to a selected demux's output pin
void demux_row_decode(uint8_t pin){

  switch(pin){
      case 0:
        digitalWrite(DEMUX_PIN_0, LOW);
        digitalWrite(DEMUX_PIN_1, LOW);
        digitalWrite(DEMUX_PIN_2, LOW);
        break;
      case 1:
        digitalWrite(DEMUX_PIN_0, HIGH);
        digitalWrite(DEMUX_PIN_1, LOW);
        digitalWrite(DEMUX_PIN_2, LOW);
        break;
      case 2:
        digitalWrite(DEMUX_PIN_0, LOW);
        digitalWrite(DEMUX_PIN_1, HIGH);
        digitalWrite(DEMUX_PIN_2, LOW);
        break;
      case 3:
        digitalWrite(DEMUX_PIN_0, HIGH);
        digitalWrite(DEMUX_PIN_1, HIGH);
        digitalWrite(DEMUX_PIN_2, LOW);
        break;
      case 4:
        digitalWrite(DEMUX_PIN_0, LOW);
        digitalWrite(DEMUX_PIN_1, LOW);
        digitalWrite(DEMUX_PIN_2, HIGH);
        break;
      case 5:
        digitalWrite(DEMUX_PIN_0, HIGH);
        digitalWrite(DEMUX_PIN_1, LOW);
        digitalWrite(DEMUX_PIN_2, HIGH);
        break;
      case 6:
        digitalWrite(DEMUX_PIN_0, LOW);
        digitalWrite(DEMUX_PIN_1, HIGH);
        digitalWrite(DEMUX_PIN_2, HIGH);
        break;
      case 7:
        digitalWrite(DEMUX_PIN_0, HIGH);
        digitalWrite(DEMUX_PIN_1, HIGH);
        digitalWrite(DEMUX_PIN_2, HIGH);
        break;
    }

}

//unsigned long apple_blink_timing = millis(); // Stores the time from before movement_wait().
//unsigned long head_blink_timing = millis(); // Stores the time from before movement_wait().

// Think about negating display as a flag ?
int refresh_screen(){

  //byte temp_snake;
  //byte temp_apple;
  //unsigned long time_check = millis();

  /*if(time_check - head_blink_timing > HEAD_BLINK_TIME){
    temp_snake = snake_fields[snake_head->pos_x][snake_head->pos_y]; // temp variable to remember the apple's state.
    set_state(snake_head->pos_x, snake_head->pos_y, EMPTY_FIELD);
  }*/

  /*if(time_check - apple_blink_timing > APPLE_BLINK_TIME && !game_over){
    temp_apple = snake_fields[apple_location_x][apple_location_y]; // temp variable to remember the snake's head state.
    set_state(apple_location_x, apple_location_y, EMPTY_FIELD);
  }*/

  for(int i = 0; i < ROWS; i++){

    // Demux decoding actual row selected (to output)
    demux_row_decode(i);

    for(int j = 0; j < COLS; j++){
        digitalWrite( j + PIN_A, !(cell_states[i] & (0b10000000 >> j)) ); // We had to change cell states to negation before applying because for columns LOW = ON.
    }
    
    clear_cols();
  }

  /*if(time_check - head_blink_timing > HEAD_BLINK_TIME){
    set_state(snake_head->pos_x, snake_head->pos_y, temp_snake);
    snake_fields[snake_head->pos_x][snake_head->pos_y] = temp_snake;
    if(time_check - head_blink_timing > 2 * HEAD_BLINK_TIME){
      head_blink_timing = millis(); //if the time has passed (time reached x2 the value) reset the timer.
    }
  }*/
  /*if(time_check - apple_blink_timing > APPLE_BLINK_TIME && !game_over){
    set_state(apple_location_x, apple_location_y, temp_apple);
    snake_fields[apple_location_x][apple_location_y] = temp_apple;
    if(time_check - apple_blink_timing > 2 * APPLE_BLINK_TIME){
      apple_blink_timing = millis(); //if the time has passed (time reached x2 the value) reset the timer.
    }
  }*/

  return 0;

}

// Displays specified letter on 8x8 LED matrix (more precisely, sets the 8x8 cell matrix fields to specified values (LOW/HIGH states)).
void display_letter(char letter, int offset){

  switch(letter){
    case 'S':
      memcpy(cell_states, LETTER_S, sizeof(LETTER_S));
      break;
    case 'N':
      memcpy(cell_states, LETTER_N, sizeof(LETTER_N));
      break;
    case 'A':
      memcpy(cell_states, LETTER_A, sizeof(LETTER_A));
      break;
    case 'K':
      memcpy(cell_states, LETTER_K, sizeof(LETTER_K));
      break;
    case 'E':
      memcpy(cell_states, LETTER_E, sizeof(LETTER_E));
      break;
  }

  //Negative offset = to right, positive offset = to left.
  if(offset < 0){

    if(offset <= -COLS)
      offset = -COLS;

      offset *= -1;

    /*Serial.print("offset: "); // Debugging
    Serial.println(offset);*/

    for(int row = 0; row < ROWS; row++){
        cell_states[row] >>= offset;
        /*Serial.print(row); // Debugging
        Serial.print(": ");
        Serial.println(snake_fields[row], BIN);
        delay(1000);*/
    }

  }else{

    if(offset >= COLS)
      offset = COLS;

    for(int row = 0; row < ROWS; row++){
        cell_states[row] <<= offset;
    }

  }
}

void add_snake_segment(){

  snake_segments[snake_len] = snake_segments[snake_len-1]; //new tail

  switch(snake_segments[snake_len].direction){
    case LEFT:
      snake_segments[snake_len].pos_y += 1;
      if(snake_segments[snake_len].pos_y >= COLS)
        snake_segments[snake_len].pos_y = 0;
      break;
    case UP:
      snake_segments[snake_len].pos_x += 1;
      if(snake_segments[snake_len].pos_x >= ROWS)
        snake_segments[snake_len].pos_x = 0;
      break;
    case RIGHT:
      snake_segments[snake_len].pos_y -= 1;
      if(snake_segments[snake_len].pos_y < 0)
        snake_segments[snake_len].pos_y = 0;
      break;
    case DOWN:
      snake_segments[snake_len].pos_x -= 1;
      if(snake_segments[snake_len].pos_x < 0)
        snake_segments[snake_len].pos_x = 0;
      break;
  }

  snake_len++;

  return;
}

int move_snake(){

  //! Instead of moving all the snake segments, just make a new segment in the new direction of the head, and assign tail to it's "next" pointer then delete old tail.
  //  or do this statically just as (like) right now

  /*Serial.println("Snake segments (before):");
  for(int i = 0; i < snake_len+1; i++){
    Serial.print(i); Serial.println(": ");
    Serial.print("pos_x: "); Serial.println(snake_segments[i].pos_x);
    Serial.print("pos_y: "); Serial.println(snake_segments[i].pos_y);
    Serial.print("direction: "); Serial.println(snake_segments[i].direction);
  }
  //delay(10000);*/

  for(int i = snake_len; i > 0; i--){
    snake_segments[i] = snake_segments[i-1];
  }

  /*Serial.println("Snake segments (after):");
  for(int i = 0; i < snake_len+1; i++){
    Serial.print(i); Serial.println(": ");
    Serial.print("pos_x: "); Serial.println(snake_segments[i].pos_x);
    Serial.print("pos_y: "); Serial.println(snake_segments[i].pos_y);
    Serial.print("direction: "); Serial.println(snake_segments[i].direction);
  }
  //delay(10000);*/

  switch(snake_segments[0].direction){
    case LEFT:
      snake_segments[0].pos_y -= 1;
      if(snake_segments[0].pos_y < 0)
        snake_segments[0].pos_y = COLS-1;
      break;
    case UP:
      snake_segments[0].pos_x -= 1;
      if(snake_segments[0].pos_x < 0)
        snake_segments[0].pos_x = ROWS-1;
      break;
    case RIGHT:
      snake_segments[0].pos_y += 1;
      if(snake_segments[0].pos_y > COLS-1)
        snake_segments[0].pos_y = 0;
      break;
    case DOWN:
      snake_segments[0].pos_x += 1;
      if(snake_segments[0].pos_x > ROWS-1)
        snake_segments[0].pos_x = 0;
      break;
  }

  /*Serial.println("Snake segments (after 2):");
  for(int i = 0; i < snake_len+1; i++){
    Serial.print(i); Serial.println(": ");
    Serial.print("pos_x: "); Serial.println(snake_segments[i].pos_x);
    Serial.print("pos_y: "); Serial.println(snake_segments[i].pos_y);
    Serial.print("direction: "); Serial.println(snake_segments[i].direction);
  }
  //delay(10000);

  Serial.println("Info before \"if\":");
  //Serial.print("apple_pos_x: "); Serial.println(apple_pos_x);
  //Serial.print("apple_pos_y: "); Serial.println(apple_pos_y);
  Serial.print("cell_states[head.pos_x]: "); Serial.println(cell_states[snake_segments[0].pos_x], BIN);
  Serial.print("logic val: "); Serial.println(cell_states[snake_segments[0].pos_x] & (0b10000000 >> snake_segments[0].pos_y), BIN);*/
  //delay(2000);

  //more opitmal than checking every cell.

  if( cell_states[snake_segments[0].pos_x] & (0b10000000 >> snake_segments[0].pos_y) // If on the field we want to move our snake's head is a snake body/segment already, game over.
   && !(snake_segments[0].pos_x == apple_pos_x && snake_segments[0].pos_y == apple_pos_y)
   && !(snake_segments[0].pos_x == snake_segments[snake_len].pos_x && snake_segments[0].pos_y == snake_segments[snake_len].pos_y))
    return -1; // Game Over.

  snake_segments[snake_len].pos_x = -1;
  snake_segments[snake_len].pos_y = -1;
  snake_segments[snake_len].direction = -1;

  return 0;

}


void draw_snake(){

  for(int i = 0; i < snake_len; i++){
    set_state(snake_segments[i].pos_x, snake_segments[i].pos_y, SNAKE);
  }

  set_state(apple_pos_x, apple_pos_y, APPLE); //Marks apple location on snake's game fields
}

void apple_eat_logic(){ // (Apple + eating logic).

  // If snake's head hits the apple's cell.
  if(snake_segments[0].pos_x == apple_pos_x && snake_segments[0].pos_y == apple_pos_y){

    add_snake_segment(); // Extend snake's length by 1.

    do{
      apple_pos_x = random(ROWS); // Randomize value from 0 to ROWS-1
      apple_pos_y = random(COLS);
    }while(cell_states[apple_pos_x] & (0b10000000 >> apple_pos_y)); // is not empty (is non-zero).

  }

}

void game_logic(){

  // Performs snake movement logic.
  if(move_snake() == -1){ // If move_snake() returned -1 move was illegal or results in losing (hence game over).
    game_over = true;
    return;
  }

  apple_eat_logic();
  clear_states(); // Clears current states before marking.
  draw_snake(); // Marks cells on LED matrix to turn ON after the function ends.

  /*Serial.println("cell states:"); // Debugging
  for(int i = 0; i < ROWS; i++){
    Serial.println(cell_states[i], BIN);
  }
  Serial.println("Snake segments:");
  for(int i = 0; i < snake_len+1; i++){
    Serial.print(i); Serial.println(": ");
    Serial.print("pos_x: "); Serial.println(snake_segments[i].pos_x);
    Serial.print("pos_y: "); Serial.println(snake_segments[i].pos_y);
    Serial.print("direction: "); Serial.println(snake_segments[i].direction);
  }
  delay(15000);*/

}

//if 0 is returned, it means, no movement has been made.
int movement_wait(){

  refresh_screen();

  if(digitalRead(LEFT_BUTTON) == LOW){

    snake_segments[0].direction = LEFT;

  }else if(digitalRead(UP_BUTTON) == LOW){

    snake_segments[0].direction = UP;

  }else if(digitalRead(RIGHT_BUTTON) == LOW){

    snake_segments[0].direction = RIGHT;

  }else if(digitalRead(DOWN_BUTTON) == LOW){

    snake_segments[0].direction = DOWN;
    
  }else{
    return 0;
  }

  timed_event(200, refresh_screen); // Prevent accidental "double-press".
  return snake_segments[0].direction;

}

// Repeat this event until required time has passed.
int timed_event(int time_ms, int(*task)()){

  int ret_val = 0;
  unsigned long start_time, end_time;
  start_time = millis();
  do{
    ret_val = task();
    end_time = millis();
  }while(end_time - start_time < time_ms && !ret_val);
}

//Could technically make it by byte shifting...
void sliding_letters_intro(){

  const char intro_text[] = "SNAKE\0";
  int letter_idx = 0;

  while(intro_text[letter_idx] != '\0'){

    for(int offset = -COLS; offset < COLS; offset++){
      display_letter(intro_text[letter_idx], offset);
      timed_event(LETTER_SLIDE_TIME, refresh_screen);
    }

    letter_idx++;
  }
}

/*void intro_screen()

  display_letter('S');
  timed_event(LETTER_INTRO_TIME, refresh_screen);
  display_letter('N');
  timed_event(LETTER_INTRO_TIME, refresh_screen);
  display_letter('A');
  timed_event(LETTER_INTRO_TIME, refresh_screen);
  display_letter('K');
  timed_event(LETTER_INTRO_TIME, refresh_screen);
  display_letter('E');
  timed_event(LETTER_INTRO_TIME, refresh_screen);

  clear_states();
}*/

void setup() {

  // Columns:
  pinMode(PIN_A, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  pinMode(PIN_C, OUTPUT);
  pinMode(PIN_D, OUTPUT);
  pinMode(PIN_E, OUTPUT);
  pinMode(PIN_F, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_H, OUTPUT);

  // Rows (selected via demux):
  // (or:) Demux pins selecting rows:
  pinMode(DEMUX_PIN_0, OUTPUT);
  pinMode(DEMUX_PIN_1, OUTPUT);
  pinMode(DEMUX_PIN_2, OUTPUT);

  // Movement buttons:
  pinMode(LEFT_BUTTON, INPUT_PULLUP);
  pinMode(UP_BUTTON, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON, INPUT_PULLUP);
  pinMode(DOWN_BUTTON, INPUT_PULLUP);

  for(int i = 0; i < MAX_SNAKE_LEN; i++){
    snake_segments[i].pos_x = -1;
    snake_segments[i].pos_y = -1;
    snake_segments[i].direction = -1;
  }

  Serial.begin(9600); // Debug

  //intro_screen();
  sliding_letters_intro();

  clear_states();

  //setting up the beginning position of the snake.
  snake_segments[0].pos_x = ROWS/2 - 1;
  snake_segments[0].pos_y = COLS/2 - 1;
  snake_segments[0].direction = RIGHT;

  snake_len++;

  add_snake_segment();
  add_snake_segment();

  randomSeed(analogRead(A5)); // Randomizing the seed

  do{
    apple_pos_x = random(ROWS);
    apple_pos_y = random(COLS);
  }while(cell_states[apple_pos_x] & (0b10000000 >> apple_pos_y)); //is not empty (is non zero)

}

void reset_game(){

  wdt_enable(WDTO_4S); // Reset the device (after 3 seconds) i.e: start the new game again.
  while(1){ //make the player stuck on the refresh screen without being able to move
    timed_event(500, refresh_screen);
    delay(500); //flicker during game over rate
  }

}

void loop() {

  timed_event(MOVEMENT_WAIT_TIME, movement_wait);

  game_logic();

  if(game_over){
    reset_game();
  }

}
