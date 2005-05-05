####################################################
# this is a template for tcl module creation
#
# created by Alex Kozlowski and Peterson Trethewey
# Updated Fall 2004 by Jeffrey Chiang, and others
####################################################

# John's helper procedures

proc avg { x y } {
    return [expr ($x + $y) / 2]
}

proc sum { n } {

    set result 0
    while { $n != 0 } {
	set result [expr $result+$n]
	set n [expr $n-1]
    }
    return $result
}

proc factorial { n } {

    set result 1
    while { $n != 0 } {
	set result [expr $result*$n]
	set n [expr $n-1]
    }
    return $result
}

# John's global vars

set csize 500
set rows 5
set Boardsize [sum $rows]
# set WinningCondition $Standard

# Stuff taken from miceblocks.c

set FALSE 0
set TRUE 1
set Standard 0
set TallyBlocks 1
set TallyThrees 2
set X 99999
set O 9999
set MAX_BOARD_SIZE 6
set MIN_BOARD_SIZE 2
set NUM_OPTIONS 3

#############################################################################
# GS_InitGameSpecific sets characteristics of the game that
# are inherent to the game, unalterable.  You can use this fucntion
# to initialize data structures, but not to present any graphics.
# It is called FIRST, ONCE, only when the player
# starts playing your game, and before the player hits "New Game"
# At the very least, you must set the global variables kGameName
# and gInitialPosition in this function.
############################################################################
proc GS_InitGameSpecific {} {

    ### Set the name of the game
    
    global kGameName
    set kGameName "Ice Blocks"
    
    ### Set the initial position of the board (default 0)

    global gInitialPosition gPosition
    set gInitialPosition 3492116
    set gPosition $gInitialPosition

    global WinningCondition TRUE FALSE Standard TallyBlocks TallyThrees
    set winningString ""
    if { $WinningCondition == $TallyBlocks } {
	set winningString "blocks in a row"
    } else { if { $WinningCondition == $TallyThrees } {
	set winningString "three's in a row"
    } else { set winningString "points (Three points for every string of three and two additional points for every additional block.)" } } 

    ### Set the strings to be used in the Edit Rules

    global kStandardString kMisereString
    set kStandardString "The player with the most $winningString WINS"
    set kMisereString "The player with the most $winningString LOSES"

    ### Set the strings to tell the user how to move and what the goal is.
    ### If you have more options, you will need to edit this section

    global gMisereGame
    if {!$gMisereGame} {
	SetToWinString "To Win: Get the most $winningString"
    } else {
	SetToWinString "To Win: Get the fewest $winningString"
    }
    SetToMoveString "To Move:  You and your opponent are trying to build a pyramid shaped igloo wall out of the blue and clear ice blocks. Players take turn placing one ice block at a time. The block must be next to another block or on top of any two touching blocks."
    
    # Authors Info. Change if desired
    global kRootDir
    global kCAuthors kTclAuthors kGifAuthors
    set kCAuthors "Kevin Duncan, Neil Trotter"
    set kTclAuthors "John Lo"
    set kGifAuthors "$kRootDir/../bitmaps/DanGarcia-310x232.gif"
}


#############################################################################
# GS_NameOfPieces should return a list of 2 strings that represent
# your names for the "pieces".  If your game is some pathalogical game
# with no concept of a "piece", give a name to the game's sides.
# if the game is tic tac toe, this might be a single line: return [list x o]
# This function is called FIRST, ONCE, only when the player
# starts playing the game, and before he hits "New Game"
#############################################################################
proc GS_NameOfPieces {} {

    return [list x o]

}


#############################################################################
# GS_ColorOfPlayers should return a list of two strings, 
# each representing the color of a player.
# If a specific color appears uniquely on one player's pieces,
# it might be a good choice for that player's color.
# In impartial games, both players may share the same color.
# If the game is tic tac toe, this might be the line 
# return [list blue red]
# If the game is nim, this might be the line
# return [list green green]
# This function is called FIRST, ONCE, only when the player
# starts playing the game, and before he clicks "New Game"
# The left player's color should be the first item in the list.
# The right player's color should be second.
#############################################################################
proc GS_ColorOfPlayers {} {

    return [list blue red]
    
}


