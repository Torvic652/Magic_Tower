#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <map>
#include <cmath> // Needed for circle distance calculations

using namespace std;


// ENUMS & STRUCTS


enum class TileType {
    EMPTY, WALL, PLAYER, GOAL, KEY, DOOR, ENEMY, HP_POT, STR_POT, DEF_POT, TELEPORTER
};

struct Stats {
    int hp;
    int str;
    int def;
};


// CLASSES


class Character {
private:
    Stats stats;
public:
    Character(int h, int s, int d) : stats{ h, s, d } {}
    Stats getStats() const { return stats; }
    void modifyHealth(int amt) { stats.hp += amt; }
    void addStrength(int amt) { stats.str += amt; }
    void addDefense(int amt) { stats.def += amt; }
    bool isAlive() const { return stats.hp > 0; }
};

class Player : public Character {
private:
    int keys;
    int x, y;
    string name;
public:
    Player(int h, int s, int d) : Character(h, s, d), keys(0), x(0), y(0) {}

    int getKeys() const { return keys; }
    void addKey() { keys++; }
    void useKey() { keys--; }


    void setPos(int nx, int ny) { x = nx; y = ny; }
    int getX() const { return x; }
    int getY() const { return y; }
};
// the dungeon structure contains the grid and a mapp
struct Dungeon {
    string name;
    int rows;
    int cols;
    vector<vector<TileType>> grid;
    // Maps a teleporter's {x, y} directly to its partner's {destX, destY}
    map<pair<int, int>, pair<int, int>> teleporters;
};


// GAME logic and management


