/************************************************************************
**
** NAME:        mtttier.c
**
** DESCRIPTION: Tic-Tac-Tier!
**				Basically, this is a copy-pasted version of mttt.c,
**				only changed to use generic hash and Tier Gamesman,
**				removing unnecessary things like GPS stuff.
**
** AUTHOR:      Dan Garcia  -  University of California at Berkeley
**              Copyright (C) Dan Garcia, 1995. All rights reserved.
**				Copy-Pasted and Revised 2006 by Max Delgadillo
**
** DATE:        07/24/06
**
** UPDATE HIST:
**
**  7/24/06 :	First version of the file. SHOULD be bug-free...
**				Four of the six API functions are implemented. Only
**				gInitHashWindow and gPositionToTierPosition are left
**				to write (since the current version of the solver
**				doesn't use either of the two functions yet). They
**				should be added by the next update.
**  7/29/06 :	Fixed a BIG gNumberOfPositions glitch, as well as a
**				WhoseMove glitch that was causing double boards in the
**				hash. Also, gInitHashWindow and gPositionToTierPosition
**				are now correctly implemented, finally completing the
**				entire Tier Gamesman API example.
**
**************************************************************************/

/*************************************************************************
**
** Everything below here must be in every game file
**
**************************************************************************/

#include <stdio.h>
#include "gamesman.h"

POSITION gNumberOfPositions  = 0;
POSITION kBadPosition	     = -1;

POSITION gInitialPosition    =  0;
POSITION gMinimalPosition    =  0;

STRING   kAuthorName         = "Dan Garcia (and Max Delgadillo)";
STRING   kGameName           = "Tic-Tac-Tier";
STRING   kDBName             = "tttier";
BOOLEAN  kPartizan           = TRUE;
BOOLEAN  kDebugMenu          = FALSE;
BOOLEAN  kGameSpecificMenu   = FALSE;
BOOLEAN  kTieIsPossible      = TRUE;
BOOLEAN  kLoopy               = FALSE;
BOOLEAN  kDebugDetermineValue = FALSE;
void*	 gGameSpecificTclInit = NULL;

STRING   kHelpGraphicInterface =
"The LEFT button puts an X or O (depending on whether you went first\n\
or second) on the spot the cursor was on when you clicked. The MIDDLE\n\
button does nothing, and the RIGHT button is the same as UNDO, in that\n\
it reverts back to your your most recent position.";

STRING   kHelpTextInterface    =
"On your turn, use the LEGEND to determine which number to choose (between\n\
1 and 9, with 1 at the upper left and 9 at the lower right) to correspond\n\
to the empty board position you desire and hit return. If at any point\n\
you have made a mistake, you can type u and hit return and the system will\n\
revert back to your most recent position.";

STRING   kHelpOnYourTurn =
"You place one of your pieces on one of the empty board positions.";

STRING   kHelpStandardObjective =
"To get three of your markers (either X or O) in a row, either\n\
horizontally, vertically, or diagonally. 3-in-a-row WINS.";

STRING   kHelpReverseObjective =
"To force your opponent into getting three of his markers (either X or\n\
O) in a row, either horizontally, vertically, or diagonally. 3-in-a-row\n\
LOSES.";

STRING   kHelpTieOccursWhen = /* Should follow 'A Tie occurs when... */
"the board fills up without either player getting three-in-a-row.";

