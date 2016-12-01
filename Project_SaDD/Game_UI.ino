#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "storage.h"

// different screens (states) of game
static enum GamePages
{
  Welcome             = 0,
  SelectPlayers       = 1,
  SelectDifficulty    = 2,
  PassDevice          = 3,
  ButtonsGame         = 4,
  PotentiometerGame   = 5,
  ShakeGame           = 6,
  GameResult          = 7,
  DisplayHighscores   = 8,
  NewRecord           = 9,
  NumberOfPages       = 10,
} gameUiPage = Welcome;

// input counts
const uint32_t SwitchCount = 2;
const uint32_t ButtonCount = 2;
const uint32_t ShakeDirectionCount = 2;
const uint32_t PotentiometerPositionCount = 15;

const uint32_t Switches[SwitchCount] = { PA_7, PA_6 };
const uint32_t Buttons[ButtonCount] = { PD_2, PE_0 };
const uint32_t Potentiometer = PE_3;

// game info
const size_t   MaxPlayers = 6;
const uint32_t GamesCount = 3;
const uint32_t MaxActions = 10;
const uint32_t MaxShakes  = 3;
const uint32_t TimeLimit = 10000;

// time in milliseconds before registering another action
// prevents single shake as being registered as more than one
const uint32_t CooldownLength = 500;


// Record Handling
uint32_t currScore = 0;
char * name;
int spot = 0;
bool first = true;
int recordIndex = 0;
bool display = true;
const uint32_t NameSize = 8;
const uint32_t recordCount = 10;
char * recordHolders [recordCount];
uint32_t records [recordCount];
const unsigned int recordAddress = 0;
int newRecordIndex = 0;


// difficulty of game
typedef enum Difficulty
{
  Easy            = 1,
  Normal          = 2,
  Hard            = 3,
  DifficultyCount = 3,
} difficulty;

// button state
struct ButtonState
{
  bool state;
  bool beingDepressed;
};

// game values
struct GameState
{
  int playerCount;
  int playerIndex;
  int playersRemainingCount;
  bool playersRemaining[MaxPlayers];

  int gameIndex;
  int timeElapsed;
  int timeLimit;                        // in game
  int waitLimit;                        // between games
  char objectives[MaxActions];
  int objectiveIndex;
  int cooldownStart;

  enum Difficulty gameDifficulty;
} game;

// input states
static struct InputState
{
  bool                switches[2];
  struct ButtonState  buttons[2];
  float               dial;
} gameInputState;

// initializer
void GameUIInit()
{
  OrbitOledInit();
  OrbitOledClear();
  OrbitOledClearBuffer();
  OrbitOledSetFillPattern(OrbitOledGetStdPattern(iptnSolid));
  OrbitOledSetDrawMode(modOledSet);

  gameInputState = { 0 };
  game = { 0 };

  // set buttons and switches to input mode
  for (int i = 0; i < SwitchCount; ++i )
    pinMode(Switches[i], INPUT);
  for (int i = 0; i < ButtonCount; ++i )
    pinMode(Buttons[i], INPUT);

  // Initialize records array then receive past recordholders
  for (int i = 0; i < recordCount; i++) {
    recordHolders[i] = (char *)malloc(NameSize);
  }

  /*
    char c [] = {'f','i','r','s','t'};
    writeName(c,0,5);
    char d [] = {'t','e','s','t'};
    writeName(d,12,4);
    char fake [] = {'f','a','k','e'};
    uint32_t z=8989;
    uint32_t u=5656;
    uint32_t none = 0;
    writeRecord(&u,20,4);
    writeRecord(&z,32,4);
    for(unsigned int i=3;i<10;i++){
        writeName(fake,12*i,4);
        writeRecord(&none, 12*i+8,4);
    }*/

  for (int i = 0; i < recordCount; i++) {
    getName(recordHolders[i], recordAddress + 12 * i, NameSize);
    records[i] = getRecord(recordAddress + 12 * i + 8, sizeof(uint32_t));
  }
}

// run welcome screen
static void handlePageWelcome()
{
  OrbitOledMoveTo(0, 0);
  OrbitOledDrawString("~Ding Dong~");

  OrbitOledMoveTo(0, 12);
  OrbitOledDrawString("Btn 1 to start");

  OrbitOledMoveTo(0, 24);
  OrbitOledDrawString("Swtch 2 Records");

  if (gameInputState.buttons[0].beingDepressed)
    changeState();
  else if (!gameInputState.switches[1]) {
    OrbitOledClearBuffer();
    OrbitOledClear();
    gameUiPage = DisplayHighscores;
  }
}

