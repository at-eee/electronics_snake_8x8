// Project made using 1588BS 8x8 LED matrix (red, common-anode).
/// Version with demultiplexer (Arudino Uno has very limiting pin amount 
/// as for let's say: 16 matrix display, hence the demultiplexer (for rows??) was used.)
/// x I didn't have 4x16 demux outputting HIGH state, so I had to use demux outputting low state (CD4515BCN) + the NOT gates (3x SN74HC04N)
/// x I didn't have 3x8 demux outputting HIGH state, so I had to use demux outputting low state (CD4515BCN) + the NOT gates (3x SN74HC04N)
/// I would preferably use demux with HIGH output state (like even CD4515BCN), because it would make things easier but I only had outputting LOW (SN74LS13BN) so:
/// I used 3x8 demux outputting LOW state for rows (SN74LS13BN) + NOT gates (2x SN74HC04N)

#include <stdlib.h>

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
#define LEFT 1 //(0 before) Fixed.
#define UP 2
#define RIGHT 3
#define DOWN 4

// Number of rows and columns of our LED matrix display:
#define ROWS 8
#define COLS 8

#define LETTER_INTRO_TIME 1000 //in ms, time for display of every letter in intro.

// Snake data structure
struct snake{
  // Previous snake's segment
  struct snake* prev;
  struct snake* next;
  int pos_x; // "row" index
  int pos_y; // "column" index
  byte direction;
};
//this might be unoptimal for game logic
//only tail instead?

// Stores states of all of the LEDs on the 8x8 matrix
//byte cell_states[ROWS][COLS] = {0};

// 
int padding[200] = {0}; //somehow, some amount of my microcontroller's memory was corrupted, so i had to use this
byte snake_fields[ROWS][COLS] = {0}; // 0 is empty field, 1 is snake, and 2 is apple.
//(use these later:)
#define EMPTY_FIELD 0
#define SNAKE 1
#define APPLE 2
//byte snake_direction;
// Position of the snake's head:
struct snake head = {.prev = nullptr, .next = nullptr, .pos_x = -1, .pos_y = -1, .direction = -1};
struct snake* snake_head = &head;
struct snake* tail = &head;
//int tail_pos_x, tail_pos_y;

// Maybe COL_OFF, COL_ON, ROW_OFF and ROW_ON macros later?

// Letters:

const byte LETTER_S[8][8] = {{0, 1, 1, 1, 1, 1, 1, 0},
                             {0, 1, 0, 0, 0, 0, 0, 0},
                             {0, 1, 0, 0, 0, 0, 0, 0},
                             {0, 1, 1, 1, 1, 1, 1, 0},
                             {0, 0, 0, 0, 0, 0, 1, 0},
                             {0, 0, 0, 0, 0, 0, 1, 0},
                             {0, 0, 0, 0, 0, 0, 1, 0},
                             {0, 1, 1, 1, 1, 1, 1, 0}};

const byte LETTER_N[8][8] = {{1, 0, 0, 0, 0, 0, 0, 1},
                             {1, 1, 0, 0, 0, 0, 0, 1},
                             {1, 0, 1, 0, 0, 0, 0, 1},
                             {1, 0, 0, 1, 0, 0, 0, 1},
                             {1, 0, 0, 0, 1, 0, 0, 1},
                             {1, 0, 0, 0, 0, 1, 0, 1},
                             {1, 0, 0, 0, 0, 0, 1, 1},
                             {1, 0, 0, 0, 0, 0, 0, 1},};

const byte LETTER_A[8][8] = {{0, 0, 0, 1, 1, 0, 0, 0},
                             {0, 0, 1, 0, 0, 1, 0, 0},
                             {0, 0, 1, 0, 0, 1, 0, 0},
                             {0, 1, 0, 0, 0, 0, 1, 0},
                             {0, 1, 1, 1, 1, 1, 1, 0},
                             {1, 0, 0, 0, 0, 0, 0, 1},
                             {1, 0, 0, 0, 0, 0, 0, 1},
                             {1, 0, 0, 0, 0, 0, 0, 1},};

const byte LETTER_K[8][8] = {{0, 1, 0, 0, 0, 1, 0, 0},
                             {0, 1, 0, 0, 1, 0, 0, 0},
                             {0, 1, 0, 1, 0, 0, 0, 0},
                             {0, 1, 1, 0, 0, 0, 0, 0},
                             {0, 1, 1, 0, 0, 0, 0, 0},
                             {0, 1, 0, 1, 0, 0, 0, 0},
                             {0, 1, 0, 0, 1, 0, 0, 0},
                             {0, 1, 0, 0, 0, 1, 0, 0},};

