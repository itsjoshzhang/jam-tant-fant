// Alex Wallisch
// $log$

/*
 * The above lines will include the name and log of the last person
 * to commit this file to CVS
 */

/************************************************************************
**
** NAME:        mqland.c
**
** DESCRIPTION: Queensland
**
** AUTHOR:      Steven Kusalo, Alex Wallisch
**
** DATE:        2004-09-13
**
** UPDATE HIST: 2004-10-08      Partially wrote GameSpecificMenu
**		2004-10-03      Wrote Primitive
**				Wrote ValidTextInput
**				Wrote ConvertTextInputToMove
**                              Wrote PrintComputersMove
**                              Wrote PrintMove
**              2004-10-02:	Wrote GetMoveList
**		2004-09-27:    	Wrote vcfg
**				Wrote DoMove
**		2004-09-26:	Wrote InitializeBoard
**		2004-09-25:	Wrote PrintPosition
**
**************************************************************************/

/*************************************************************************
**
** Everything below here must be in every game file
**
**************************************************************************/

#include <stdio.h>
#include "gamesman.h"
#include "hash.h"
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>


/*************************************************************************
**
** Game-specific constants
**
**************************************************************************/

STRING   kGameName            = "Queensland"; /* The name of your game */
STRING   kAuthorName          = "Steven Kusalo, Alex Wallisch"; /* Your name(s) */
STRING   kDBName              = "qland"; /* The name to store the database under */

BOOLEAN  kPartizan            = TRUE ; /* A partizan game is a game where each player has different moves from the same board (chess - different pieces) */
BOOLEAN  kGameSpecificMenu    = TRUE ; /* TRUE if there is a game specific menu. FALSE if there is not one. */
BOOLEAN  kTieIsPossible       = TRUE ; /* TRUE if a tie is possible. FALSE if it is impossible.*/
BOOLEAN  kLoopy               = FALSE ; /* TRUE if the game tree will have cycles (a rearranger style game). FALSE if it does not.*/

BOOLEAN  kDebugMenu           = TRUE ; /* TRUE only when debugging. FALSE when on release. */
BOOLEAN  kDebugDetermineValue = TRUE ; /* TRUE only when debugging. FALSE when on release. */

POSITION gNumberOfPositions   =  0; /* The number of total possible positions | If you are using our hash, this is given by the hash_init() function*/
POSITION gInitialPosition     =  0; /* The initial hashed position for your starting board */
POSITION kBadPosition         = -1; /* A position that will never be used */

/* 
 * Help strings that are pretty self-explanatory
 * Strings than span more than one line should have backslashes (\) at the end of the line.
 */

STRING kHelpGraphicInterface =
"Not written yet";

STRING   kHelpTextInterface    =
"On your turn, use the LEGEND to choose the starting position and ending position of a piece that you want to move (or 0 if you don't want to move). Then enter an empty position where you want to place your piece."; 

STRING   kHelpOnYourTurn =
"If you want to move a piece, type the position of the piece and the position you want to move it to (e.g. \"3 1\").  Type 0 if you don't want to move.  Then, type the number of an empty position where you want to place a new piece (e.g. \"4\").  A complete move will look something like this: \"3 1 4\"";

STRING   kHelpStandardObjective =
"When all pieces are on the board, the game ends.  Any two pieces of your color that are connected by a straight (horizontal, vertical, or diagonal), unbroken line score one point for each empty space they cover.  You win if you score MORE points than your opponent.";

STRING   kHelpReverseObjective =
"When all pieces are on the board, the game ends.  Any two pieces of your color that are connected by a straight (horizontal, vertical, or diagonal), unbroken line score one point for each empty space they cover.  You win if you score MORE points than your opponent.";

STRING   kHelpTieOccursWhen =
"A tie occurs when each player has played all their pieces and have the same number of points.";

STRING   kHelpExample =
"";


/*************************************************************************
**
** #defines and structs
**
**************************************************************************/
#define BLANK '.'
#define WHITE 'X'
#define BLACK 'O'