STRING   kHelpExample =
"         ( 1 2 3 )           : - - -\n\
LEGEND:  ( 4 5 6 )  TOTAL:   : - - - \n\
         ( 7 8 9 )           : - - - \n\n\
Computer's move              :  3    \n\n\
         ( 1 2 3 )           : - - X \n\
LEGEND:  ( 4 5 6 )  TOTAL:   : - - - \n\
         ( 7 8 9 )           : - - - \n\n\
     Dan's move [(u)ndo/1-9] : { 2 } \n\n\
         ( 1 2 3 )           : - O X \n\
LEGEND:  ( 4 5 6 )  TOTAL:   : - - - \n\
         ( 7 8 9 )           : - - - \n\n\
Computer's move              :  6    \n\n\
         ( 1 2 3 )           : - O X \n\
LEGEND:  ( 4 5 6 )  TOTAL:   : - - X \n\
         ( 7 8 9 )           : - - - \n\n\
     Dan's move [(u)ndo/1-9] : { 9 } \n\n\
         ( 1 2 3 )           : - O X \n\
LEGEND:  ( 4 5 6 )  TOTAL:   : - - X \n\
         ( 7 8 9 )           : - - O \n\n\
Computer's move              :  5    \n\n\
         ( 1 2 3 )           : - O X \n\
LEGEND:  ( 4 5 6 )  TOTAL:   : - X X \n\
         ( 7 8 9 )           : - - O \n\n\
     Dan's move [(u)ndo/1-9] : { 7 } \n\n\
         ( 1 2 3 )           : - O X \n\
LEGEND:  ( 4 5 6 )  TOTAL:   : - X X \n\
         ( 7 8 9 )           : O - O \n\n\
Computer's move              :  4    \n\n\
         ( 1 2 3 )           : - O X \n\
LEGEND:  ( 4 5 6 )  TOTAL:   : X X X \n\
         ( 7 8 9 )           : O - O \n\n\
Computer wins. Nice try, Dan.";

/*************************************************************************
**
** Everything above here must be in every game file
**
**************************************************************************/

/*************************************************************************
**
** Every variable declared here is only used in this file (game-specific)
**
**************************************************************************/

#define BOARDSIZE     9           /* 3x3 board */
// I decided on chars instead of the "BlankOX" struct because it's
// easier for the generic hash to handle.
#define Blank	'-'
#define o		'O'
#define x		'X'
// This is so I don't have to change "BlankOX" occurences everywhere.
typedef char BlankOX;

/** Function Prototypes **/
BOOLEAN AllFilledIn(BlankOX*);
POSITION BlankOXToPosition(BlankOX*,BlankOX);
BlankOX* PositionToBlankOX(POSITION);
BOOLEAN ThreeInARow(BlankOX*, int, int, int);
POSITION GetCanonicalPosition(POSITION);
BlankOX WhoseTurn(POSITION);
STRING MoveToString( MOVE );

//TIER GAMESMAN
void SetupTierStuff();
POSITION InitializeHashWindow(TIER, POSITION);
TIERLIST* TierChildren(TIER);
TIER PositionToTier(POSITION);
TIERPOSITION PositionToTierPosition(POSITION, TIER);
UNDOMOVELIST* GenerateUndoMovesToTier(POSITION, TIER);
POSITION UnDoMove(POSITION, UNDOMOVE);
// until I learn how to overwrite contexts:
int Tier0Context;
int HashWindowContext;
// Actual functions are at the end of this file

//SYMMETRIES
BOOLEAN kSupportsSymmetries = TRUE; /* Whether we support symmetries */

#define NUMSYMMETRIES 8   /*  4 rotations, 4 flipped rotations */

int gSymmetryMatrix[NUMSYMMETRIES][BOARDSIZE];

/* Proofs of correctness for the below arrays:
**
** FLIP						ROTATE
**
** 0 1 2	2 1 0		0 1 2		6 3 0		8 7 6		2 5 8
** 3 4 5 -> 5 4 3		3 4 5	->	7 4 1  ->	5 4 3	->	1 4 7
** 6 7 8	8 7 6		6 7 8		8 5 2		2 1 0		2 1 0
*/

/* This is the array used for flipping along the N-S axis */
int gFlipNewPosition[] = { 2, 1, 0, 5, 4, 3, 8, 7, 6 };

/* This is the array used for rotating 90 degrees clockwise */
int gRotate90CWNewPosition[] = { 6, 3, 0, 7, 4, 1, 8, 5, 2 };


/************************************************************************
**
** NAME:        InitializeGame
**
** DESCRIPTION: Prepares the game for execution.
**              Initializes required variables.
**              Sets up gDatabase (if necessary).
**
************************************************************************/

// for the hash init
int vcfg(int* this_cfg) {
	return (this_cfg[0] == this_cfg[1] || this_cfg[0] + 1 == this_cfg[1]);
}