class GameManager {
private:
    vector<Dungeon> dungeons;
    //convert tile type to display character
    char tileToChar(TileType t) const {
        switch (t) {
        case TileType::EMPTY: return ' ';
        case TileType::WALL: return '#';
        case TileType::PLAYER: return '@';
        case TileType::GOAL: return 'G';
        case TileType::KEY: return 'K';
        case TileType::DOOR: return 'X';
        case TileType::ENEMY: return 'E';
        case TileType::HP_POT: return 'H';
        case TileType::STR_POT: return 'S';
        case TileType::DEF_POT: return 'D';
        case TileType::TELEPORTER: return 'T';
        default: return '?';
        }
    }
    //get integer input with validation 
    int getIntInput(const string& prompt, int minVal = -2147483648, int maxVal = 2147483647) {
        string input;
        int val;
        while (true) {
            cout << prompt;
            getline(cin, input);
            try {
                size_t pos;
                val = stoi(input, &pos);
                if (pos == input.length() && val >= minVal && val <= maxVal) {
                    return val;
                }
            }
            catch (...) {}
            cout << "Invalid input. Please enter a valid number within range.\n";
        }
    }
    //get string input for easier command
    string getLowerStrInput(const string& prompt) {
        string val;
        cout << prompt;
        getline(cin, val);
        transform(val.begin(), val.end(), val.begin(), ::tolower);
        return val;
    }
    //simple battle system
    bool conductBattle(Player& p, Character& e) {
        int pDmg = max(0, p.getStats().str - e.getStats().def);
        int eDmg = max(0, e.getStats().str - p.getStats().def);

        if (pDmg == 0) {
            cout << "You cannot pierce the enemy's armor! You flee the battle.\n";
            p.modifyHealth(-p.getStats().hp);
            return false;
        }

        while (p.isAlive() && e.isAlive()) {
            e.modifyHealth(-pDmg);
            if (!e.isAlive()) return true;

            p.modifyHealth(-eDmg);
            if (!p.isAlive()) return false;
        }
        return false;
    }
    //main game loop for playing the dungeon
    void playDungeonLoop(Dungeon activeDungeon) {
        Player p(50, 5, 2);

        for (int i = 0; i < activeDungeon.rows; i++) {
            for (int j = 0; j < activeDungeon.cols; j++) {
                if (activeDungeon.grid[i][j] == TileType::PLAYER) {
                    p.setPos(j, i);
                    activeDungeon.grid[i][j] = TileType::EMPTY;
                }
            }
        }

        string message = "You entered the dungeon!";

        while (true) {
            cout << "\n========================================\n";
            cout << " HP: " << p.getStats().hp << " | STR: " << p.getStats().str
                << " | DEF: " << p.getStats().def << " | Keys: " << p.getKeys() << "\n";
            cout << "========================================\n";

            for (int i = 0; i < activeDungeon.rows; i++) {
                for (int j = 0; j < activeDungeon.cols; j++) {
                    if (p.getX() == j && p.getY() == i) {
                        cout << '@';
                    }
                    else {
                        cout << tileToChar(activeDungeon.grid[i][j]);
                    }
                }
                cout << "\n";
            }
            cout << "\n> " << message << "\n\n";
            message = "";

            string move = getLowerStrInput("Enter move (Up/U, Down/D, Left/L, Right/R) or Quit/Q: ");
            int dx = 0, dy = 0;

            if (move == "up" || move == "u") dy = -1;
            else if (move == "down" || move == "d") dy = 1;
            else if (move == "left" || move == "l") dx = -1;
            else if (move == "right" || move == "r") dx = 1;
            else if (move == "quit" || move == "q") {
                cout << "Returning to main menu...\n";
                return;
            }
            else {
                message = "Invalid input! Use directions or abbreviations.";
                continue;
            }

            int nx = p.getX() + dx;
            int ny = p.getY() + dy;

            if (nx < 0 || nx >= activeDungeon.cols || ny < 0 || ny >= activeDungeon.rows) {
                message = "You bumped into the boundaries of the world!";
                continue;
            }
            
            TileType targetTile = activeDungeon.grid[ny][nx];

            if (targetTile == TileType::WALL) {
                message = "You bumped into a wall!";
            }
            else if (targetTile == TileType::EMPTY) {
                p.setPos(nx, ny);
                message = "Moved.";
            }
            else if (targetTile == TileType::KEY) {
                p.addKey();
                activeDungeon.grid[ny][nx] = TileType::EMPTY;
                p.setPos(nx, ny);
                message = "You picked up a Key!";
            }
            else if (targetTile == TileType::DOOR) {
                if (p.getKeys() > 0) {
                    p.useKey();
                    activeDungeon.grid[ny][nx] = TileType::EMPTY;
                    p.setPos(nx, ny);
                    message = "You unlocked the door!";
                }
                else {
                    message = "The door is locked. You need a Key.";
                }
            }
            else if (targetTile == TileType::TELEPORTER) {
                // Look up coordinates in map to find the linked partner
                auto it = activeDungeon.teleporters.find({ nx, ny });
                if (it != activeDungeon.teleporters.end()) {
                    p.setPos(it->second.first, it->second.second);
                    message = "*ZWOOSH* You stepped into a teleporter and warped!";
                }
                else {
                    // Safety catch if map data is corrupted
                    p.setPos(nx, ny);
                    message = "This teleporter seems broken...";
                }
            }
            else if (targetTile == TileType::HP_POT) {
                p.modifyHealth(20);
                activeDungeon.grid[ny][nx] = TileType::EMPTY;
                p.setPos(nx, ny);
                message = "Drank a Health Potion! (+20 HP)";
            }
            else if (targetTile == TileType::STR_POT) {
                p.addStrength(2);
                activeDungeon.grid[ny][nx] = TileType::EMPTY;
                p.setPos(nx, ny);
                message = "Drank a Strength Potion! (+2 STR)";
            }
            else if (targetTile == TileType::DEF_POT) {
                p.addDefense(2);
                activeDungeon.grid[ny][nx] = TileType::EMPTY;
                p.setPos(nx, ny);
                message = "Drank a Defense Potion! (+2 DEF)";
            }
            else if (targetTile == TileType::ENEMY) {
                Character enemy(20, 5, 2);
                cout << "\n[!] BATTLE INITIATED [!]\nEnemy Stats: HP 20 | STR 5 | DEF 2\n";

                bool win = conductBattle(p, enemy);
                if (win) {
                    activeDungeon.grid[ny][nx] = TileType::EMPTY;
                    p.setPos(nx, ny);
                    message = "You defeated the enemy!";
                }
                else {
                    cout << "\n=== GAME OVER ===\nYou perished in battle.\n\n";
                    return;
                }
            }
            else if (targetTile == TileType::GOAL) {
                cout << "\n*** VICTORY! ***\nYou reached the Goal and escaped the dungeon!\n\n";
                return;
            }
        }
    }
    //Menu to select dungeon to play or return to main menu if no dungeons or after playing one
    void loadDungeonMenu() {
        if (dungeons.empty()) {
            cout << "No dungeons available to play!\n";
            return;
        }

        cout << "\n--- Select a Dungeon ---\n";
        for (size_t i = 0; i < dungeons.size(); ++i) {
            cout << i + 1 << ") " << dungeons[i].name << "\n";
        }
        cout << dungeons.size() + 1 << ") Back to Menu\n";

        int choice = getIntInput("Enter selection: ", 1, dungeons.size() + 1);
        if (choice <= dungeons.size()) {
            playDungeonLoop(dungeons[choice - 1]);
        }
    }
    //level editor with tools for single object, rectangle, hollow rectangle  and circle + teleporter
    void launchLevelEditor() {
        cout << "\n--- LEVEL EDITOR ---\n";
        int r = getIntInput("Enter number of rows (1-50): ", 1, 50);
        int c = getIntInput("Enter number of columns (1-50): ", 1, 50);
        
        Dungeon d;
        d.rows = r; d.cols = c;
        d.grid = vector<vector<TileType>>(r, vector<TileType>(c, TileType::EMPTY));

        while (true) {
            cout << "\n    ";
            for (int j = 1; j <= c; ++j) cout << j % 10 << " ";
            cout << "\n";
            for (int i = 0; i < r; ++i) {
                cout << (i + 1 < 10 ? " " : "") << i + 1 << "| ";
                for (int j = 0; j < c; ++j) {
                    cout << tileToChar(d.grid[i][j]) << " ";
                }
                cout << "\n";
            }

            cout << "\n--- EDITOR TOOLS ---\n"
                << "1) Single Object       2) Rectangle (Filled)\n"
                << "3) Hollow Rectangle    4) Circle\n"
                << "5) SAVE DUNGEON & EXIT\n";

            int tool = getIntInput("Select a tool (1-5): ", 1, 5);
            //save option validates that there is exactly 1 player and at least 1 goal
            if (tool == 5) {
                int pCount = 0, gCount = 0;
                for (const auto& row : d.grid) {
                    for (auto tile : row) {
                        if (tile == TileType::PLAYER) pCount++;
                        if (tile == TileType::GOAL) gCount++;
                    }
                }
                if (pCount != 1) {
                    cout << "\n[!] Cannot save: Dungeon must have exactly ONE Player starting position (@).\n";
                    continue;
                }
                if (gCount < 1) {
                    cout << "\n[!] Cannot save: Dungeon must have at least ONE Goal (G).\n";
                    continue;
                }

                cout << "Enter a name for your dungeon: ";
                getline(cin, d.name);
                dungeons.push_back(d);
                cout << "Dungeon saved successfully! Added to Play menu.\n";
                break;
            }

            cout << "\nAvailable Objects:\n"
                << "1) Empty Space   2) Wall           3) Player\n"
                << "4) Goal          5) Key            6) Locked Door\n"
                << "7) Enemy         8) Health Potion  9) Strength Potion\n"
                << "10) Defense Potion                 11) Teleporter\n\n";
            //object selection by number or name for easier use of tools and teleporters
            string choice = getLowerStrInput("Select an object (number or name): ");
            TileType selectedTile = TileType::EMPTY;

            if (choice == "1" || choice == "empty") selectedTile = TileType::EMPTY;
            else if (choice == "2" || choice == "wall") selectedTile = TileType::WALL;
            else if (choice == "3" || choice == "player") selectedTile = TileType::PLAYER;
            else if (choice == "4" || choice == "goal") selectedTile = TileType::GOAL;
            else if (choice == "5" || choice == "key") selectedTile = TileType::KEY;
            else if (choice == "6" || choice == "door") selectedTile = TileType::DOOR;
            else if (choice == "7" || choice == "enemy") selectedTile = TileType::ENEMY;
            else if (choice == "8" || choice == "health") selectedTile = TileType::HP_POT;
            else if (choice == "9" || choice == "strength") selectedTile = TileType::STR_POT;
            else if (choice == "10" || choice == "defense") selectedTile = TileType::DEF_POT;
            else if (choice == "11" || choice == "teleporter") selectedTile = TileType::TELEPORTER;
            else {
                cout << "Invalid object choice!\n";
                continue;
            }

            // TELEPORTER LOGIC (Forces Single Object placement to pair correctly)
            if (selectedTile == TileType::TELEPORTER) {
                cout << "\n[Teleporters must be placed in pairs!]\n";
                int r1 = getIntInput("Enter Row for Teleporter A: ", 1, r) - 1;
                int c1 = getIntInput("Enter Col for Teleporter A: ", 1, c) - 1;
                int r2 = getIntInput("Enter Row for Teleporter B: ", 1, r) - 1;
                int c2 = getIntInput("Enter Col for Teleporter B: ", 1, c) - 1;

                d.grid[r1][c1] = TileType::TELEPORTER;
                d.grid[r2][c2] = TileType::TELEPORTER;

                // Link coordinates bi-directionally {x, y}
                d.teleporters[{c1, r1}] = { c2, r2 };
                d.teleporters[{c2, r2}] = { c1, r1 };
                cout << "Teleporters linked and placed!\n";
            }
            // STANDARD SHAPE TOOLS
            else {
                if (tool == 1) { // Single Object
                    int row = getIntInput("Enter row: ", 1, r) - 1;
                    int col = getIntInput("Enter col: ", 1, c) - 1;
                    d.grid[row][col] = selectedTile;
                }
                else if (tool == 2 || tool == 3) { // Rectangles
                    int r1 = getIntInput("Enter Corner 1 Row: ", 1, r) - 1;
                    int c1 = getIntInput("Enter Corner 1 Col: ", 1, c) - 1;
                    int r2 = getIntInput("Enter Corner 2 Row: ", 1, r) - 1;
                    int c2 = getIntInput("Enter Corner 2 Col: ", 1, c) - 1;

                    int minR = min(r1, r2), maxR = max(r1, r2);
                    int minC = min(c1, c2), maxC = max(c1, c2);

                    for (int i = minR; i <= maxR; ++i) {
                        for (int j = minC; j <= maxC; ++j) {
                            if (tool == 2) {
                                d.grid[i][j] = selectedTile; // Filled
                            }
                            else {
                                if (i == minR || i == maxR || j == minC || j == maxC) {
                                    d.grid[i][j] = selectedTile; // Hollow
                                }
                            }
                        }
                    }
                }
                else if (tool == 4) { // Circle
                    int rc = getIntInput("Enter Center Row: ", 1, r) - 1;
                    int cc = getIntInput("Enter Center Col: ", 1, c) - 1;
                    int rad = getIntInput("Enter Radius (in grid spaces): ", 0, max(r, c));

                    for (int i = 0; i < r; ++i) {
                        for (int j = 0; j < c; ++j) {
                            // Euclidean distance squared check: (x1-x2)^2 + (y1-y2)^2 <= r^2
                            if ((i - rc) * (i - rc) + (j - cc) * (j - cc) <= rad * rad) {
                                d.grid[i][j] = selectedTile;
                            }
                        }
                    }
                }
            }
        }
    }