#define MAX_HEIGHT 15
#define MIN_HEIGHT 3
#define MAX_WIDTH 15
#define MIN_WIDTH 3

#define pieceat(B, x, y) ((B)[(y) * width + (x)])
#define get_location(x, y) ((y) * width + (x))
#define get_x_coord(location) ((location) % width)
#define get_y_coord(location) ((location) / width)

/* This represents a move as being source*b^2 + dest*b +place 
 * where b is the size of the board, i.e. width*height.
 */
#define get_move_source(move) ((move) / (width*width*height*height))
#define get_move_dest(move) (((move) % (width*width*height*height)) / (width*height))
#define get_move_place(move) ((move) % (width*height))
#define set_move_source(move, source) ((move) += ((source)*width*width*height*height)) 
#define set_move_dest(move, dest) ((move)+= ((dest)*width*height))
#define set_move_place(move, place) ((move) += (place))

/*************************************************************************
**
** Global Variables
**
*************************************************************************/

/* Default values, can be changed in GameSpecific Menu */
int height = 4;
int width = 4;
int numpieces = 4;
enum rules_for_sliding {MUST_SLIDE, MAY_SLIDE, NO_SLIDE} slide_rules = MAY_SLIDE;
BOOLEAN scoreDiagonal = TRUE;
BOOLEAN scoreStraight = TRUE;
BOOLEAN moveDiagonal = TRUE;
BOOLEAN moveStraight = TRUE;

/*************************************************************************
**
** Function Prototypes
**
*************************************************************************/

/* External */
extern GENERIC_PTR	SafeMalloc ();
extern void		SafeFree ();

int vcfg(int* this_cfg);
int next_player(POSITION position);

int countPieces(char *board, char piece);
int scoreBoard(char *board, char player);
void ChangeBoardSize();
void ChangeNumPieces();
void ChangeScoringSystem();
void ChangeMovementRules();
MOVELIST* add_all_place_moves(int source_pos, int dest_pos, char* board, MOVELIST* moves);
BOOLEAN valid_move(int source_pos, int dest_pos, char* board);

/*************************************************************************
**
** Global Database Declaration
**
**************************************************************************/

extern VALUE     *gDatabase;


/************************************************************************
**
** NAME:        InitializeGame
**
** DESCRIPTION: Prepares the game for execution.
**              Initializes required variables.
**              Sets up gDatabase (if necessary).
** 
************************************************************************/

void InitializeGame () {
	int i, j;
	int pieces_array[10] = {BLANK, 0, width * height, WHITE, 0, numpieces, BLACK, 0, numpieces, -1 };
	char* board = (char*)malloc(sizeof(char) * width * height);
	
	gNumberOfPositions = generic_hash_init(width * height, pieces_array, vcfg);
	for (j = 0; j < height; j++) {
		for (i = 0; i < width; i++) {
			pieceat(board, i, j) = BLANK;
		}
	}
	gInitialPosition = generic_hash(board, 1);
}


/************************************************************************
**
** NAME:        GenerateMoves
**
** DESCRIPTION: Creates a linked list of every move that can be reached
**              from this position. Returns a pointer to the head of the
**              linked list.
** 
** INPUTS:      POSITION position : Current position for move
**                                  generation.
**
** OUTPUTS:     (MOVELIST *)      : A pointer to the first item of
**                                  the linked list of generated moves
**
** CALLS:       MOVELIST *CreateMovelistNode();
**
************************************************************************/

MOVELIST *GenerateMoves (POSITION position)
{
	MOVELIST *moves = NULL;
    
	/* Use CreateMovelistNode(move, next) to 'cons' together a linked list */
	int player;
	int s, d; /* source, dest */
	BOOLEAN validDestination;
	int i, j;
	char* board = (char*) SafeMalloc(sizeof(char) * width * height);
	char players_piece;
	MOVE move;
   
	player = next_player(position);
	players_piece = (player == 1 ? WHITE : BLACK);
	board = generic_unhash(position, board);
	/* Check moves that don't slide a piece from SOURCE to DEST */
	if (slide_rules != MUST_SLIDE) {
		moves = add_all_place_moves(0, 0, board, moves);
	}
	
	if (slide_rules != NO_SLIDE) {
		for (s = width * height - 1; s >= 0; s--) {
			if (pieceat(board, get_x_coord(s), get_y_coord(s)) == players_piece) {
				for (d = width * height - 1; d >= 0; d--) {
					if (valid_move(s,d, board)) {
						moves = add_all_place_moves(s, d, board, moves);
					}
				}
			}
		}
	}
		
	SafeFree(board);
	return moves;
}
	




