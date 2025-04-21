#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <cstdlib>
#include <ctime>
#include <conio.h>
#include <chrono>
#include <thread>
#include <windows.h>
#include <queue>
#include <mutex>
#include <atomic>
#include <tuple>
using namespace std;
// Game constants
constexpr int WIDTH = 10, HEIGHT = 10;// Maze dimensions
constexpr int ENEMY_UPDATE_INTERVAL = 500;// How often enemies move (ms)
constexpr int MAX_ENEMIES = 3; // Maximum number of enemies
constexpr int FRAME_DELAY = 50; // Delay between frames (ms)

// Thread synchronization objects
mutex game_mutex;// Protects game state
atomic<bool> game_running(true);// Controls game loop

// Helper function to set console cursor position
void setCursor(int x, int y) {
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), {static_cast<SHORT>(y), static_cast<SHORT>(x)});
}
// Helper function to set console text color
void setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}
// Base class for all game entities (player, enemies, exit)
class Entity {
public:
    int x, y;// Position coordinates
    char direction = 'X'; // Current facing direction (U, D, L, R, X)

    Entity(int x = 0, int y = 0) : x(x), y(y) {}
    // Returns the character representing the direction
    char dirChar() const {
        switch (direction) {
            case 'U': return '^';
            case 'D': return 'v';
            case 'L': return '<';
            case 'R': return '>';
            default: return 'X';
        }
    }
};
// Exit class - the goal the player needs to reach
class Exit : public Entity {
public:
    using Entity::Entity;
    // Calculate the initial path from player start to exit
    void calcPath(const vector<vector<char>>& maze) {
        vector<vector<bool>> visited(HEIGHT, vector<bool>(WIDTH));
        vector<vector<pair<int, int>>> parent(HEIGHT, vector<pair<int, int>>(WIDTH, {-1, -1}));
        queue<pair<int, int>> q({{1, 1}});  // Start from player's initial position
        visited[1][1] = true;
        // Possible movement directions (up, down, left, right)
        constexpr array dx{-1, 1, 0, 0}, dy{0, 0, -1, 1};
        // BFS to find path from start to exit
        while (!q.empty()) {
            auto [x, y] = q.front(); q.pop();
            for (int d = 0; d < 4; ++d) {
                int nx = x + dx[d], ny = y + dy[d];
                if (nx >= 0 && nx < HEIGHT && ny >= 0 && ny < WIDTH &&
                    maze[nx][ny] != '#' && !visited[nx][ny]) {
                    visited[nx][ny] = true;
                    parent[nx][ny] = {x, y};
                    q.push({nx, ny});
                }
            }
        }
        // If no path exists, return
        if (!visited[this->x][this->y]) return;
        // Trace back the path to determine initial direction
        int cx = this->x, cy = this->y;
        while (parent[cx][cy] != pair{1, 1} && parent[cx][cy] != pair{-1, -1})
            tie(cx, cy) = parent[cx][cy];
        // Set exit direction based on path
        if (cx == this->x - 1) direction = 'D';
        else if (cx == this->x + 1) direction = 'U';
        else if (cy == this->y - 1) direction = 'R';
        else if (cy == this->y + 1) direction = 'L';
    }
    // Exit is always represented by 'E'
    char dirChar() const { return 'E'; }
};
// Player class - the main character controlled by user
class Player : public Entity {
public:
    int score = 0;      // Current score
    int moves = 0;      // Number of moves made
    int level = 1;      // Current level
    Player(int x = 1, int y = 1) : Entity(x, y) {}
    // Move player in specified direction if possible
    void move(char dir, const vector<vector<char>>& maze) {
        constexpr array dx{-1, 1, 0, 0}, dy{0, 0, -1, 1};
        int d = "WSAD"s.find(dir);  // Map input to direction index
        if (d == string::npos) return;
        // Calculate new position
        int nx = x + dx[d], ny = y + dy[d];
        // Check if move is valid (within bounds and not blocked)
        if (nx >= 0 && nx < HEIGHT && ny >= 0 && ny < WIDTH && maze[nx][ny] != '#') {
            x = nx; y = ny; moves++;
        }
    }
};
// Enemy class - AI controlled entities that chase player
class Enemy : public Entity {
public:
    using Entity::Entity;
    // Update enemy position based on player location
    void update(const vector<vector<char>>& maze, const Player& p) {
        // 60% chance to move toward player, 40% random move
        if (rand() % 100 < 60) {
            // Calculate direction toward player
            int dx = p.x - x, dy = p.y - y;
            // Try to move in primary direction, fallback to secondary
            if (abs(dx) > abs(dy)) 
                tryMove(dx > 0 ? 'D' : 'U', maze) || tryMove(dy > 0 ? 'R' : 'L', maze);
            else 
                tryMove(dy > 0 ? 'R' : 'L', maze) || tryMove(dx > 0 ? 'D' : 'U', maze);
        } else {
            // Random move
            tryMove("UDLR"[rand() % 4], maze);
        }
    }
private:
    // Attempt to move in specified direction
    bool tryMove(char dir, const vector<vector<char>>& maze) {
        constexpr array dx{-1, 1, 0, 0}, dy{0, 0, -1, 1};
        int d = "UDLR"s.find(dir);
        int nx = x + dx[d], ny = y + dy[d];
        
        // Check if move is valid
        if (nx >= 0 && nx < HEIGHT && ny >= 0 && ny < WIDTH && maze[nx][ny] != '#') {
            x = nx; y = ny; direction = dir;
            return true;
        }
        direction = 'X';
        return false;
    }
};
// Create a random maze for the current level
vector<vector<char>> createMaze(int level, Exit& exit) {
    vector<vector<char>> maze(HEIGHT, vector<char>(WIDTH, ' '));
    
    // Keep generating mazes until we get a valid one with path to exit
    do {
        // Initialize empty maze
        for (auto& row : maze) fill(row.begin(), row.end(), ' ');
        
        // Create borders
        for (int i = 0; i < HEIGHT; i++) maze[i][0] = maze[i][WIDTH-1] = '#';
        for (int j = 0; j < WIDTH; j++) maze[0][j] = maze[HEIGHT-1][j] = '#';
        
        // Add random walls (fewer walls at higher levels)
        for (int i = 1; i < HEIGHT-1; i++)
            for (int j = 1; j < WIDTH-1; j++)
                if (rand() % max(4, 10-level) == 0 && !(i == 1 && j == 1))
                    maze[i][j] = '#';
        
        // Place exit in bottom-right corner
        exit = {HEIGHT-2, WIDTH-2};
        maze[exit.x][exit.y] = 'E';
        exit.calcPath(maze);
        
        // Place coins (more coins at higher levels)
        for (int coins = min(5, 3+level); coins > 0; ) {
            int x = rand() % (HEIGHT-2) + 1, y = rand() % (WIDTH-2) + 1;
            if (maze[x][y] == ' ') { maze[x][y] = '*'; coins--; }
        }
    } while (![&](){
        // BFS to check if exit is reachable from start
        vector<vector<bool>> vis(HEIGHT, vector<bool>(WIDTH));
        queue<pair<int, int>> q({{1, 1}});
        vis[1][1] = true;
        constexpr array dx{-1, 1, 0, 0}, dy{0, 0, -1, 1};
        
        while (!q.empty()) {
            auto [x, y] = q.front(); q.pop();
            if (x == exit.x && y == exit.y) return true;  // Path found
            for (int d = 0; d < 4; ++d) {
                int nx = x + dx[d], ny = y + dy[d];
                if (nx >= 0 && nx < HEIGHT && ny >= 0 && ny < WIDTH &&
                    maze[nx][ny] != '#' && !vis[nx][ny]) {
                    vis[nx][ny] = true;
                    q.push({nx, ny});
                }
            }
        }
        return false;  // No path found
    }());
    
    return maze;
}
// Create enemies for current level
vector<Enemy> createEnemies(int level) {
    vector<Enemy> enemies;
    // More enemies at higher levels (up to MAX_ENEMIES)
    for (int i = 0, count = min(MAX_ENEMIES, 1 + level/2); i < count; ) {
        // Random position not too close to start or exit
        int x = rand() % (HEIGHT-4) + 2, y = rand() % (WIDTH-4) + 2;
        if ((x != 1 || y != 1) && (x != HEIGHT-2 || y != WIDTH-2))
            enemies.emplace_back(x, y), i++;
    }
    return enemies;
}
// Draw the current game state
void drawMaze(const vector<vector<char>>& maze, const Player& p, const vector<Enemy>& enemies, const Exit& e) {
    lock_guard<mutex> lock(game_mutex);  // Protect game state during drawing
    setCursor(0, 0);
    // Draw each cell
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            bool drawn = false;
            
            // Draw enemies first (so they appear on top)
            for (const auto& en : enemies)
                if (i == en.x && j == en.y) {
                    setColor(12); cout << en.dirChar(); drawn = true; break;
                }
            
            if (!drawn) {
                if (i == p.x && j == p.y) setColor(10), cout << 'P';  // Player
                else if (i == e.x && j == e.y) setColor(11), cout << 'E';  // Exit
                else switch(maze[i][j]) {
                    case '#': setColor(8); cout << char(219); break;  // Wall
                    case '*': setColor(14); cout << '*'; break;       // Coin
                    default: setColor(15); cout << ' ';               // Empty
                }
            }
        }
        cout << '\n';
    }
    // Display game info
    cout << "Level: " << p.level << " | Score: " << p.score
         << " | Moves: " << p.moves << " | Enemies: " << enemies.size() << "\n"
         << "WASD: Move | Q: Quit | P: Save\n";
}
// Save game state to file
void saveGame(const Player& p, const vector<vector<char>>& maze, const vector<Enemy>& enemies, const Exit& e) {
    ofstream file("savegame.dat", ios::binary);
    if (!file) return;
    // Write player data
    file.write(reinterpret_cast<const char*>(&p), sizeof(p));
    // Write maze
    for (const auto& row : maze) file.write(row.data(), row.size());
    // Write enemies
    size_t count = enemies.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& en : enemies) file.write(reinterpret_cast<const char*>(&en), sizeof(en));
    //Write exit
    file.write(reinterpret_cast<const char*>(&e), sizeof(e));
}
//Load game state from file
bool loadGame(Player& p, vector<vector<char>>& maze, vector<Enemy>& enemies, Exit& e) {
    ifstream file("savegame.dat", ios::binary);
    if (!file) return false;
    
    //Read player data
    file.read(reinterpret_cast<char*>(&p), sizeof(p));
    
    //Read maze
    maze.resize(HEIGHT, vector<char>(WIDTH));
    for (auto& row : maze) file.read(row.data(), row.size());
    
    //read enemies
    size_t count;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    enemies.resize(count);
    for (auto& en : enemies) file.read(reinterpret_cast<char*>(&en), sizeof(en));
    
    //Read exit
    file.read(reinterpret_cast<char*>(&e), sizeof(e));
    
    return file.good();
}

