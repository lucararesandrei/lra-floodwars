# lra-floodwars v10
*lra-floodwars* is a bot that plays 'flood-it' (or 'flood-wars'), a game with 5 colours on a 50x50 grid, and chooses the best move to win.  

### Flood-wars
Two opponents are placed on separate corners of the grid; one's at the bottom-left corner, and the other at the top-right corner.  
They take turns changing their colour, which changes the colour of their already-captured area, and automatically captures all connected grid cells on their border of the same colour.  
The winner is the player with the most captured area, which is decided after the whole grid is 2 colors (the two players captured everything) or after the limit of 150 moves.  

### The bot
*lra-floodwars* was developed by me in about 2 months for a small tournament in my class.  
So far, I've implemented the following:  
 . Game rules (so it won't make an illegal move),  
 . Negamax algorithm (with iterative deepening, killer moves heuristic, and move ordering),  
 . Static evaluation with component graphs and Voronoi diagram (or Voronoi distance, basically calculates the minimum number of moves from cell A to cell B),  
 . Transposition tables with Zobrist hashing (xor hashing),  
 . Undo moves stack (very efficient with bit operations).  

### Extra game rules
The time limit for each turn is 1 second (and no memory limit).  
The grid is at least 10x10, and at most 50x50.  
On each turn, the player to move practically has 3 possible colours to change in; on your turn, you are not allowed to change into your current colour or the opponents' current colour.  
Between two bots A and B, the 'game' they play consists of two 'matches', one where A is the bottom-left player and B is the top-right player, and vice-versa.  
The bot reads from stdin the following: a character 'J'/'S' representing the player's side (J standing for jos, which is 'down' in romanian, and S standing for sus, which translates to 'up'), followed by the NxM grid of colours.  
Since you can't read colours, the five colours available are repesented by these characters: '.#+@*'.  
The bot prints to stdout the new NxM grid, with the move already applied.  

### Conclusion
I truly recommend implementing a flood-wars bot (though it would be better to have two players), as it really helps understanding complex game-logic algorithms such as minimax (with or without alpha-beta pruning), and other interesting ideas such as weird static evaluation ideas or transposition tables.  
*lra-floodwars* is open-source in this repository, for everyone to see, use, or even change!  