#############################################################################
# GS_SetupRulesFrame sets up the rules frame;
# Adds widgets to the rules frame that will allow the user to 
# select the variant of this game to play. The options 
# selected by the user should be stored in a set of global
# variables.
# This procedure must initialize the global variables to some
# valid game variant.
# The rules frame must include a standard/misere setting.
# Args: rulesFrame (Frame) - The rules frame to which widgets
# should be added
# Modifies: the rules frame and its global variables
# Returns: nothing
#############################################################################
proc GS_SetupRulesFrame { rulesFrame } {
    global Standard TallyBlocks TallyThrees
    global cv

    set standardRule \
	[list \
	     "What would you like your winning condition to be:" \
	     "Standard" \
	     "Misere" \
	    ]

    set winConditionRule \
	[list \
	     "The winner is the player with the most: " \
	     "Points" \
	     "Blocks in row" \
	     "Three's in row" \
	    ]

#     set boardSizeRule \
# 	[list \
# 	      "How many blocks should be on the bottom row?" \
# 	      "2" \
# 	      "3" \
# 	      "4" \
# 	      "5" \
# 	      "6" \
# 	     ]

    # List of all rules, in some order
    set ruleset [list $standardRule $winConditionRule] 
    # $boardSizeRule

    # Declare and initialize rule globals
    global gMisereGame
    set gMisereGame 0

    global WinningCondition
    set WinningCondition $Standard

#     global rows
#     set rows 5

    # List of all rule globals, in same order as rule list
    set ruleSettingGlobalNames [list "gMisereGame" "WinningCondition"]
    # "rows"

    global kLabelFont
    set ruleNum 0
    foreach rule $ruleset {
	frame $rulesFrame.rule$ruleNum -borderwidth 2 -relief raised
	pack $rulesFrame.rule$ruleNum  -fill both -expand 1
	message $rulesFrame.rule$ruleNum.label -text [lindex $rule 0] \
	    -font $kLabelFont
	pack $rulesFrame.rule$ruleNum.label -side left
	set rulePartNum 0
	foreach rulePart [lrange $rule 1 end] {
	    radiobutton $rulesFrame.rule$ruleNum.p$rulePartNum \
		-text $rulePart \
		-variable [lindex $ruleSettingGlobalNames $ruleNum] \
		-value $rulePartNum -highlightthickness 0 -font $kLabelFont
	    pack $rulesFrame.rule$ruleNum.p$rulePartNum -side left \
		-expand 1 -fill both
	    incr rulePartNum
	}
	incr ruleNum
    } 
}


#############################################################################
# GS_GetOption gets the game option specified by the rules frame
# Returns the option of the variant of the game specified by the 
# global variables used by the rules frame
# Args: none
# Modifies: nothing
# Returns: option (Integer) - the option of the game as specified by 
# getOption and setOption in the module's C code
#############################################################################
proc GS_GetOption { } {
    global csize rows Boardsize
    global WinningCondition gMisereGame MIN_BOARD_SIZE NUM_OPTIONS \
	Standard TallyBlocks TallyThrees TRUE FALSE
    set option [expr $rows - $MIN_BOARD_SIZE]
    set option [expr $option * $NUM_OPTIONS]
    if { $WinningCondition == $TallyBlocks } { 
	set option [expr $option+1] 
    } elseif { $WinningCondition == $TallyThrees } { 
	    set option [expr $option+2] 
    }
    set option [expr $option * 2]
    if { $gMisereGame == $TRUE } { set option [expr $option+1] }
    set option [expr $option+1]
    return $option
#     # TODO: Needs to change with more variants
#     global gMisereGame
#     set option 1
#     set option [expr $option + (1-$gMisereGame)]
#     return $option
}


#############################################################################
# GS_SetOption modifies the rules frame to match the given options
# Modifies the global variables used by the rules frame to match the 
# given game option.
# This procedure only needs to support options that can be selected 
# using the rules frame.
# Args: option (Integer) -  the option of the game as specified by 
# getOption and setOption in the module's C code
# Modifies: the global variables used by the rules frame
# Returns: nothing
#############################################################################
proc GS_SetOption { option } {
    global csize rows Boardsize
    global WinningCondition gMisereGame MIN_BOARD_SIZE NUM_OPTIONS \
	Standard TallyBlocks TallyThrees TRUE FALSE
    set option [expr $option-1]
    set gMisereGame $FALSE
    if { ($option % 2) != 0 } { set gMisereGame $TRUE }
    set option [expr $option / 2]
    set tmp [expr $option % $NUM_OPTIONS]
    if { $tmp == 0 } { 
	set WinningCondition $Standard 
    } elseif { $tmp == 1 } { 
	set WinningCondition $TallyBlocks 
    } elseif { $tmp == 2 } { 
	set WinningCondition $TallyThrees 
    }
    set option [expr $options / $NUM_OPTIONS]
    set rows [expr $option + $MIN_BOARD_SIZE]
    set Boardsize [sum $rows]
    #     # TODO: Needs to change with more variants
    #     global gMisereGame
    #     set option [expr $option - 1]
    #     set gMisereGame [expr 1-($option%2)]
}


