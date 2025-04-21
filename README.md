==========================
     MAZE OF TERROR
==========================

A Console-Based Maze Adventure Game in C++
Created by: [Your Name]
Version: 1.0
-------------------------
 ABOUT THE GAME
--------------------------
Maze of Terror is a thrilling console game where you control a player trapped in a randomly generated maze. 
Your goal? Collect coins, dodge deadly enemies, and reach the exit to survive and progress through increasingly difficult levels.
But beware... the enemies are smart. They track you down. One wrong step—and it’s game over!
--------------------------
 CONTROLS
--------------------------
W - Move Up  
S - Move Down  
A - Move Left  
D - Move Right  
P - Save Game  
Q - Quit Game
--------------------------
 OBJECTIVE
--------------------------
- Navigate the maze and reach the exit (marked as 'E').
- Collect coins ('*') to increase your score.
- Avoid enemies ('^', 'v', '<', '>') at all costs!
- Complete each level to unlock the next one—more enemies, more danger!
--------------------------
 SAVE & LOAD
--------------------------
You can save your progress anytime by pressing 'P'.  
To continue from where you left off, choose **Continue Game** in the main menu.
Your save file will be stored as `savegame.dat` in the same directory.
--------------------------
HOW TO RUN
--------------------------
1. Make sure you're on Windows (uses `<windows.h>` and `<conio.h>`).
2. Compile using a C++ compiler (like g++ or MSVC).  
   Example using g++:
   ```bash
   g++ -o MazeOfTerror main.cpp
