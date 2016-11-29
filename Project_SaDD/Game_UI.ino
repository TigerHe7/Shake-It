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
  DialGame            = 5,
  ShakeGame           = 6,
  GameResult          = 7,
  DisplayHighscores   = 8,
  NumberOfPages       = 9,
} gameUiPage = Welcome;

// input counts
const uint32_t SwitchCount = 2;
const uint32_t ButtonCount = 2;
const uint32_t ShakeDirectionCount = 3;
const uint32_t DialPositionCount = 10;

// input identifiers
const uint32_t Switches[SwitchCount] = { PA_7, PA_6 };
const uint32_t Buttons[ButtonCount] = { PD_2, PE_0 };
const uint32_t Potentiometer = PE_3;

// game info
const size_t   MaxPlayers = 6;
const uint32_t GamesCount = 3;
const int MaxActions = 10;

// record info
const uint32_t NameSize = 8;
const uint32_t RecordCount = 10;
char * btnRecordHolders [RecordCount];
uint32_t btnRecords [RecordCount];
const unsigned int btnAddress =0;
char * shakeRecordHolders [RecordCount];
uint32_t shakeRecords [RecordCount];
const unsigned int shakeAddress =120;

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
  for(int i = 0; i < SwitchCount; ++i )
    pinMode(Switches[i], INPUT);
  for(int i = 0; i < ButtonCount; ++i )
    pinMode(Buttons[i], INPUT);

  // Initialize records array then receive past recordholders
  for(int i=0;i<RecordCount;i++){
    btnRecordHolders[i]= (char *)malloc(NameSize);
    shakeRecordHolders[i]=(char*)malloc(NameSize);
  }

  for(int i=0;i<RecordCount;i++){
    getName(btnRecordHolders[i],btnAddress+12*i,NameSize);
    btnRecords[i]=getRecord(btnAddress+12*i+8, sizeof(uint32_t));
    getName(shakeRecordHolders [i], shakeAddress+12*i, NameSize);
    shakeRecords[i]=getRecord(shakeAddress+12*i+8, sizeof(uint32_t));
  }
}

// run welcome screen
static void handlePageWelcome()
{
  OrbitOledMoveTo(5, 5);
  OrbitOledDrawString("~Ding Dong~");

  OrbitOledMoveTo(0, 20);
  OrbitOledDrawString("Press Btn 1");

  if(gameInputState.buttons[0].beingDepressed)
    changeState();
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

  if(gameInputState.buttons[0].beingDepressed)
      changeState();
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

  if(gameInputState.buttons[0].beingDepressed)
  {
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
  OrbitOledDrawChar((char) 48 + 5 - (game.timeElapsed++ / (game.waitLimit/5)));

  if (game.timeElapsed > game.waitLimit)
    changeState();
}

// run button game
static void handleButtonsGame() {

  OrbitOledMoveTo(0, 0);
  OrbitOledDrawString("Press Btns");

//  OrbitOledMoveTo(0, 50);
  OrbitOledDrawChar('[');
  OrbitOledDrawChar(game.timeElapsed + 48);
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
    if (game.objectives[game.objectiveIndex] == 1) {
      game.objectives[game.objectiveIndex++] = ' ';
    } else {
      eliminatePlayer();
      gameUiPage = PassDevice;
    }
  } else if (gameInputState.buttons[1].beingDepressed) {
    if (game.objectives[game.objectiveIndex] == 2) {
      game.objectives[game.objectiveIndex++] = ' ';
    } else {
      eliminatePlayer();
      gameUiPage = PassDevice;
    }
  }

  // completed objective
  if (game.objectiveIndex == MaxActions) {
    OrbitOledClearBuffer();
    OrbitOledClear();
    gameUiPage = PassDevice;
  }
}

// *** to be implemented
static void handleDialGame() {
  OrbitOledMoveTo(0, 0);
  OrbitOledDrawString("fit between: ");
}

// *** to be implemented
static void handleShakeGame() {
  OrbitOledMoveTo(0, 0);
  OrbitOledDrawString("shake in order: ");
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
        game.objectives[i] = rand() % ButtonCount + 1;
      break;
    case DialGame:
      for (int i = 0; i < MaxActions; i++)
        game.objectives[i] = rand() % DialPositionCount;
      break;
    case ShakeGame:
      for (int i = 0; i < MaxActions; i++)
        game.objectives[i] = rand() % ShakeDirectionCount;
      break;
  }
}

// remove current player from competition
// should be followed by changeGame()
static void eliminatePlayer() {
  game.playersRemaining[game.playerIndex] = false;
  game.playersRemainingCount -= 1;
}