/************************************************************************
**
** NAME:        DoMove
**
** DESCRIPTION: Applies the move to the position.
** 
** INPUTS:      POSITION position : The old position
**              MOVE     move     : The move to apply to the position
**
** OUTPUTS:     (POSITION)        : The position that results from move
**
** CALLS:       Some Board Hash Function
**              Some Board Unhash Function
**
*************************************************************************/

POSITION DoMove (POSITION position, MOVE move) {
	/* This function does ZERO ZILCH ABSOLUTELY-NONE-AT-ALL error checking.  move had better be valid */
	
	int player = next_player(position);
	char players_piece = player == 1 ? WHITE : BLACK;
	POSITION new;
	char* board = (char*) SafeMalloc(sizeof(char) * width * height);
	int source = get_move_source(move);
	int dest = get_move_dest(move);
	int place = get_move_place(move);
	
	board = generic_unhash(position, board);

	if (source != dest) {
	    /* Place a new piece at the destination of the SLIDE move */
	    pieceat(board, get_x_coord(dest), get_y_coord(dest)) = players_piece;
	
	    /* Erase the piece from the source of the SLIDE move */
	    pieceat(board, get_x_coord(source), get_y_coord(source)) = BLANK;
	}
	
	/* Place a new piece at the location of the PLACE move */
	pieceat(board, get_x_coord(place), get_y_coord(place)) = players_piece;

	new = generic_hash(board, player);

	SafeFree(board);
	return new;
}


/************************************************************************
**
** NAME:        Primitive
**
** DESCRIPTION: Returns the value of a position if it fulfills certain
**              'primitive' constraints.
**
**              Example: Tic-tac-toe - Last piece already placed
**
**              Case                                  Return Value
**              *********************************************************
**              Current player sees three in a row    lose
**              Entire board filled                   tie
**              All other cases                       undecided
** 
** INPUTS:      POSITION position : The position to inspect.
**
** OUTPUTS:     (VALUE)           : one of
**                                  (win, lose, tie, undecided)
**
** CALLS:       None              
**
************************************************************************/

VALUE Primitive (POSITION position) {
    char* board = (char*) SafeMalloc(sizeof(char) * width * height);
    board = generic_unhash(position, board);
    if (countPieces(board,WHITE) != numpieces || countPieces(board, BLACK) != numpieces) {
	SafeFree(board);
	return undecided;
    } else {
	int blackscore = scoreBoard(board, BLACK);
	int whitescore = scoreBoard(board, WHITE);
	SafeFree(board);
	if (whitescore == blackscore) {
	    return tie;
	} else if (whitescore > blackscore) {
	    return gStandardGame ? win : lose;
	} else {
	    return gStandardGame ? lose : win;
	}
    }
}

/************************************************************************
**
** NAME:        PrintPosition
**
** DESCRIPTION: Prints the position in a pretty format, including the
**              prediction of the game's outcome.
** 
** INPUTS:      POSITION position    : The position to pretty print.
**              STRING   playersName : The name of the player.
**              BOOLEAN  usersTurn   : TRUE <==> it's a user's turn.
**
** CALLS:       Unhash()
**              GetPrediction()      : Returns the prediction of the game
**
************************************************************************/