// choose number of players with knob
static void handlePageSelectPlayers()
{
  OrbitOledMoveTo(3, 5);
  OrbitOledDrawString("# of players");

  OrbitOledMoveTo(0, 20);
  OrbitOledDrawString("Twist: ");
  game.playerCount = (((int)gameInputState.dial) / 200 % MaxPlayers) + 1;
  OrbitOledDrawChar((char)game.playerCount + 48);

  if (gameInputState.buttons[0].beingDepressed) {
    game.playersRemainingCount = game.playerCount;
    for (int i = 0; i < MaxPlayers; i++) {
      if (i < game.playerCount)
        game.playersRemaining[i] = true;
      else
        game.playersRemaining[i] = false;
    }
    changeState();
  }
}

// choose number of players with knob
static void handlePageSelectDifficulty()
{
  OrbitOledMoveTo(0, 5);
  OrbitOledDrawString("How tuf r ya?");

  OrbitOledMoveTo(0, 20);
  OrbitOledDrawString("Twist: ");
  game.gameDifficulty = (enum Difficulty) (((int)gameInputState.dial) / 300 % DifficultyCount + 1);
  switch (game.gameDifficulty) {
    case Easy:   OrbitOledDrawString("EZ-PZ!"); break;
    case Normal: OrbitOledDrawString("Avg Jo"); break;
    case Hard:   OrbitOledDrawString("Tuff?!"); break;
  }

  if (gameInputState.buttons[0].beingDepressed)
  {
    switch (game.gameDifficulty) {
      case Easy:   game.waitLimit = 5000; game.timeLimit = 10000; break;
      case Normal: game.waitLimit = 2500; game.timeLimit = 7000; break;
      case Hard:   game.waitLimit = 1000; game.timeLimit = 5000; break;
    }
    changeState();
  }
}

// state during which players exchange device
static void handlePassDevice()
{
  OrbitOledMoveTo(5, 0);
  OrbitOledDrawString("Pass to P");
  OrbitOledDrawChar(49 + game.playerIndex);

  OrbitOledMoveTo(5, 15);
  OrbitOledDrawString("Time Left: ");
  OrbitOledDrawChar((char) 48 + 5 - (game.timeElapsed++ / (game.waitLimit / 5)));

  if (game.timeElapsed > game.waitLimit)
    changeState();
}

// run button game
static void handleButtonsGame() {

  OrbitOledMoveTo(0, 0);
  OrbitOledDrawString("Press Btns");

  //  OrbitOledMoveTo(0, 50);
  OrbitOledDrawChar('[');
  if (game.timeLimit == 10000 && game.timeElapsed < 1000)
    OrbitOledDrawString("10");
  else {
    OrbitOledDrawChar(' ');
    OrbitOledDrawChar((game.timeLimit / 1000) - (game.timeElapsed) / 1000 + 48);
  }
  OrbitOledDrawChar(']');

  OrbitOledMoveTo(0, 15);
  OrbitOledDrawString(game.objectives);

  // countdown
  if (game.timeElapsed++ == game.timeLimit) {
    eliminatePlayer();
    gameUiPage = PassDevice;
  }

  // button pressed
  if (gameInputState.buttons[0].beingDepressed) {
    if (game.objectives[game.objectiveIndex] == '1') {
      game.objectives[game.objectiveIndex++] = ' ';
    } else {
      eliminatePlayer();
      gameUiPage = PassDevice;
    }
  } else if (gameInputState.buttons[1].beingDepressed) {
    if (game.objectives[game.objectiveIndex] == '2') {
      game.objectives[game.objectiveIndex++] = ' ';
    } else {
      eliminatePlayer();
      gameUiPage = PassDevice;
    }
  }

  // completed objective
  if (game.objectiveIndex == MaxActions) {
    game.objectiveIndex = 0;
    OrbitOledClearBuffer();
    OrbitOledClear();
    gameUiPage = PassDevice;
  }
}

char pot [] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
static void handlePotentiometerGame() {
  OrbitOledMoveTo(0, 0);
  OrbitOledDrawString("Hit the line!");
  OrbitOledDrawChar('[');
  if (game.timeLimit == 10000 && game.timeElapsed < 1000)
    OrbitOledDrawString("10");
  else {
    OrbitOledDrawChar(' ');
    OrbitOledDrawChar((game.timeLimit / 1000) - (game.timeElapsed) / 1000 + 48);
  }
  OrbitOledDrawChar(']');

  int spot = (analogRead(Potentiometer) / 285 % 15);

  for (int i = 0; i < 15; i++)pot[i] = ' ';
  pot[spot] = 'X';
  OrbitOledMoveTo(3, 12);
  OrbitOledDrawString(pot);
  OrbitOledMoveTo(game.objectives[game.objectiveIndex], 24);
  OrbitOledDrawChar('|');
  OrbitOledMoveTo(game.objectives[game.objectiveIndex] + 6, 24);
  OrbitOledDrawChar('|');

  if (8 * spot == game.objectives[game.objectiveIndex]) {
    OrbitOledClearBuffer();
    OrbitOledClear();
    game.objectiveIndex++;
  }
  if (game.objectiveIndex == 10) {
    OrbitOledClearBuffer();
    OrbitOledClear();
    game.objectiveIndex = 0;
    gameUiPage = PassDevice;
  }
}