#############################################################################
# GS_Initialize is where you can start drawing graphics.  
# Its argument, c, is a canvas.  Please draw only in this canvas.
# You could put an opening animation in this function that introduces the game
# or just draw an empty board.
# This function is called ONCE after GS_InitGameSpecific, and before the
# player hits "New Game"
#############################################################################
proc GS_Initialize { c } {
    global csize rows Boardsize
    set Boardsize [sum $rows]

    # you may want to start by setting the size of the canvas; this line 
    # isn't necessary
    $c configure -width $csize -height $csize

    set cols $rows
    set bsize [expr $csize * .90]
    set width [expr $bsize / $rows]
    set hside $width
    set hoffs [expr $width * .70]
    set xincr [expr $width / 2]
    set yincr [expr .5 * ($hside - $hoffs)]
    set xoffset [expr ($csize - ($rows * $width)) / 2]
    set yoffset [expr ($csize - ($rows * $width)) / 2]
    set cnt 0
    for {set j [expr $rows - 1]} {$j >= 0} {set j [expr $j - 1]} {
	for {set i 0} {$i < $cols} {set i [expr $i + 1]} {
	    set cnt [expr $cnt+1]
	    MakeMoveIndicator $c \
		[avg [expr $i * $width + $xoffset] \
		     [expr ($i+1) * $width + $xoffset]] \
		[avg [expr $j * $hside + $yoffset] \
		     [expr ($j+1) * $hside + $yoffset]] \
		[expr $width / 2] \
		[expr $hside / 2] \
		[expr $hoffs / 2] \
		$cnt
	    MakeXBlock $c \
		[avg [expr $i * $width + $xoffset] \
		     [expr ($i+1) * $width + $xoffset]] \
		[avg [expr $j * $hside + $yoffset] \
		     [expr ($j+1) * $hside + $yoffset]] \
		[expr $width / 2] \
		[expr $hside / 2] \
		[expr $hoffs / 2] \
		$cnt
	    MakeOBlock $c \
		[avg [expr $i * $width + $xoffset] \
		     [expr ($i+1) * $width + $xoffset]] \
		[avg [expr $j * $hside + $yoffset] \
		     [expr ($j+1) * $hside + $yoffset]] \
		[expr $width / 2] \
		[expr $hside / 2] \
		[expr $hoffs / 2] \
		$cnt
# 	    $c create oval [expr $i * $width + $xoffset + .4*$width] \
# 		[expr $j * $width + $yoffset + .4*$width] \
# 		[expr ($i+1) * $width + $xoffset - .4*$width] \
# 		[expr ($j+1) * $width + $yoffset - .4*$width] \
# 		-fill white -outline "" \
# 		-tag [list moveindicators mi-$cnt]
# 	    $c create rect [expr $i * $width + $xoffset] \
# 		[expr $j * $width + $yoffset] \
# 		[expr ($i+1) * $width + $xoffset] \
# 		[expr ($j+1) * $width + $yoffset] \
# 		-fill white -tag X$cnt
# 	    $c create rect [expr $i * $width + $xoffset] \
# 		[expr $j * $width + $yoffset] \
# 		[expr ($i+1) * $width + $xoffset] \
# 		[expr ($j+1) * $width + $yoffset] \
# 		-fill white -tag O$cnt
# 	    MakeX $c [expr $i * $width + $xoffset] \
# 		[expr $j * $width + $yoffset] $width $cnt
# 	    MakeO $c [expr $i * $width + $xoffset] \
# 		[expr $j * $width + $yoffset] $width $cnt
	}
	set cols [expr $cols-1]
	set xoffset [expr $xoffset + $xincr]
	set yoffset [expr $yoffset + $yincr]
    }
    $c create rectangle 0 0 $csize $csize -fill "light gray" -tag table
    $c lower pieces table
#     $c raise X1
#     $c raise O6
#     $c raise mi-2
#     $c itemconfigure Block6 -fill red
#     GS_ShowMoves $c value 3492116 { { 1 Win } { 2 Lose } { 3 Tie } }
#     puts [C_GenericUnhash 3492116 15]
#     GS_DrawPosition $c 3492116
#     set john [sum 4]
#     GS_DrawPosition $c 0
#     puts [C_GenericUnhash 0 15]
#     $c raise X1 base
    update idletasks
}