void PrintPosition (POSITION position, STRING playersName, BOOLEAN usersTurn) {
    char *board = (char*) SafeMalloc(sizeof(char) * width * height);
    board= generic_unhash(position, board);
    	int i, j;
    	
	if (width < 4) {
		printf("  Queensland!\n");
	}
	else {
		for (i = 0; i < width - 1; i++)
		{
			printf(" ");
		}
		printf("Queensland!\n");
	}
	
	printf("/");						/* Top row */
	for (i = 0; i < (2 * width + 7); i++) {			/* /===============\ */
		printf("=");
	}
	printf("\\\n");
	
	printf("|    ");					/* Second row */
	for (i = 'a'; i < width + 'a'; i++) {			/* |    a b c d    | */
		printf("%c ", i);
	}
	printf("   |\n");
								/* Third row */
	printf("|  /");						/* |  /---------\  | */
    	for (i = 0; i < (2*width+1); i++) {
   		printf("-");
	}
	printf("\\  |\n");
		
	
	for (j = 0; j < height; j++) {				/* Body of board */
		printf("| %d", j);
		/* Right now, we do not print stock of remaining pieces. If we did, O's would go here. */
		printf("| ");
		for (i = 0; i < width; i++) {
			switch(pieceat(board, i, j)) {
				case BLANK:
					printf("%c ", BLANK);
					break;
				case WHITE:
					printf("%c ", WHITE);
					break;
				case BLACK:
					printf("%c ", BLACK);
					break;
				default:
				    BadElse("PrintPosition");
			}
		}
		printf("|  ");
		/* Right now, we do not print stock of remaining pieces.  If we did, X's would go here. */
		printf("|\n");
	}
	printf("|  \\");					/* Third-from-bottom row */
    	for (i = 0; i < (2*width+1); i++) {			/* |  \---------/  | */
   		printf("-");
	}
	printf("/  |\n");
	
	printf("|");						/* Second-from-bottom row */
	for (i = 0; i < (2 * width + 7); i++) {			/* |               | */
	/* If we had a status display at the bottom of the board, it would go here. */
		printf(" ");
	}
	printf("|\n");
	
	printf("\\");						/* Bottom row */
	for (i = 0; i < (2 * width + 7); i++) {			/* \===============/ */
		printf("=");
	}
	printf("/\n");
	printf("%s\n", GetPrediction(position, playersName, usersTurn));
	SafeFree(board);
}
	

/************************************************************************
**
** NAME:        PrintComputersMove
**
** DESCRIPTION: Nicely formats the computers move.
** 
** INPUTS:      MOVE    computersMove : The computer's move. 
**              STRING  computersName : The computer's name. 
**
************************************************************************/

void PrintComputersMove (MOVE computersMove, STRING computersName)
{
    printf("%8s's move: ", computersName);
    PrintMove(computersMove);
    printf("\n");
}


/************************************************************************
**
** NAME:        PrintMove
**
** DESCRIPTION: Prints the move in a nice format.
** 
** INPUTS:      MOVE move         : The move to print. 
**
************************************************************************/

void PrintMove (MOVE move)
{
    if (get_move_source(move) == 0 && get_move_dest(move) == 0) {
	printf("[%c%d]", 
		get_x_coord(get_move_place(move)) + 'a', 
		get_y_coord(get_move_place(move)));
    }
    else {
	printf("[%c%d %c%d %c%d]",
		get_x_coord(get_move_source(move)) + 'a', 
		get_y_coord(get_move_source(move)),
		get_x_coord(get_move_dest(move)) + 'a', 
		get_y_coord(get_move_dest(move)),
		get_x_coord(get_move_place(move)) + 'a', 
		get_y_coord(get_move_place(move)));
    }
}


/************************************************************************
**
** NAME:        GetAndPrintPlayersMove
**
** DESCRIPTION: Finds out if the player wishes to undo, abort, or use
**              some other gamesman option. The gamesman core does
**              most of the work here. 
**
** INPUTS:      POSITION position    : Current position
**              MOVE     *move       : The move to fill with user's move. 
**              STRING   playersName : Current Player's Name
**
** OUTPUTS:     USERINPUT          : One of
**                                   (Undo, Abort, Continue)
**
** CALLS:       USERINPUT HandleDefaultTextInput(POSITION, MOVE*, STRING)
**                                 : Gamesman Core Input Handling
**
************************************************************************/