//Main game loop
void gameLoop(Player& p, vector<vector<char>>& maze, vector<Enemy>& enemies, Exit& e) {
    game_running = true;
    
    //Enemy AI thread (runs independently)
    thread enemyThread([&](){
        auto lastUpdate = chrono::steady_clock::now();
        while (game_running) {
            // Update enemies at fixed intervals
            if (chrono::steady_clock::now() - lastUpdate >= chrono::milliseconds(ENEMY_UPDATE_INTERVAL)) {
                lock_guard<mutex> lock(game_mutex);
                for (auto& en : enemies) en.update(maze, p);
                lastUpdate = chrono::steady_clock::now();
            }
            this_thread::sleep_for(chrono::milliseconds(10));
        }
    });
    //Main game loop
    while (game_running) {
        drawMaze(maze, p, enemies, e);
        {
            lock_guard<mutex> lock(game_mutex);
            //Check for collision with enemies
            for (const auto& en : enemies)
                if (p.x == en.x && p.y == en.y) {
                    setColor(12); cout << "GAME OVER! Caught by enemy!\n"; setColor(15);
                    this_thread::sleep_for(2s);
                    game_running = false;
                    break;
                }
            //Check for coin collection
            if (maze[p.x][p.y] == '*') { p.score += 10; maze[p.x][p.y] = ' '; }
            
            // Check for level completion
            if (p.x == e.x && p.y == e.y) {
                setColor(10); cout << "LEVEL COMPLETE!\n"; setColor(15);
                this_thread::sleep_for(2s);
                p.level++; p.x = p.y = 1; p.moves = 0;  // Reset for next level
                maze = createMaze(p.level, e);
                enemies = createEnemies(p.level);
            }
        }
        // Handle user input
        if (_kbhit()) {
            char input = toupper(_getch());
            if (input == 'Q') game_running = false;  // Quit
            else if (input == 'P') {  // Save
                lock_guard<mutex> lock(game_mutex);
                saveGame(p, maze, enemies, e);
                setColor(10); cout << "Game saved!\n"; this_thread::sleep_for(1s);
            } else if ("WSAD"s.find(input) != string::npos) {  // Movement
                lock_guard<mutex> lock(game_mutex);
                p.move(input, maze);
            }
        }
        this_thread::sleep_for(chrono::milliseconds(FRAME_DELAY));
    }
    enemyThread.join();
}
int main() {
    // Set up console
    system("mode con: cols=40 lines=25");
    srand(static_cast<unsigned int>(time(nullptr)));
    Player player;
    vector<vector<char>> maze;
    vector<Enemy> enemies;
    Exit exit;
    while (true) {
        system("cls");
        setColor(11); cout << "=== MAZE OF TERROR ===\n"; setColor(15);
        cout << "     ______" << endl;
        cout << "  .-'      '-." << endl;
        cout << " /            \\" << endl;
        cout << "|              |" << endl;
        cout << "|,  .-.  .-.  ,|" << endl;
        cout << "| )(_o/  \\o_)( |" << endl;
        cout << "|/     /\\     \\|" << endl;
        cout << "(_     ^^     _)" << endl;
        cout << " \\__|IIIIII|__/" << endl;
        cout << "  | \\IIIIII/ |" << endl;
        cout << "  \\          /" << endl;
        cout << "   `--------`\n";
        // Menu options
        cout << "1. New Game\n2. Continue Game\n3. Exit\nSelect option: ";
        char choice = _getch();

        if (choice == '1') {
            player = Player();  // Reset player
            maze = createMaze(player.level, exit);
            enemies = createEnemies(player.level);
            gameLoop(player, maze, enemies, exit);
        }
        else if (choice == '2') {
            if (loadGame(player, maze, enemies, exit)) {
                gameLoop(player, maze, enemies, exit);
            } else {
                setColor(12); cout << "No saved game found!\n"; setColor(15);
                this_thread::sleep_for(chrono::seconds(2));
            }
        }
        else if (choice == '3') {
            break;
        }
    }

    return 0;
}
