# lra-floodwars
*lra-floodwars* is a bot that plays 'flood-it' (or 'flood-wars'), a game with 5 colours on a 50x50 grid, and chooses the best move.  

## Flood-wars
Two warriors are placed on separate corners of the grid; one player starts from the bottom-left corner, and the other from the top-right corner.  
They take turns changing their colour, which means changing the colour of their territory and conquering neighbouring cells with the same (new) colour.  
The winner is the player with the most captured area, which is decided based on total area after the whole grid is made up of only 2 colors (meaning they captured every cell available) or after the limit of 150 moves.  

## More about lra-floodwars
*lra-floodwars* was developed over the span of 2 months for a small competition between other very talented programmers.  
So far, I've implemented the following:  
* Game rules (so it won't make an illegal move),  
* Negamax algorithm (with iterative deepening, killer moves heuristic, and move ordering),  
* Static evaluation with component graphs and Voronoi diagram (or Voronoi distance, which calculates the minimum number of moves from cell A to cell B),  
* Transposition tables with Zobrist hashing (XOR hashing),  
* Undo moves stack (very efficient with bit operations).  

## Game rules
The time limit for each turn is 1 second (for a bot, of course).  
The grid is at least 10 * 10, and at most 50 * 50.  
On each turn, the player to move practically has 3 possible colours to change in; on your turn, you are not allowed to change into your current colour or the opponents' current colour.  
Between two players A and B, the 'game' they play consists of two 'matches', one where A is the bottom-left player and B is the top-right player, and vice-versa.  
The bot reads (from stdin) the following: a character 'J'/'S' representing the player's side (J is the initial for 'jos', which is 'down' in romanian, and S standing for 'sus', which translates to 'up'), followed by the N * M grid of colours.  
The five colours available are repesented by these characters: . # + @ *  
The bot prints (to stdout) the new N * M grid, with the move already applied.  