USERINPUT GetAndPrintPlayersMove (POSITION position, MOVE *move, STRING playersName)
{
    USERINPUT input;
    USERINPUT HandleDefaultTextInput();
    
    for (;;) {
        /***********************************************************
         * CHANGE THE LINE BELOW TO MATCH YOUR MOVE FORMAT
         ***********************************************************/
	printf("%8s's move [(undo)/(SOURCE DESTINATION PLACE)/(PLACE)] : ", playersName);
	
	input = HandleDefaultTextInput(position, move, playersName);
	
	if (input != Continue)
		return input;
    }

    /* NOTREACHED */
    return Continue;
}


/************************************************************************
**
** NAME:        ValidTextInput
**
** DESCRIPTION: Rudimentary check to check if input is in the move form
**              you are expecting. Does not check if it is a valid move.
**              Only checks if it fits the move form.
**
**              Reserved Input Characters - DO NOT USE THESE ONE CHARACTER
**                                          COMMANDS IN YOUR GAME
**              ?, s, u, r, h, a, c, q
**                                          However, something like a3
**                                          is okay.
** 
**              Example: Tic-tac-toe Move Format : Integer from 1 to 9
**                       Only integers between 1 to 9 are accepted
**                       regardless of board position.
**                       Moves will be checked by the core.
**
** INPUTS:      STRING input : The string input the user typed.
**
** OUTPUTS:     BOOLEAN      : TRUE if the input is a valid text input.
**
************************************************************************/

BOOLEAN ValidTextInput (STRING input) {
   

    /* FILL IN */
    
    return TRUE;
}


/************************************************************************
**
** NAME:        ConvertTextInputToMove
**
** DESCRIPTION: Converts the string input your internal move representation.
**              Gamesman already checked the move with ValidTextInput
**              and ValidMove.
** 
** INPUTS:      STRING input : The VALID string input from the user.
**
** OUTPUTS:     MOVE         : Move converted from user input.
**
************************************************************************/

MOVE ConvertTextInputToMove (STRING input) {
	
	int first, second, third;	
	char* curr = input;
	MOVE move = 0;
	
	/* Eat leading whitespace */
	while (isspace((int)*curr)) curr++;
	
	/* Turn the next two chars into an position
	 * on the board */
	first = XYToNumber(curr);
	curr +=2;
	
	/* Eat more whitespace */
	while (isspace((int)*curr)) curr++;
	
	/* Check if we are at the end of the string. If
	 * so then this is a place only.*/
	if (!(*curr)) {
		set_move_source(move, 0);
		set_move_dest(move, 0);
		set_move_place(move, first);
	} else {
		/* Turn the next two chars into an position
		 * on the board */
		second = XYToNumber(curr);
		curr +=2;
	
		/* Eat more whitespace */
		while (isspace((int)*curr)) curr++;
	
		/* Turn the next two chars into an position
		 * on the board */
		third = XYToNumber(curr);
		curr +=2;
     
		set_move_source(move, first);
		set_move_dest(move, second);
		set_move_place(move, third);
	}

	return move;
}


/************************************************************************
**
** NAME:        GameSpecificMenu
**
** DESCRIPTION: Prints, receives, and sets game-specific parameters.
**
**              Examples
**              Board Size, Board Type
**
**              If kGameSpecificMenu == FALSE
**                   Gamesman will not enable GameSpecificMenu
**                   Gamesman will not call this function
** 
**              Resets gNumberOfPositions if necessary
**
************************************************************************/