const byte LETTER_E[8][8] = {{0, 1, 1, 1, 1, 1, 1, 0},
                             {0, 1, 0, 0, 0, 0, 0, 0},
                             {0, 1, 0, 0, 0, 0, 0, 0},
                             {0, 1, 1, 1, 1, 1, 1, 0},
                             {0, 1, 0, 0, 0, 0, 0, 0},
                             {0, 1, 0, 0, 0, 0, 0, 0},
                             {0, 1, 0, 0, 0, 0, 0, 0},
                             {0, 1, 1, 1, 1, 1, 1, 0},};

/*
Pin layout of 1588BS LED 8x8 matrix screen:

A-H - column identifiers (Activated by negative current)
0-7 - row identifiers (Activated by positive current)

                        (counting from 1 from bottom left (from the side where the part number of the display is written))
                        |
                        v
LED matrix identifiers  physical (display) pins   Demux output pins:    pins on arduino:
0                       9                         0                     selected by demux (managed by arduino pins 2-4)
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
  snake_fields[row][col] = state;
}

/*void hide_cell(int row, int col){
  cell_states[row][column] = LOW;
}*/
//pros and cons?

// Clears all the current column states.
void clear_row(){ // (Doesn't reset (set to 0) the actual snake_fields, but sets the column states of the previous row to low (physical arduino
                  // pins) instead, to prevent them from influencing the displaying of the next rows).

  // Clears colums:
  for(int i = PIN_A; i <= PIN_H; i++){
    digitalWrite(i, HIGH); // HIGH means turned OFF for the columns (cathodes (HIGH is interfering with the current direction and/or the possible anode's (row's) HIGH)).
  }

} // Maybe rename to cleanup later or something?

// Resets all of the LED states (snake_fields) to 0.
void clear_states(){
  for(int i = 0; i < ROWS; i++)
    for(int j = 0; j < COLS; j++){
      set_state(i, j, 0);
    }
}