proc MakeMoveIndicator { c x y w h1 h2 cnt } {
    set h3 [expr $h2 / 3]
    $c create polygon \
	[expr $x - $w] [expr $y - $h1] \
	[expr $x - $w] [expr $y + $h1] \
	$x [expr $y + $h2] \
	[expr $x + $w] [expr $y + $h1] \
	[expr $x + $w] [expr $y - $h1] \
	$x [expr $y - $h2] \
	-fill "white" -dash - -joinstyle round \
	-outline black -width 2\
	-tag [list pieces moveindicators moveblock-$cnt mi-$cnt]
    $c create oval [expr $x - $h3] [expr $y - $h3] \
	[expr $x + $h3] [expr $y + $h3] -fill cyan -outline "" \
	-tag [list pieces moveindicators movedot-$cnt mi-$cnt]
    $c bind movedot-$cnt <ButtonRelease-1> \
	"ReturnFromHumanMove $cnt"
}

proc MakeXBlock { c x y w h1 h2 cnt } {
    set h3 [expr $h2 / 3]
    $c create polygon \
	[expr $x - $w] [expr $y - $h1] \
	[expr $x - $w] [expr $y + $h1] \
	$x [expr $y + $h2] \
	[expr $x + $w] [expr $y + $h1] \
	[expr $x + $w] [expr $y - $h1] \
	$x [expr $y - $h2] \
	-fill "light blue" -joinstyle round \
	-outline "black" -width 2\
	-tag [list pieces X$cnt]
}

proc MakeOBlock { c x y w h1 h2 cnt } {
    set h3 [expr $h2 / 3]
    $c create polygon \
	[expr $x - $w] [expr $y - $h1] \
	[expr $x - $w] [expr $y + $h1] \
	$x [expr $y + $h2] \
	[expr $x + $w] [expr $y + $h1] \
	[expr $x + $w] [expr $y - $h1] \
	$x [expr $y - $h2] \
	-fill "dark gray" -joinstyle round \
	-outline "black" -width 2\
	-tag [list pieces O$cnt]
}

# proc MakeX { c x y w cnt } {
#     $c create line [expr $x  + $w * 0.10] [expr $y + $w * 0.10] \
# 	[expr $x + $w * 0.90] [expr $y + $w * 0.90] -width 10 \
# 	-fill blue -capstyle round -tag X$cnt
#     $c create line [expr $x  + $w * 0.10] [expr $y + $w * 0.90] \
# 	[expr $x + $w * 0.90] [expr $y + $w * 0.10] -width 10 \
# 	-fill blue -capstyle round -tag X$cnt
# }

# proc MakeO { c x y w cnt } {
#     $c create oval [expr $x  + $w * 0.10] [expr $y + $w * 0.10] \
# 	[expr $x + $w * 0.90] [expr $y + $w * 0.90] -width 10 \
# 	-outline red -tag O$cnt
# }

#############################################################################
# GS_Deinitialize deletes everything in the playing canvas.  I'm not sure 
# why this is here, so whoever put this here should update this.  -Jeff
#############################################################################
proc GS_Deinitialize { c } {
    $c delete all
}


#############################################################################
# GS_DrawPosition this draws the board in an arbitrary position.
# It's arguments are a canvas, c, where you should draw and the
# (hashed) position.  For example, if your game is a rearranger,
# here is where you would draw pieces on the board in their correct positions.
# Imagine that this function is called when the player
# loads a saved game, and you must quickly return the board to its saved
# state.  It probably shouldn't animate, but it can if you want.
#
# BY THE WAY: Before you go any further, I recommend writing a tcl function 
# that UNhashes You'll thank yourself later.
# Don't bother writing tcl that hashes, that's never necessary.
#############################################################################
proc GS_DrawPosition { c position } {
    global csize rows Boardsize
    set Boardsize [sum $rows]

    $c lower pieces table
    set pieceStr [string range [C_GenericUnhash $position $Boardsize] \
		      0 [expr $Boardsize-1]]
    for {set slot 0} {$slot < $Boardsize} {set slot [expr $slot+1]} {
	if {[string compare [string index $pieceStr $slot] "x"] == 0} {
	    $c raise X[expr $slot+1]
	} elseif {[string compare [string index $pieceStr $slot] "o"] == 0} {
	    $c raise O[expr $slot+1]
	}
    }
}