void InitializeGame()
{
  // SYMMETRY
  gCanonicalPosition = GetCanonicalPosition;
  int i, j, temp; /* temp is used for debugging */
  if(kSupportsSymmetries) { /* Initialize gSymmetryMatrix[][] */
    for(i = 0 ; i < BOARDSIZE ; i++) {
      temp = i;
      for(j = 0 ; j < NUMSYMMETRIES ; j++) {
	if(j == NUMSYMMETRIES/2)
	  temp = gFlipNewPosition[i];
	if(j < NUMSYMMETRIES/2)
	  temp = gSymmetryMatrix[j][i] = gRotate90CWNewPosition[temp];
	else
	  temp = gSymmetryMatrix[j][i] = gRotate90CWNewPosition[temp];
      }
    }
  }

  gMoveToStringFunPtr = &MoveToString;

  //Setup Tier Stuff (at bottom)
  SetupTierStuff();

  //fow now, a GLOBAL HASH:
  int game[10] = { o, 0, 4, x, 0, 5, Blank, 0, 9, -1 };
  gNumberOfPositions = generic_hash_init(BOARDSIZE, game, vcfg);
  HashWindowContext = generic_hash_cur_context();

   // gInitialPosition
  BlankOX* theBlankOX = (BlankOX *) SafeMalloc(BOARDSIZE * sizeof(BlankOX));
  for (i = 0; i < BOARDSIZE; i++)
  	theBlankOX[i] = Blank;
  gInitialPosition = BlankOXToPosition(theBlankOX, x);

}

void FreeGame()
{}

/************************************************************************
**
** NAME:        DebugMenu
**
** DESCRIPTION: Menu used to debub internal problems. Does nothing if
**              kDebugMenu == FALSE
**
************************************************************************/

void DebugMenu() { }

/************************************************************************
**
** NAME:        GameSpecificMenu
**
** DESCRIPTION: Menu used to change game-specific parmeters, such as
**              the side of the board in an nxn Nim board, etc. Does
**              nothing if kGameSpecificMenu == FALSE
**
************************************************************************/

void GameSpecificMenu() { }

/************************************************************************
**
** NAME:        SetTclCGameSpecificOptions
**
** DESCRIPTION: Set the C game-specific options (called from Tcl)
**              Ignore if you don't care about Tcl for now.
**
************************************************************************/

void SetTclCGameSpecificOptions(theOptions)
int theOptions[];
{
  /* No need to have anything here, we have no extra options */
}

/************************************************************************
**
** NAME:        DoMove
**
** DESCRIPTION: Apply the move to the position.
**
** INPUTS:      POSITION position : The old position
**              MOVE     move     : The move to apply.
**
** OUTPUTS:     (POSITION) : The position that results after the move.
**
************************************************************************/

POSITION DoMove(POSITION position, MOVE move)
{
    BlankOX* board = PositionToBlankOX(position);
    board[move] = WhoseTurn(position);
    return BlankOXToPosition(board, (WhoseTurn(position) == x ? o : x));
}

/************************************************************************
**
** NAME:        GetInitialPosition
**
** DESCRIPTION: Ask the user for an initial position for testing. Store
**              it in the space pointed to by initialPosition;
**
** OUTPUTS:     POSITION initialPosition : The position to fill.
**
************************************************************************/

POSITION GetInitialPosition()
{
  BlankOX* board = (BlankOX *) SafeMalloc(BOARDSIZE * sizeof(BlankOX));
  signed char c;
  int i = 0, xcount = 0, ycount = 0;

  printf("\n\n\t----- Get Initial Position -----\n");
  printf("\n\tPlease input the position to begin with.\n");
  printf("\tNote that it should be in the following format:\n\n");
  printf("O - -\nO - -            <----- EXAMPLE \n- X X\n\n");

  getchar();
  while(i < BOARDSIZE && (c = getchar()) != EOF) {
    if(c == 'x' || c == 'X') {
      board[i++] = x; xcount++;
    } else if(c == 'o' || c == 'O' || c == '0') {
      board[i++] = o; ycount++;
    } else if(c == '-')
      board[i++] = Blank;
    else
      ;   /* do nothing */
  }

  return(BlankOXToPosition(board, (xcount == ycount ? x : o)));
}

/************************************************************************
**
** NAME:        PrintComputersMove
**
** DESCRIPTION: Nicely format the computers move.
**
** INPUTS:      MOVE   *computersMove : The computer's move.
**              STRING  computersName : The computer's name.
**
************************************************************************/