// Need some kind of way to track records maybe with an int
static void handleGameResult(uint32_t score)
{
  int index=10;
  while(score> btnRecords[index-1] && index>0)
    index--;

// Update the list of records
  for(int i=9;i>index;i--){
    btnRecords[i]=btnRecords[i-1];
    writeRecord(&btnRecords[i-1],btnAddress+12*i+8,4);
    char * tempName = (char *) malloc(8);
    btnRecordHolders[i]=btnRecordHolders[i-1];
    getName(tempName, btnAddress+12*i-12, 8);
    writeName(tempName, btnAddress+12*i,8);
  }
  if(index!=10){
    getName(index);
    btnRecords[index]=score;
    writeRecord(&score, btnAddress+12*index+8, 4);
  }
}

// Need another state for this
static void getName(int i){
  char name []= {' ',' ',' ',' ',' ',' ',' ',' '};
  int index=0;
  name[index] = ((((int)gameInputState.dial) / 200) % MaxPlayers) + 1;

  if(gameInputState.buttons[0].beingDepressed){
    index++;
  }
  if(gameInputState.buttons[1].beingDepressed){
    btnRecordHolders[i]=name;
    writeName(name, btnAddress+12*i, 8);
  }
  handlePageWelcome();
}

// TODO need to say which button to press to return to welcome page
static void handleDisplayHighscores() {
  int index = 0;
  char * recordHolder = btnRecordHolders[index];

  while(!gameInputState.buttons[0].beingDepressed){
  if (gameInputState.buttons[1].beingDepressed && index <9) {
       index++;
       recordHolder = btnRecordHolders[index];
       OrbitOledMoveTo(0, 0);
       OrbitOledDrawString(recordHolder);
       OrbitOledMoveTo(0, 15);
       OrbitOledDrawString(intToChar(btnRecords[index]));
    }
  else if(gameInputState.buttons[2].beingDepressed && index>0){
      index--;
      recordHolder = btnRecordHolders[index];
      OrbitOledMoveTo(0, 0);
      OrbitOledDrawString(recordHolder);
      OrbitOledMoveTo(0, 15);
      OrbitOledDrawString(intToChar(btnRecords[index]));
    }
  }

  handlePageWelcome();
}

// updates input readings
static void uiInputTick()
{
  //
  for(int i = 0; i < SwitchCount; ++i )
    gameInputState.switches[i] = digitalRead(Switches[i]);

    // detect when buttons are pressed down
  for(int i = 0; i < ButtonCount; ++i )
  {
    bool previousState = gameInputState.buttons[i].state;
    gameInputState.buttons[i].state = digitalRead(Buttons[i]);
    gameInputState.buttons[i].beingDepressed = (!previousState && gameInputState.buttons[i].state);
  }

  // get dial reading
  gameInputState.dial = analogRead(Potentiometer);
}

// determine which state game is in
// update screen
void changeState()
{
  OrbitOledClearBuffer();
  OrbitOledClear();
  switch(gameUiPage)
  {
  case Welcome:
    gameUiPage = SelectPlayers;
    break;

  case SelectPlayers:
    game.playersRemainingCount = game.playerCount;
    for (int i = 0; i < MaxPlayers; i++) {
      if (i < game.playerCount)
        game.playersRemaining[i] = true;
      else
        game.playersRemaining[i] = false;
    }
    gameUiPage = SelectDifficulty;
    break;

  case SelectDifficulty:
    switch (game.gameDifficulty) {
      case Easy:   game.waitLimit = 5000; game.timeLimit = 10000; break;
      case Normal: game.waitLimit = 2500; game.timeLimit = 7000; break;
      case Hard:   game.waitLimit = 1000; game.timeLimit = 5000; break;
    }
    gameUiPage = PassDevice;
    break;

  case PassDevice:
    OrbitOledClearBuffer();
    OrbitOledClear();
    changeGame();
    break;

  case ButtonsGame:
    handleButtonsGame();
    break;

  case DialGame:
    handleDialGame();
    break;

  case ShakeGame:
    handleShakeGame();
    break;

  case GameResult:
    handleGameResult(1);
    break;

  case DisplayHighscores:
    handleDisplayHighscores();
    break;
  }
  OrbitOledUpdate();
}


// determine which state game is in
// update screen
void GameUITick()
{
  uiInputTick();
  switch(gameUiPage)
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

  case DialGame:
    handleDialGame();
    break;

  case ShakeGame:
    handleShakeGame();
    break;

  case GameResult:
    handleGameResult(1);
    break;

  case DisplayHighscores:
    handleDisplayHighscores();
    break;
  }
  OrbitOledUpdate();
}