void GameSpecificMenu () {
    char c;
    while (TRUE) {
	printf("\n\nGame Options:\n\n");
	printf("\ts)\tchange the (S)ize of the board\n");
	printf("\tp)\tchange the number of (P)ieces\n");
	printf("\tc)\tchange the s(C)oring system of the game\n");
	printf("\tm)\tchange the (M)ovement rules\n");
	printf("\tb)\t(B)ack to the main menu\n");
	printf("\nSelect an option:  ");
	while (!isalpha(c = getc(stdin)));
	c = tolower(c);
	switch (c) {
	case 's':
	    ChangeBoardSize();
	    break;
	case 'p':
	    ChangeNumPieces();
	    break;
	case 'c':
	    ChangeScoringSystem();
	    break;
	case 'm':
	    ChangeMovementRules();
	    break;
	case 'b':
	    return;
	default:
	    printf("Invalid option. Please try again.\n");
    }
  }    
}
 
/************************************************************************
**
** NAME:        SetTclCGameSpecificOptions
**
** DESCRIPTION: Set the C game-specific options (called from Tcl)
**              Ignore if you don't care about Tcl for now.
** 
************************************************************************/

void SetTclCGameSpecificOptions (int options[])
{
    
}
  
  
/************************************************************************
**
** NAME:        GetInitialPosition
**
** DESCRIPTION: Called when the user wishes to change the initial
**              position. Asks the user for an initial position.
**              Sets new user defined gInitialPosition and resets
**              gNumberOfPositions if necessary
** 
** OUTPUTS:     POSITION : New Initial Position
**
************************************************************************/

POSITION GetInitialPosition ()
{
    return 0;
}


/************************************************************************
**
** NAME:        NumberOfOptions
**
** DESCRIPTION: Calculates and returns the number of variants
**              your game supports.
**
** OUTPUTS:     int : Number of Game Variants
**
************************************************************************/

int NumberOfOptions ()
{
    return 0;
}


/************************************************************************
**
** NAME:        getOption
**
** DESCRIPTION: A hash function that returns a number corresponding
**              to the current variant of the game.
**              Each set of variants needs to have a different number.
**
** OUTPUTS:     int : the number representation of the options.
**
************************************************************************/

int getOption ()
{
    return 0;
}


/************************************************************************
**
** NAME:        setOption
**
** DESCRIPTION: The corresponding unhash function for game variants.
**              Unhashes option and sets the necessary variants.
**
** INPUT:       int option : the number representation of the options.
**
************************************************************************/

void setOption (int option)
{
    
}


/************************************************************************
**
** NAME:        DebugMenu
**
** DESCRIPTION: Game Specific Debug Menu (Gamesman comes with a default
**              debug menu). Menu used to debug internal problems.
**
**              If kDebugMenu == FALSE
**                   Gamesman will not display a debug menu option
**                   Gamesman will not call this function
** 
************************************************************************/

void DebugMenu ()
{
    
}


/************************************************************************
**
** Everything specific to this module goes below these lines.
**
** Things you want down here:
** Move Hasher
** Move Unhasher
** Any other function you deem necessary to help the ones above.
** 
************************************************************************/

int vcfg(int *this_cfg) {
	/* If number of BLACKs is equal to or one less than number of WHITEs then this configuration is valid. */
	return this_cfg[2] == this_cfg[1] || this_cfg[2] + 1 == this_cfg[1];
}

int next_player(POSITION position) {
	char* board = (char*)SafeMalloc(sizeof(char) * width * height);
	int numWhite, numBlack;
	
	board = generic_unhash(position, board);
	numBlack = countPieces(board, BLACK);
	numWhite = countPieces(board, WHITE);
	SafeFree (board);
	return (numWhite > numBlack ? 2 : 1);
}