static void handleShakeGame() {
  OrbitOledMoveTo(0, 0);
  OrbitOledDrawString("SHAKE ");
  switch (game.objectives[game.objectiveIndex]) {
    case 0: OrbitOledDrawString("Sideways"); break;
    case 1: OrbitOledDrawString("Up/Down"); break;
  }

  // draw coundown timer
  OrbitOledDrawChar('[');
  OrbitOledDrawChar((game.timeLimit / 1000) - (game.timeElapsed) / 1000 + 48);
  OrbitOledDrawChar(']');
  // countdown
  if (game.timeElapsed++ == game.timeLimit) {
    eliminatePlayer();
    changeGame();
  }

  if (game.timeElapsed > game.cooldownStart + CooldownLength) {
    if (game.objectives[game.objectiveIndex] == 0 && xShaking()) {
      game.objectives[game.objectiveIndex++] = ' ';
      game.cooldownStart = game.timeElapsed;
      OrbitOledClearBuffer();
      OrbitOledClear();
    }
    if (game.objectives[game.objectiveIndex] == 1 && yShaking()) {
      game.objectives[game.objectiveIndex++] = ' ';
      game.cooldownStart = game.timeElapsed;
      OrbitOledClearBuffer();
      OrbitOledClear();
    }
  }

  // completed objective
  if (game.objectiveIndex == MaxShakes) {
    changeGame();
  }
}

// set new game
static void changeGame() {
  // get random new player
  game.playerIndex = (rand() % game.playersRemainingCount);
  do {
    if (++game.playerIndex == MaxPlayers) {
      game.playerIndex = 0;
    }
  } while (game.playersRemaining[game.playerIndex] == false);

  // get random new game
  // assumes button game is first game
  gameUiPage = (enum GamePages) (rand() % GamesCount + ButtonsGame);
  setobjectives();
  game.objectiveIndex == 0;
  game.timeElapsed = 0;
}

// create new random set of actions for next game
// should only be used inside changeGame()
static void setobjectives() {
  switch (gameUiPage) {
    case ButtonsGame:
      for (int i = 0; i < MaxActions; i++)
        game.objectives[i] = (char)49 + (rand() % ButtonCount);
      break;
    case PotentiometerGame:
      for (int i = 0; i < MaxActions; i++)
        game.objectives[i] = 8 * (rand() % PotentiometerPositionCount);
      break;
    case ShakeGame:
      for (int i = 0; i < MaxShakes; i++)
        game.objectives[i] = rand() % ShakeDirectionCount;
      for (int i = MaxShakes; i < MaxActions; i++)
        game.objectives[i] = 0;
      break;
  }
  game.objectiveIndex = 0;
}

// remove current player from competition
// should be followed by changeGame()
static void eliminatePlayer() {
  game.playersRemaining[game.playerIndex] = false;
  game.playersRemainingCount -= 1;
}

// Need some kind of way to track records maybe with an int
static void handleGameResult()
{
  OrbitOledMoveTo(0, 0);
  OrbitOledDrawString("Game Over!");
  OrbitOledMoveTo(0, 12);
  OrbitOledDrawString("Press any button");

  if (gameInputState.buttons[0].beingDepressed || gameInputState.buttons[1].beingDepressed) {
    newRecordIndex = 10;
    while (currScore > records[newRecordIndex - 1] && newRecordIndex > 0)
      newRecordIndex--;

    // Update the list of records
    for (int i = 9; i > newRecordIndex; i--) {
      records[i] = records[i - 1];
      writeRecord(&records[i - 1], recordAddress + 12 * i + 8, 4);
      char * tempName = (char *) malloc(8);
      recordHolders[i] = recordHolders[i - 1];
      getName(tempName, recordAddress + 12 * i - 12, 8);
      writeName(tempName, recordAddress + 12 * i, 8);
    }
    if (newRecordIndex != 10) {
      gameUiPage = NewRecord;
      records[newRecordIndex] = currScore;
      writeRecord(&currScore, recordAddress + 12 * newRecordIndex + 8, 4);
    }
    else {
      OrbitOledClearBuffer();
      OrbitOledClear();
      gameUiPage = PassDevice; // Welcome
    }
  }
}