void PrintComputersMove(computersMove,computersName)
     MOVE computersMove;
     STRING computersName;
{
  printf("%8s's move              : %2d\n", computersName, computersMove+1);
}

/************************************************************************
**
** NAME:        Primitive
**
** DESCRIPTION: Return the value of a position if it fulfills certain
**              'primitive' constraints. Some examples of this is having
**              three-in-a-row with TicTacToe. TicTacToe has two
**              primitives it can immediately check for, when the board
**              is filled but nobody has one = primitive tie. Three in
**              a row is a primitive lose, because the player who faces
**              this board has just lost. I.e. the player before him
**              created the board and won. Otherwise undecided.
**
** INPUTS:      POSITION position : The position to inspect.
**
** OUTPUTS:     (VALUE) an enum which is oneof: (win,lose,tie,undecided)
**
** CALLS:       BOOLEAN ThreeInARow()
**              BOOLEAN AllFilledIn()
**              PositionToBlankOX()
**
************************************************************************/

VALUE Primitive(POSITION position)
{
  BlankOX* board = PositionToBlankOX(position);
  VALUE value;

  if (ThreeInARow(board, 0, 1, 2) ||
      ThreeInARow(board, 3, 4, 5) ||
      ThreeInARow(board, 6, 7, 8) ||
      ThreeInARow(board, 0, 3, 6) ||
      ThreeInARow(board, 1, 4, 7) ||
      ThreeInARow(board, 2, 5, 8) ||
      ThreeInARow(board, 0, 4, 8) ||
      ThreeInARow(board, 2, 4, 6))
    value = gStandardGame ? lose : win;
  else if (AllFilledIn(board))
    value = tie;
  else
    value = undecided;

  if (board != NULL)
  	SafeFree(board);
  return value;
}

/************************************************************************
**
** NAME:        PrintPosition
**
** DESCRIPTION: Print the position in a pretty format, including the
**              prediction of the game's outcome.
**
** INPUTS:      POSITION position   : The position to pretty print.
**              STRING   playerName : The name of the player.
**              BOOLEAN  usersTurn  : TRUE <==> it's a user's turn.
**
** CALLS:       PositionToBlankOX()
**              GetValueOfPosition()
**              GetPrediction()
**
************************************************************************/

void PrintPosition(POSITION position, STRING playerName, BOOLEAN usersTurn)
{
  BlankOX* board = PositionToBlankOX(position);

  printf("\n         ( 1 2 3 )           : %c %c %c\n",
     board[0], board[1], board[2]);
  printf("LEGEND:  ( 4 5 6 )  TOTAL:   : %c %c %c\n",
	 board[3], board[4], board[5]);
  printf("         ( 7 8 9 )           : %c %c %c %s\n\n",
	 board[6], board[7], board[8], GetPrediction(position,playerName,usersTurn));

  if (board != NULL)
  	SafeFree(board);
}

/************************************************************************
**
** NAME:        GenerateMoves
**
** DESCRIPTION: Create a linked list of every move that can be reached
**              from this position. Return a pointer to the head of the
**              linked list.
**
** INPUTS:      POSITION position : The position to branch off of.
**
** OUTPUTS:     (MOVELIST *), a pointer that points to the first item
**              in the linked list of moves that can be generated.
**
** CALLS:       MOVELIST *CreateMovelistNode(MOVE,MOVELIST *)
**
************************************************************************/

MOVELIST* GenerateMoves(POSITION position)
{
	int i;
	MOVELIST* moves = NULL;

	BlankOX* board = PositionToBlankOX(position);

	for (i = BOARDSIZE-1; i >= 0; i--)
		if (board[i] == Blank)
			moves = CreateMovelistNode(i, moves);

	if (board != NULL)
		SafeFree(board);
	return moves;
}

/**************************************************/
/**************** SYMMETRY FUN BEGIN **************/
/**************************************************/

/************************************************************************
**
** NAME:        GetCanonicalPosition
**
** DESCRIPTION: Go through all of the positions that are symmetrically
**              equivalent and return the SMALLEST, which will be used
**              as the canonical element for the equivalence set.
**
** INPUTS:      POSITION position : The position return the canonical elt. of.
**
** OUTPUTS:     POSITION          : The canonical element of the set.
**
************************************************************************/