    void loadDefaultDungeons() {

        //default dungeon
        // Modified  to include Teleporter!
        Dungeon d2;
        d2.name = "Georgina  with Teleporter";
        d2.rows = 7; d2.cols = 7;
        d2.grid = vector<vector<TileType>>(d2.rows, vector<TileType>(d2.cols, TileType::EMPTY));
        for (int i = 0; i < d2.rows; ++i) {
            for (int j = 0; j < d2.cols; ++j) {
                if (i == 0 || i == d2.rows - 1 || j == 0 || j == d2.cols - 1)
                    d2.grid[i][j] = TileType::WALL;
            }
        }
        d2.grid[1][1] = TileType::PLAYER;
        d2.grid[1][3] = TileType::ENEMY;
        d2.grid[1][5] = TileType::TELEPORTER; // Teleporter A

        d2.grid[2][3] = TileType::WALL;
        d2.grid[2][5] = TileType::WALL;

        d2.grid[3][1] = TileType::KEY;
        d2.grid[3][3] = TileType::DOOR;
        d2.grid[3][5] = TileType::GOAL;

        // Secret teleport room
        d2.grid[5][1] = TileType::TELEPORTER; // Teleporter B
        d2.grid[5][3] = TileType::DEF_POT;
        d2.grid[5][5] = TileType::STR_POT;

        // Link Teleporter A at {5, 1} to Teleporter B at {1, 5}
        d2.teleporters[{5, 1}] = { 1, 5 };
        d2.teleporters[{1, 5}] = { 5, 1 };

        dungeons.push_back(d2);
    }

public:
    GameManager() {
        loadDefaultDungeons();
    }

    void run() {
        while (true) {
            cout << "\nWelcome to Magic Tower:\n";
            cout << " 1) Enter a dungeon\n";
            cout << " 2) Design a dungeon\n";
            cout << " 3) Exit\n";

            int choice = getIntInput("Select an option: ", 1, 3);
            if (choice == 1) {
                loadDungeonMenu();
            }
            else if (choice == 2) {
                launchLevelEditor();
            }
            else {
                cout << "Exiting program. Goodbye!\n";
                break;
            }
        }
    }
};


//main
int main() {
    GameManager game;
    game.run();
    return 0;
}