// Decodes passed value to a selected demux output pin
void demux_row_decode(int pin){

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

// Think about negating display as a flag ?
int refresh_screen(){

  for(int i = 0; i < ROWS; i++){

    // Demux decoding actual row selected (to output)
    demux_row_decode(i);

    for(int j = 0; j < COLS; j++){
        digitalWrite(j + PIN_A, !snake_fields[i][j]); // We had to change cell states to negation before applying because for columns LOW = ON.
    }
    
    delay(1); // Delay in ms
    // Making sure we turn off this row after. - we no longer need to make sure (demultiplexer)
    // digitalWrite(i + PIN_0, LOW);
    clear_row();

  }

  return 0;

}

// Displays specified letter on 8x8 LED matrix (more precisely, sets the 8x8 cell matrix fields to specified values (LOW/HIGH states)).
void display_letter(char letter){

  switch(letter){
    case 'S':
      memcpy(snake_fields, LETTER_S, sizeof(LETTER_S));
      break;
    case 'N':
      memcpy(snake_fields, LETTER_N, sizeof(LETTER_N));
      break;
    case 'A':
      memcpy(snake_fields, LETTER_A, sizeof(LETTER_A));
      break;
    case 'K':
      memcpy(snake_fields, LETTER_K, sizeof(LETTER_K));
      break;
    case 'E':
      memcpy(snake_fields, LETTER_E, sizeof(LETTER_E));
      break;
  }

}

void add_snake_segment(){

  struct snake* new_node = malloc(sizeof(struct snake));
  if(snake_head->prev == nullptr){ // If snake is of length 1 (! think about shortening this)

    tail = new_node;
    tail->next = snake_head;
    tail->prev = nullptr;
    snake_head->prev = tail;
    tail->pos_x = snake_head->pos_x;
    tail->pos_y = snake_head->pos_y;

    switch(snake_head->direction){
      case LEFT:
        tail->pos_y += 1; // *(-1) because it will be spawn/added 1 pixel to the opposite direction.
        break;
      case UP:
        tail->pos_x += 1;
        break;
      case RIGHT:
        tail->pos_y -= 1;
        break;
      case DOWN:
        tail->pos_x -= 1;
        break;
    }

    tail->direction = tail->next->direction;

    return;
  }

  // in case snake is of length 2+.
  new_node->next = tail;
  new_node->prev = nullptr;
  tail->prev = new_node;
  tail = new_node;
  tail->pos_x = tail->next->pos_x;
  tail->pos_y = tail->next->pos_y;

  switch(tail->next->direction){
    case LEFT:
      tail->pos_y += 1; // *(-1) because it will be spawn/added 1 pixel to the opposite direction.
      break;
    case UP:
      tail->pos_x += 1;
      break;
    case RIGHT:
      tail->pos_y -= 1;
      break;
    case DOWN:
      tail->pos_x -= 1;
      break;
  }

  tail->direction = tail->next->direction;
  return;

  //Przesledzic/przewertować/skim through it jezcze
}

void move_snake(){

  // Head (snake's front logic)
  struct snake* new_head = malloc(sizeof(struct snake)); //0x2
  snake_head->next = new_head; //0x2
  new_head->pos_x = snake_head->pos_x;
  new_head->pos_y = snake_head->pos_y;

  switch(snake_head->direction){
    case LEFT:
      new_head->pos_y -= 1;
      break;
    case UP:
      new_head->pos_x -= 1;
      break;
    case RIGHT:
      new_head->pos_y += 1;
      break;
    case DOWN:
      new_head->pos_x += 1;
      break;
  }
  //!!! Checks for borders/bounds of matrix!!!

  new_head->direction = snake_head->direction;
  new_head->prev = snake_head; //0x1
  new_head->next = nullptr;
  snake_head = new_head; //0x2
  //^variable storing 0x1, stores 0x2 now.

  // Tail (snake's back logic)
  if(snake_head->prev->prev == nullptr){ // If snake was of length 1, discard node/segment behind the snake's head.
    free(snake_head->prev);
    snake_head->prev = nullptr; // In order to avoid dangling pointer.
    tail = snake_head; // setting tail to head (because snake is of length 1)/
    return;
  }else if(snake_head->prev->prev == tail){ // If snake is of length 2 (later think if that's the same as for length n).
    tail = snake_head->prev;
    free(snake_head->prev->prev);
    tail->prev = nullptr; // (same as: snake_head->prev->prev = nullptr?)
    return;
  }
  
  tail = tail->next;
  free(tail->prev);
  tail->prev = nullptr; // In order to avoid dangling pointer.

}

void draw_snake(){

  //snake_fields[snake_head->pos_x][snake_head->pos_y] = 1;
  set_state(snake_head->pos_x, snake_head->pos_y, 1);
  struct snake* previous = snake_head->prev;
  while(previous != nullptr){
    //snake_fields[previous->pos_x][previous->pos_y] = 1;
    set_state(previous->pos_x, previous->pos_y, 1);
    previous = previous->prev;
  }
}

void game_logic(){

  //this logic will change later, but removing the old snake's head "location".
  //snake_fields[snake_head->pos_x][snake_head->pos_y] = 0;
  clear_states();

  //! Instead of moving all the snake segments, just make a new segment in the new direction of the head, and assign tail to it's "next" pointer then delete old tail.

  move_snake(); // Performs snake movement logic.
  draw_snake(); // Marks LED matrix cells to turn on.
  //snake_fields[snake_head->pos_x][snake_head->pos_y] = 1;

}

//if 0 is returned, it means, no movement has been made.
int movement_wait(){

  refresh_screen();

  if(digitalRead(LEFT_BUTTON) == LOW){

    snake_head->direction = LEFT;

  }else if(digitalRead(UP_BUTTON) == LOW){

    snake_head->direction = UP;

  }else if(digitalRead(RIGHT_BUTTON) == LOW){

    snake_head->direction = RIGHT;

  }else if(digitalRead(DOWN_BUTTON) == LOW){

    snake_head->direction = DOWN;
    
  }else{
    return 0;
  }

  delay(200); // Prevent accidental "double-press".
  return snake_head->direction;

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

  //setting up the beginning position of the snake.
  //head_pos_x = ROWS/2 - 1;
  //head_pos_y = COLS/2 - 1;
  snake_head->pos_x = ROWS/2 - 1;
  snake_head->pos_y = COLS/2 - 1;
  //set_state(snake_head->pos_x, snake_head->pos_y, 1);
  snake_head->direction = RIGHT;

  //somehow, some amount of my microcontroller's memory was corrupted, so i had to use this:
  padding[199] = 0;
  if(padding[0] == 0){
    digitalWrite(A4, LOW);
  }

  add_snake_segment();
  add_snake_segment();
}

void loop() {

  timed_event(1000, movement_wait);

  game_logic();

}