POSITION GetCanonicalPosition(POSITION position)
{
  POSITION newPosition, theCanonicalPosition, DoSymmetry();
  int i;

  theCanonicalPosition = position;

  for(i = 0 ; i < NUMSYMMETRIES ; i++) {
    newPosition = DoSymmetry(position, i);    /* get new */
    if(newPosition < theCanonicalPosition)    /* THIS is the one */
      theCanonicalPosition = newPosition;     /* set it to the ans */
  }

  return(theCanonicalPosition);
}

/************************************************************************
**
** NAME:        DoSymmetry
**
** DESCRIPTION: Perform the symmetry operation specified by the input
**              on the position specified by the input and return the
**              new position, even if it's the same as the input.
**
** INPUTS:      POSITION position : The position to branch the symmetry from.
**              int      symmetry : The number of the symmetry operation.
**
** OUTPUTS:     POSITION, The position after the symmetry operation.
**
************************************************************************/

POSITION DoSymmetry(POSITION position, int symmetry)
{
  int i;
  BlankOX *board, *symmBoard;

  board = PositionToBlankOX(position);
  symmBoard = (BlankOX *) SafeMalloc(BOARDSIZE * sizeof(BlankOX));

  /* Copy from the symmetry matrix */

  for(i = 0 ; i < BOARDSIZE ; i++)
    symmBoard[i] = board[gSymmetryMatrix[symmetry][i]];

  if (board != NULL)
    SafeFree(board);
  return(BlankOXToPosition(symmBoard, WhoseTurn(position)));
}

/**************************************************/
/**************** SYMMETRY FUN END ****************/
/**************************************************/

/************************************************************************
**
** NAME:        GetAndPrintPlayersMove
**
** DESCRIPTION: This finds out if the player wanted an undo or abort or not.
**              If so, return Undo or Abort and don't change theMove.
**              Otherwise get the new theMove and fill the pointer up.
**
** INPUTS:      POSITION *thePosition : The position the user is at.
**              MOVE *theMove         : The move to fill with user's move.
**              STRING playerName     : The name of the player whose turn it is
**
** OUTPUTS:     USERINPUT             : Oneof( Undo, Abort, Continue )
**
** CALLS:       ValidMove(MOVE, POSITION)
**              BOOLEAN PrintPossibleMoves(POSITION) ...Always True!
**
************************************************************************/

USERINPUT GetAndPrintPlayersMove(POSITION thePosition, MOVE *theMove, STRING playerName)
{
  USERINPUT ret;

  do {
    printf("%8s's move [(u)ndo/1-9] :  ", playerName);

    ret = HandleDefaultTextInput(thePosition, theMove, playerName);
    if(ret != Continue)
      return(ret);

  }
  while (TRUE);
  return(Continue); /* this is never reached, but lint is now happy */
}

/************************************************************************
**
** NAME:        ValidTextInput
**
** DESCRIPTION: Return TRUE iff the string input is of the right 'form'.
**              For example, if the user is allowed to select one slot
**              from the numbers 1-9, and the user chooses 0, it's not
**              valid, but anything from 1-9 IS, regardless if the slot
**              is filled or not. Whether the slot is filled is left up
**              to another routine.
**
** INPUTS:      STRING input : The string input the user typed.
**
** OUTPUTS:     BOOLEAN : TRUE iff the input is a valid text input.
**
************************************************************************/

BOOLEAN ValidTextInput(STRING input)
{
  return(input[0] <= '9' && input[0] >= '1');
}

/************************************************************************
**
** NAME:        ConvertTextInputToMove
**
** DESCRIPTION: Convert the string input to the internal move representation.
**
** INPUTS:      STRING input : The string input the user typed.
**
** OUTPUTS:     MOVE : The move corresponding to the user's input.
**
************************************************************************/

MOVE ConvertTextInputToMove(STRING input)
{
  return((MOVE) input[0] - '1'); /* user input is 1-9, our rep. is 0-8 */
}

/************************************************************************
**
** NAME:        PrintMove
**
** DESCRIPTION: Print the move in a nice format.
**
** INPUTS:      MOVE *theMove         : The move to print.
**
************************************************************************/