/*
 * This function tallys and returns how many points
 * the given player (the argument should either be
 * BLACK or WHITE) gets for this board.
 */
 int scoreBoard(char *board, char player) {
     int x;
     int y;
     int score = 0;
  
     for (x = 0; x < width; x++) {
	 for (y = 0; y < height; y++) { 
	     if(pieceat(board, x, y) == player) {
		 int i; /* used when x varies */
		 int j;	/* used when y varies */ 

		 if (scoreStraight) {

		     /* count any horizontal lines going to the right */
		     for (i = x+1; i < width && pieceat(board, i, y) == BLANK; i++) { }
		     if (i < width && pieceat(board, i, y) == player) {
			 score += (i-x-1);
		     }	

		     /* count any vertical lines going down */
		     for (j = y+1; j < height && pieceat(board, x, j) == BLANK; j++) { }
		     if (j < height && pieceat(board, x, j) == player) {
			 score += (j-y-1);
		     }
		 }

		 if (scoreDiagonal) {
		     
		     /* count any diagonal lines going up and to the right */
		     for (i = x+1, j = y-1; i < width && j >= 0 && pieceat(board, i, j) == BLANK; i++, j--) { }
		     if (i < width && j >= 0 && pieceat(board, i, j) == player) {
			 score += (i-x-1);
		     }

		     /* count any diagonal lines going down and to the right */
		     for (i = x+1, j = y+1; i < width && j < height && pieceat(board, i, j) == BLANK; i++, j++) { }
		     if (i < width && j < height && pieceat(board, i, j) == player) {
			 score += (i-x-1); 
		     }	
		 }
	     }
	 }
     }
     return score;
 }

/* Counts the number of pieces of the given type
 * that are on the board.
 */
int countPieces(char *board, char piece) {
    int x;
    int y;
    int count = 0;
    for (x = 0; x < width; x++) {
	for (y = 0; y < height; y++) {
	    if (board[get_location(x,y)] == piece) {
		count++;
	    }
	}
    }
    return count;
}

/* Allows the user to change the width and height
 * of the board.
 */
void ChangeBoardSize() {
    int newWidth, newHeight;
    while(TRUE) {
	printf("\n\nEnter a new size for the board (width height): ");
	scanf("%d %d", &newWidth, &newHeight);
	if (newWidth <= MAX_WIDTH && newHeight <= MAX_HEIGHT && newWidth >= MIN_WIDTH & newHeight >= MIN_HEIGHT) {
	    height = newHeight;
	    width = newWidth;
	    printf("\nThe board is now %d by %d.\n", width, height);
	    return;
	}
	printf("Board must be between %d by %d and %d by %d.  Please try again.\n",
	       MIN_WIDTH, MIN_HEIGHT,
	       MAX_WIDTH, MAX_HEIGHT);
    }
}

/* Allows the user to change the number of pieces
 * each player starts with.
 */
void ChangeNumPieces() {
    int newAmount;
    while(TRUE) {
	printf("\n\nEnter the number of pieces each team starts with: ");
	scanf("%d", &newAmount);
	if (newAmount <= width && newAmount >= 1) {
	    numpieces = newAmount;
	    printf("\nNumber of pieces changed to %d\n", numpieces);
	    return;
	}
	printf("Number must be between 1 and %d.  Please try again.\n", width);
    }
}

/* Allows the user to change how the board
 * is scored.
 */
void ChangeScoringSystem() {
    char c;
    while(TRUE) {
	printf("\n\nScoring Options (current is %s):\n\n",
	       scoreStraight ? (scoreDiagonal ? "SCORE BOTH" : "SCORE STRAIT") : "SCORE DIAGONAL");
	printf("\ts)\tcount only (S)traight lines (horizonal and vertical)\n");
	printf("\td)\tcount only (D)iagonal lines\n");
	printf("\to)\tcount b(O)th straight and diagonal lines\n");
	printf("\tb)\t(B)ack to the previous menu\n");
	printf("\nSelect an option: ");
	while (!isalpha(c = getc(stdin)));
	c = tolower(c);
	switch(c) {
	case 's':
	    scoreStraight = TRUE;
	    scoreDiagonal = FALSE;
	    ChangeScoringSystem();
	    return;
	case 'd':
	    scoreStraight = FALSE;
	    scoreDiagonal = TRUE;
	    ChangeScoringSystem();
	    return;
	case 'o':
	    scoreStraight = TRUE;
	    scoreDiagonal = TRUE;
	    ChangeScoringSystem();
	    return;
	case 'b':
	    return;
	default:
	    printf("Invalid option. Please try again");
	}
    }
}