static void handleNewRecord() {
  if (first) {
    name = (char *) malloc(9);
    for (int i = 0; i < 8; i++)name[i] = ' ';
    name[8] = '\0';
    first = false;
  }
  OrbitOledMoveTo(0, 0);
  OrbitOledDrawString("New Record!");
  OrbitOledMoveTo(0, 12);
  OrbitOledDrawString("Enter your name:");

  name[spot] = (char)(65 + (analogRead(Potentiometer) / 160 % 26));
  OrbitOledMoveTo(0, 24);
  OrbitOledDrawString(name);

  if (gameInputState.buttons[0].beingDepressed && spot < 8) {
    spot++;
  }
  else if (!gameInputState.switches[0] && spot > 0) {
    spot--;
  }
  else if (gameInputState.buttons[1].beingDepressed && spot != 0) {
    writeName(name, recordAddress + newRecordIndex * 12, 8);
    getName(recordHolders[newRecordIndex], recordAddress + newRecordIndex * 12, NameSize);
    OrbitOledClearBuffer();
    OrbitOledClear();
    gameUiPage = PassDevice; //Welcome
    first = true;
  }
}

static void handleDisplayHighscores() {
  if (display) {
    OrbitOledMoveTo(0, 0);
    OrbitOledDrawString("Records.Back S2");

    OrbitOledMoveTo(0, 11);
    OrbitOledDrawString("Name       Score");

    char * recordHolder = recordHolders[recordIndex];
    //recordHolder = (char *) realloc(recordHolder, 9);
    //recordHolder[8]='\0';

    OrbitOledMoveTo(0, 22);
    OrbitOledDrawString(recordHolder);
    OrbitOledDrawString("   ");
    OrbitOledDrawString(intToChar(records[recordIndex]));
    display = false;
  }

  if (gameInputState.switches[1]) {
    OrbitOledClearBuffer();
    OrbitOledClear();
    recordIndex = 0;
    display = true;
    gameUiPage = Welcome;
  }
  else if (gameInputState.buttons[0].beingDepressed && recordIndex < 9) {
    recordIndex++;
    OrbitOledClearBuffer();
    OrbitOledClear();
    display = true;
  }
  else if (gameInputState.buttons[1].beingDepressed && recordIndex > 0) {
    recordIndex--;
    OrbitOledClearBuffer();
    OrbitOledClear();
    display = true;
  }
}

// updates input readings
static void uiInputTick()
{
  // detect switch positions
  for (int i = 0; i < SwitchCount; ++i )
    gameInputState.switches[i] = digitalRead(Switches[i]);

  // detect when buttons are pressed down
  for (int i = 0; i < ButtonCount; ++i )
  {
    bool previousState = gameInputState.buttons[i].state;
    gameInputState.buttons[i].state = digitalRead(Buttons[i]);
    gameInputState.buttons[i].beingDepressed = (!previousState && gameInputState.buttons[i].state);
  }

  // get dial reading
  gameInputState.dial = analogRead(Potentiometer);
}

// determine which state to change to
// clear screen
void changeState()
{
  OrbitOledClearBuffer();
  OrbitOledClear();
  switch (gameUiPage)
  {
    case Welcome:
      gameUiPage = SelectPlayers;
      break;

    case SelectPlayers:
      gameUiPage = SelectDifficulty;
      break;

    case SelectDifficulty:
      gameUiPage = PassDevice;
      break;

    case PassDevice:
      changeGame();
      break;

    case ButtonsGame:
      gameUiPage = PassDevice;
      break;

    case PotentiometerGame:
      gameUiPage = PassDevice;
      break;

    case ShakeGame:
      gameUiPage = PassDevice;
      break;

    case GameResult:
      gameUiPage = Welcome;
      break;

    case DisplayHighscores:
      gameUiPage = Welcome;
      break;
  }
}

// determine which state game is in
// update screen
void GameUITick()
{
  uiInputTick();
  switch (gameUiPage)
  {
    case Welcome:
      handlePageWelcome();
      break;

    case SelectPlayers:
      handlePageSelectPlayers();
      break;

    case SelectDifficulty:
      handlePageSelectDifficulty();
      break;

    case PassDevice:
      handlePassDevice();
      break;

    case ButtonsGame:
      handleButtonsGame();
      break;

    case PotentiometerGame:
      handlePotentiometerGame();
      break;

    case ShakeGame:
      handleShakeGame();
      break;

    case GameResult:
      handleGameResult();
      break;

    case NewRecord:
      handleNewRecord();
      break;

    case DisplayHighscores:
      handleDisplayHighscores();
      break;
  }
  OrbitOledUpdate();
}