void PrintMove(MOVE theMove)
{
    STRING str = MoveToString( theMove );
    printf( "%s", str );
    SafeFree( str );
}


/************************************************************************
**
** NAME:        MoveToString
**
** DESCRIPTION: Returns the move as a STRING
**
** INPUTS:      MOVE *move         : The move to put into a string.
**
************************************************************************/

STRING MoveToString (MOVE theMove)
{
  STRING m = (STRING) SafeMalloc( 3 );
  /* The plus 1 is because the user thinks it's 1-9, but MOVE is 0-8 */
  sprintf( m, "%d", theMove + 1);

  return m;
}


BlankOX* PositionToBlankOX(POSITION position)
{
	char* board = (char *) SafeMalloc(BOARDSIZE * sizeof(char));
	return (BlankOX *) generic_unhash(position, board);
}

POSITION BlankOXToPosition(BlankOX* board, BlankOX turn)
{
	POSITION position = generic_hash((char*)board, (turn == x ? 1 : 2));
	if(board != NULL)
		SafeFree(board);
	return position;
}

BOOLEAN ThreeInARow(BlankOX* board, int a, int b, int c)
{
  return(board[a] == board[b] &&
		 board[b] == board[c] &&
		 board[c] != Blank );
}

BOOLEAN AllFilledIn(BlankOX* board)
{
  BOOLEAN answer = TRUE;
  int i;

  for(i = 0; i < BOARDSIZE; i++)
    answer &= (board[i] == o || board[i] == x);

  return(answer);
}

BlankOX WhoseTurn(POSITION position)
{
	if (whoseMove(position) == 1)
		return x;
	else return o;
}

int NumberOfOptions()
{
  return 2 ;
}

int getOption()
{
  int option = 0;
  option += gStandardGame;
  option *= 2;
  option += gSymmetries;
  return option+1;
}

void setOption(int option)
{
    option -= 1;
    gSymmetries = option % 2;
    option /= 2;
    gStandardGame = option;
}


//TIER GAMESMAN

void SetupTierStuff() {
	// gUsingTierGamesman
	gUsingTierGamesman = TRUE;
	// gTierSolveList
	gTierSolveListPtr = NULL;
	int tier;
	for (tier = BOARDSIZE; tier >= 0; tier--) { // 10 tiers, 0 through 9
	  gTierSolveListPtr = CreateTierlistNode(tier, gTierSolveListPtr);
	} // solve list = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }
	// All other function pointers
	gInitializeHashWindowFunPtr		= &InitializeHashWindow;
	gTierChildrenFunPtr				= &TierChildren;
	gPositionToTierFunPtr			= &PositionToTier;
	gPositionToTierPositionFunPtr	= &PositionToTierPosition;
	gGenerateUndoMovesToTierFunPtr	= &GenerateUndoMovesToTier;
	gUnDoMoveFunPtr					= &UnDoMove;
	// Tier-Specific Hashes
	int piecesArray[10] = { o, 0, 0, x, 0, 0, Blank, 0, 0, -1 };
	int piecesOnBoard;
	for (tier = 0; tier <= BOARDSIZE; tier++) {
		piecesOnBoard = BOARDSIZE - tier;
		// Os = piecesOnBoard / 2
		piecesArray[1] = piecesArray[2] = piecesOnBoard/2;
		// Xs = piecesOnBoard / 2 (+1 if tier % 2 == 0, else +0)
		piecesArray[4] = piecesArray[5] = piecesOnBoard/2 + (tier%2 == 0 ? 1 : 0);
		// Blanks = tier
		piecesArray[7] = piecesArray[8] = tier;
		// make the hashes
		generic_hash_init(BOARDSIZE, piecesArray, NULL);
		if (tier == 0) // since we can't discard contexts, I use this:
			Tier0Context = generic_hash_cur_context();
	}
}