/* Allows the user to change the movement rules */
void ChangeMovementRules() {
    char c;
    while (TRUE) {
	printf("\n\nMovement options:\n\n");
	printf("\tm)\tToggle the way pieces can (M)ove (currently pieces move like %s)\n",
	       moveDiagonal ? (moveStraight ? "QUEENS" : "BISHOPS") : "ROOKS");
	printf("\ts)\tToggle the (S)tructure of a move (currently a player %s move a piece before placing a piece)\n",
	       slide_rules == MAY_SLIDE ? "MAY" : (slide_rules == MUST_SLIDE ? "MUST" : "CANNOT"));
	printf("\tb)\t(B)ack to the previous menu\n");
	printf("\nSelect an option: ");
	while (!isalpha(c = getc(stdin)));
	c = tolower(c);
	switch(c) {
	case 'm':
	    if (moveDiagonal && moveStraight) {
		moveDiagonal = FALSE;
		moveStraight = TRUE;
	    } else if (moveStraight) {
		moveDiagonal = TRUE;
		moveStraight = FALSE;
	    } else {
		moveDiagonal = TRUE;
		moveStraight = TRUE;
	    }
	    ChangeMovementRules();
	    return;
	case 's':
	    if (slide_rules == MAY_SLIDE) {
		slide_rules = MUST_SLIDE;
	    } else if (slide_rules == MUST_SLIDE) {
		slide_rules = NO_SLIDE;
	    } else {
		slide_rules = MAY_SLIDE;
	    }
	    ChangeMovementRules();
	    return;
	case 'b':
	    return;
	default:
	    printf("Invalid option. Please try again");
	}
    }
}

/* Converts a string of two characters to a board position */
int XYToNumber(char* xy) {
	int x, y;
	
	x = tolower(xy[0]) - 'a';
	y = xy[1] - '0';
	return get_location(x, y);
}


MOVELIST* add_all_place_moves(int source_pos, int dest_pos, char* board, MOVELIST* moves) {
	int px, py;
	MOVE move;
	for (px = width - 1; px >= 0; px--) {
		for (py = height - 1; py >= 0; py--) {
			if (pieceat(board, px, py) == BLANK) {
				move = 0;
				set_move_source(move, source_pos);
				set_move_dest(move, dest_pos);
				set_move_place(move, get_location(px, py));
				moves = CreateMovelistNode(move, moves);
			}
		}
	}
	return moves;
}

BOOLEAN valid_move(int source_pos, int dest_pos, char* board) {
	int sx = get_x_coord(source_pos); 
	int sy = get_y_coord(source_pos);
	int dx = get_x_coord(dest_pos);
	int dy = get_y_coord(dest_pos);
	int i, j;
	
	if (pieceat(board, dx, dy) != BLANK) {
		return FALSE;
	}
	else if (sx == dx && sy == dy){
		return FALSE;
	}
	else if ((sx == dx || sy == dy) && !moveStraight) {
		return FALSE;
	}
	else if (sx == dx) {
		for (i = (sy < dy ? sy + 1 : sy - 1); i != dy; (sy < dy ? i++ : i--)) {
			if (pieceat(board, sx, i) != BLANK) {
				return FALSE;
			}
		}
	}
	else if (sy == dy) {
		for (i = (sx < dx ? sx + 1 : sx - 1); i != dx; (sx < dx ? i++ : i--)) {
			if (pieceat(board, i, sy) != BLANK) {
				return FALSE;
			}
		}
	}
	else if ((abs(sx - dx) == abs (sy - dy)) && !moveDiagonal) {
		return FALSE;
	}
	else if (abs(sx - dx) == abs(sy - dy)) { /* Check if (sx, sy) and (dx, dy) are on the same diagonal line */
		for (i = (sx < dx ? sx + 1 : sx - 1), j = (sy < dy ? sy + 1 : sy - 1); i != dx; (sx < dx ? i++ : i--), (sy < dy ? j++ : j--)) {
			if (pieceat(board, i, j) != BLANK) {
				return FALSE;
			}
		}
	}
	else return FALSE;
	return TRUE;
}