#############################################################################
# GS_NewGame should start playing the game. 
# It's arguments are a canvas, c, where you should draw 
# the hashed starting position of the game.
# This is called just when the player hits "New Game"
# and before any moves are made.
#############################################################################
proc GS_NewGame { c position } {
    # TODO: The default behavior of this funciton is just to draw the position
    # but if you want you can add a special behaivior here like an animation
    GS_DrawPosition $c $position
}


#############################################################################
# GS_WhoseMove takes a position and returns whose move it is.
# Your return value should be one of the items in the list returned
# by GS_NameOfPieces.
# This function is called just before every move.
#############################################################################
proc GS_WhoseMove { position } {
    # Optional Procedure
    return ""    
}


#############################################################################
# GS_HandleMove draws (animates) a move being made.
# The user or the computer has just made a move, animate it or draw it
# or whatever.  Draw the piece moving if your game is a rearranger, or
# the piece appearing if it's a "dart board"
#
# By the way, if you animate, a function that will be useful for you is
# update idletasks.  You can call this to force the canvas to update if
# you make changes before tcl enters the event loop again.
#############################################################################
proc GS_HandleMove { c oldPosition theMove newPosition } {
    # TODO: The default behavior of this funciton is just to draw the position
    # but if you want you can add a special behaivior here like an animation
    GS_DrawPosition $c $newPosition
}


#############################################################################
# GS_ShowMoves draws the move indicator (be it an arrow or a dot, whatever the
# player clicks to make the move)  It is also the function that handles 
# coloring of the moves according to value. It is called by gamesman just 
# before the player is prompted for a move.
#
# Arguments:
# c = the canvas to draw in as usual
# moveType = a string which is either value, moves or best according to which 
# radio button is down
# position = the current hashed position
# moveList = a list of lists.  Each list contains a move and its value.
# These moves are represented as numbers (same as in C)
# The value will be either "Win" "Lose" or "Tie"
# Example:  moveList: { 73 Win } { 158 Lose } { 22 Tie } 
#############################################################################
proc GS_ShowMoves { c moveType position moveList } {

    foreach item $moveList {
	set move [lindex $item 0]
	set value [lindex $item 1]
	set color cyan

	if {$moveType == "value"} {
	    if {$value == "Tie"} {
		set color yellow
	    } elseif {$value == "Lose"} {
		set color green
	    } elseif {$value == "Win"} {
		set color red 
	    } else {
		set color cyan
		BadElse GS_ShowMoves
	    }
	}
	
	$c itemconfigure movedot-$move -fill $color
	$c raise mi-$move
    }
    update idletasks
}


#############################################################################
# GS_HideMoves erases the moves drawn by GS_ShowMoves.  It's arguments are the 
# same as GS_ShowMoves.
# You might not use all the arguments, and that's okay.
#############################################################################
proc GS_HideMoves { c moveType position moveList} {
    $c lower moveindicators table
}


#############################################################################
# GS_HandleUndo handles undoing a move (possibly with animation)
# Here's the undo logic
# The game was in position A, a player makes move M bringing the game to 
# position B
# then an undo is called
# currentPosition is the B
# theMoveToUndo is the M
# positionAfterUndo is the A
#
# By default this function just calls GS_DrawPosition, but you certainly don't 
# need to keep that.
#############################################################################
proc GS_HandleUndo { c currentPosition theMoveToUndo positionAfterUndo} {

    ### TODO if needed
    GS_DrawPosition $c $positionAfterUndo
}


#############################################################################
# GS_GetGameSpecificOptions is not quite ready, don't worry about it .
#############################################################################
proc GS_GetGameSpecificOptions { } {
}


#############################################################################
# GS_GameOver is called the moment the game is finished ( won, lost or tied)
# You could use this function to draw the line striking out the winning row in 
# tic tac toe for instance.  Or, you could congratulate the winner.
# Or, do nothing.
#############################################################################
proc GS_GameOver { c position gameValue nameOfWinningPiece nameOfWinner lastMove} {

    ### TODO if needed
    
}


#############################################################################
# GS_UndoGameOver is called when the player hits undo after the game is 
# finished.
# This is provided so that you may undo the drawing you did in GS_GameOver 
# if you drew something.
# For instance, if you drew a line crossing out the winning row in tic tac 
# toe, this is where you sould delete the line.
#
# note: GS_HandleUndo is called regardless of whether the move undoes the 
# end of the game, so IF you choose to do nothing in GS_GameOver, you 
# needn't do anything here either.
#############################################################################
proc GS_UndoGameOver { c position } {

    ### TODO if needed

}