// All Hash Windows include the TIER argument and the tier below it.
// Except for Tier 0, which just includes itself.
POSITION InitializeHashWindow(TIER tier, POSITION position) {
	BlankOX *board, turn;
	if (position != kBadPosition) {
		board = PositionToBlankOX(position);
		turn = WhoseTurn(position);
	}
	int piecesArray[10] = { o, 4, 4, x, 5, 5, Blank, 0, 0, -1 };
	if (tier != 0) {
		int piecesOnBoard = BOARDSIZE-tier;
		// Os; min = piecesOnBoard / 2,
		// 		max = piecesOnBoard / 2 (+1 if tier % 2 == 0, else +0)
		piecesArray[1] = piecesOnBoard/2;
		piecesArray[2] = piecesOnBoard/2 + (tier%2 == 0 ? 1 : 0);
		// Xs; min = piecesOnBoard / 2 (+1 if tier % 2 == 0, else +0),
		//		max = piecesOnBoard / 2 + 1
		piecesArray[4] = piecesOnBoard/2 + (tier%2 == 0 ? 1 : 0);
		piecesArray[5] = piecesOnBoard/2 + 1;
		// Blanks; min = tier-1, max = tier
		piecesArray[7] = tier-1;
		piecesArray[8] = tier;
	}
	gNumberOfPositions = generic_hash_init(BOARDSIZE, piecesArray, NULL);
	HashWindowContext = generic_hash_cur_context();
	if (position != kBadPosition)
		return BlankOXToPosition(board, turn);
	else return 0;
}

// Tier = Pieces left to place. So a tier's child is always
// itself - 1. Except for Tier 0, of course.
TIERLIST* TierChildren(TIER tier) {
	TIERLIST* tierlist = NULL;
	if (tier != 0)
		tierlist = CreateTierlistNode(tier-1, tierlist);
	return tierlist;
}

// Tier = Pieces left to place. So count the pieces on the
// board and minus it from BOARDSIZE(a.k.a. 9).
TIER PositionToTier(POSITION position) {
	BlankOX* board = PositionToBlankOX(position);
	BlankOX turn = WhoseTurn(position);
	int i, piecesOnBoard = 0;
	for (i = 0; i < BOARDSIZE; i++)
		if (board[i] != Blank)
			piecesOnBoard++;
	if (board != NULL)
		SafeFree(board);
	if ((piecesOnBoard % 2 == 0 && turn == x) ||
		(piecesOnBoard % 2 == 1 && turn == o))
		return BOARDSIZE - piecesOnBoard;
	return kBadTier;
}

// Switch to the correct hash initialized in SetupTierStuff, translate,
// switch back to the hash window, and return.
TIERPOSITION PositionToTierPosition(POSITION position, TIER tier) {
	BlankOX *board = PositionToBlankOX(position),
			turn = WhoseTurn(position);
	generic_hash_context_switch(Tier0Context+tier);
	TIERPOSITION tierPos;
	if (position == kBadPosition)
		tierPos = generic_hash_max_pos();
	else tierPos = BlankOXToPosition(board, turn);
	generic_hash_context_switch(HashWindowContext);
	return tierPos;
}

// Basically the OPPOSITE of GenerateMoves: look for the
// pieces that opponent has on the board, and those are the undomoves.
// Also, ALWAYS generate to tier one above it.
UNDOMOVELIST* GenerateUndoMovesToTier(POSITION position, TIER tier) {
	BlankOX* board = PositionToBlankOX(position);

	int i, piecesOnBoard = 0;
	UNDOMOVELIST* undomoves = NULL;
	BlankOX turn = (WhoseTurn(position) == x ? o : x);
	for (i = BOARDSIZE-1; i >= 0; i--) {
		if (board[i] != Blank) // this is all in leu of calling
			piecesOnBoard++; //gPositionToTier (saves memory/time)
		if (board[i] == turn) //It's opposing player's turn
			undomoves = CreateUndoMovelistNode(i, undomoves);
	}
	if (board != NULL)
  		SafeFree(board);
  	// Check TIER, should be one above current tier
  	if ((BOARDSIZE - piecesOnBoard) + 1 != tier) {
		FreeUndoMoveList(undomoves); // since it won't be used
		return NULL;
	}
  	return undomoves;
}

// UNDOMOVE = Just like a MOVE (0-8) but it tells
// where to TAKE a piece rather than PLACE it.
POSITION UnDoMove(POSITION position, UNDOMOVE undomove) {
    BlankOX* board = PositionToBlankOX(position);
    board[undomove] = Blank;
    return BlankOXToPosition(board, (WhoseTurn(position) == x ? o : x));
}