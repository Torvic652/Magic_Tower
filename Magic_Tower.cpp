#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <fstream>

using namespace std;

//enum structs

enum class TileType {EMPTY, WALL, PLAYER, GOAL, KEY, DOOR, ENEMY, HP_POT, STR_POT, DEF_POT, TELEPORTER, DAMAGE_FLOOR};

struct Stats {
    int hp;
    int str;
    int def;
};

struct Tile {
    TileType type;
    int hp; // for enemies
    int str; // for enemies
    int def; // for enemies
    int value; // for pots and damage floors
    string text; //keys doors and damage floor tyes
    int targetX, targetY; //for teleporters




    Tile() {
        type = TileType::EMPTY;
        hp = 0; str = 0; def = 0;
        value = 0; text = "";
        targetX = 0; targetY = 0;
    }
};


struct KeyInv {
    string type;
    int count;
};

// CLASSES

class Character {
protected:
    Stats stats;
public:
    Character(int h, int s, int d) {
        stats.hp = h;
        stats.str = s;
        stats.def = d;
    }
    Stats getStats() const { return stats; }
    void modifyHealth(int amt) { stats.hp += amt; }
    void addStrength(int amt) { stats.str += amt; }
    void addDefense(int amt) { stats.def += amt; }
    bool isAlive() const { return stats.hp > 0; }
};

class Player : public Character {
private:
    vector<KeyInv> keys; 
    int x, y;
public:
    Player(int h, int s, int d) : Character(h, s, d), x(0), y(0) {}

    string getKeysDisplay() const {
        if (keys.empty()) return "None";
        string res = "";
        bool hasKeys = false;
        for (int i = 0; i < keys.size(); ++i) {
            if (keys[i].count > 0) {
                res += keys[i].type + ":" + to_string(keys[i].count) + " ";
                hasKeys = true;
            }
        }
        return hasKeys ? res : "None";
    }

    void addKey(const string& type) {
        for (int i = 0; i < keys.size(); ++i) {
            if (keys[i].type == type) {
                keys[i].count++;
                return;
            }
        }
        KeyInv newKey;
        newKey.type = type;
        newKey.count = 1;
        keys.push_back(newKey);
    }

    bool useKey(const string& type) {
        for (int i = 0; i < keys.size(); ++i) {
            if (keys[i].type == type && keys[i].count > 0) {
                keys[i].count--;
                return true;
            }
        }
        return false;
    }

    void setPos(int nx, int ny) { x = nx; y = ny; }
    int getX() const { return x; }
    int getY() const { return y; }
};

struct Dungeon {
    string name;
    int rows;
    int cols;
    vector<vector<Tile>> grid;
};


// GAME ENGINE

class GameManager {
private:
    vector<Dungeon> dungeons;

    // Get raw input without converting to lowercase to make file paths easier to handle
    string getRawStringInput(const string& prompt) {
        string val;
        cout << prompt;
        getline(cin, val);
        return val;
    }

    // Custom lower-case function 
    string toLower(string s) const {
        string res = "";
        for (int i = 0; i < s.length(); i++) {
            if (s[i] >= 'A' && s[i] <= 'Z') {
                res += (char)(s[i] + 32);
            } else {
                res += s[i];
            }
        }
        return res;
    }

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
            case TileType::DAMAGE_FLOOR: return '~';
            default: return '?';
        }
    }

    // Still used for raw stat inputs, but not menus!
    int getIntInput(const string& prompt) {
        string input;
        int val = 0;
        bool valid = false;
        while (!valid) {
            cout << prompt;
            getline(cin, input);
            valid = true;
            bool isNeg = false;
            int start = 0;
            if (input.length() > 0 && input[0] == '-') {
                isNeg = true;
                start = 1;
            }
            if (input.length() == start) valid = false; // Empty or just "-"
            val = 0;
            for (int i = start; i < input.length(); i++) {
                if (input[i] >= '0' && input[i] <= '9') {
                    val = val * 10 + (input[i] - '0');
                } else {
                    valid = false;
                    break;
                }
            }
            if (valid) {
                if (isNeg) val = -val;
                return val;
            }
            cout << "Invalid number.\n";
        }
        return 0;
    }

    string getStringInput(const string& prompt) {
        string val;
        cout << prompt;
        getline(cin, val);
        return toLower(val);
    }

    bool conductBattle(Player& p, Character& e) {
        int pDmg = p.getStats().str - e.getStats().def;
        if (pDmg < 0) pDmg = 0;
        int eDmg = e.getStats().str - p.getStats().def;
        if (eDmg < 0) eDmg = 0;

        cout << "\n--- BATTLE LOG ---\n";
        if (pDmg == 0) {
            cout << "You cannot pierce the enemy's armor! Your attack does 0 damage.\n";
            cout << "You perish in the ensuing counter-attacks.\n";
            p.modifyHealth(-p.getStats().hp); 
            return false;
        }

        while (p.isAlive() && e.isAlive()) {
            e.modifyHealth(-pDmg);
            cout << "> You strike the enemy for " << pDmg << " damage. Enemy HP: " 
                 << (e.getStats().hp > 0 ? e.getStats().hp : 0) << "\n";
            
            if (!e.isAlive()) {
                cout << "Enemy defeated!\n";
                return true;
            }

            p.modifyHealth(-eDmg);
            cout << "> The enemy strikes you for " << eDmg << " damage. Your HP: " 
                 << (p.getStats().hp > 0 ? p.getStats().hp : 0) << "\n";
            
            if (!p.isAlive()) return false;
        }
        return false;
    }

    void inspectTile(Tile t) const {
        cout << "\n[INSPECT] ";
        if (t.type == TileType::EMPTY) cout << "Empty floor. Safe to walk.";
        else if (t.type == TileType::WALL) cout << "A solid wall. Blocks movement.";
        else if (t.type == TileType::GOAL) cout << "The exit! Reach it to win.";
        else if (t.type == TileType::KEY) cout << "A " << t.text << " Key. Pick it up to open " << t.text << " doors.";
        else if (t.type == TileType::DOOR) cout << "A locked " << t.text << " door. Requires a " << t.text << " Key.";
        else if (t.type == TileType::ENEMY) cout << "Enemy Stats -> HP: " << t.hp << " | STR: " << t.str << " | DEF: " << t.def;
        else if (t.type == TileType::HP_POT) cout << "Health Potion. Restores " << t.value << " HP.";
        else if (t.type == TileType::STR_POT) cout << "Strength Potion. Grants " << t.value << " STR.";
        else if (t.type == TileType::DEF_POT) cout << "Defense Potion. Grants " << t.value << " DEF.";
        else if (t.type == TileType::TELEPORTER) cout << "A mysterious teleporter.";
        else if (t.type == TileType::DAMAGE_FLOOR) cout << "Hazard: " << t.text << ". Deals " << t.value << " damage every step!";
        cout << "\n";
    }

    void playDungeonLoop(Dungeon activeDungeon) {
        Player p(50, 5, 2);

        for (int i = 0; i < activeDungeon.rows; i++) {
            for (int j = 0; j < activeDungeon.cols; j++) {
                if (activeDungeon.grid[i][j].type == TileType::PLAYER) {
                    p.setPos(j, i);
                    activeDungeon.grid[i][j].type = TileType::EMPTY;
                }
            }
        }

        string message = "You entered the dungeon!";
        
        cout << "\n=== DUNGEON LEGEND ===\n"
             << "@ : Player       # : Wall           G : Goal\n"
             << "K : Key          X : Door           E : Enemy\n"
             << "H : Health Pot   S : Strength Pot   D : Defense Pot\n"
             << "T : Teleporter   ~ : Damage Floor\n"
             << "Type 'inspect' to look at a tile before stepping on it!!!\n"
             << "======================\n";

        while (true) {
            cout << "\n========================================\n";
            cout << " HP: " << p.getStats().hp << " | STR: " << p.getStats().str
                 << " | DEF: " << p.getStats().def << "\n Keys: [" << p.getKeysDisplay() << "]\n";
            cout << "========================================\n";

            for (int i = 0; i < activeDungeon.rows; i++) {
                for (int j = 0; j < activeDungeon.cols; j++) {
                    if (p.getX() == j && p.getY() == i) {
                        cout << '@';
                    }
                    else {
                        cout << tileToChar(activeDungeon.grid[i][j].type);
                    }
                }
                cout << "\n";
            }
            cout << "\n> " << message << "\n\n";
            message = "";

            string move = getStringInput("Enter move (w/a/s/d) or 'inspect' or 'quit': ");
            int dx = 0, dy = 0;

            if (move == "w") dy = -1;
            else if (move == "s") dy = 1;
            else if (move == "a") dx = -1;
            else if (move == "d") dx = 1;
            else if (move == "quit" || move == "q") {
                cout << "Fleeing the dungeon... Returning to menu.\n";
                return;
            }
            else if (move == "inspect" || move == "i") {
                string dir = getStringInput("Which direction to inspect? (w/a/s/d): ");
                int ix = p.getX(), iy = p.getY();
                if (dir == "w") iy--;
                else if (dir == "s") iy++;
                else if (dir == "a") ix--;
                else if (dir == "d") ix++;
                else {
                    message = "Invalid direction.";
                    continue;
                }
                if (ix < 0 || ix >= activeDungeon.cols || iy < 0 || iy >= activeDungeon.rows) {
                    message = "Nothing but the void there.";
                } else {
                    inspectTile(activeDungeon.grid[iy][ix]);
                }
                continue;
            }
            else {
                message = "Invalid input! Use w/a/s/d, inspect, or quit.";
                continue;
            }

            int nx = p.getX() + dx;
            int ny = p.getY() + dy;

            if (nx < 0 || nx >= activeDungeon.cols || ny < 0 || ny >= activeDungeon.rows) {
                message = "You bumped into the boundaries of the world!";
                continue;
            }

            Tile targetTile = activeDungeon.grid[ny][nx];

            if (targetTile.type == TileType::WALL) {
                message = "You bumped into a wall!";
            }
            else if (targetTile.type == TileType::EMPTY) {
                p.setPos(nx, ny);
                message = "Moved.";
            }
            else if (targetTile.type == TileType::KEY) {
                p.addKey(targetTile.text);
                activeDungeon.grid[ny][nx].type = TileType::EMPTY;
                p.setPos(nx, ny);
                message = "You picked up a " + targetTile.text + " Key!";
            }
            else if (targetTile.type == TileType::DOOR) {
                if (p.useKey(targetTile.text)) {
                    activeDungeon.grid[ny][nx].type = TileType::EMPTY;
                    p.setPos(nx, ny);
                    message = "You unlocked the " + targetTile.text + " door!";
                }
                else {
                    message = "The door is locked. You need a " + targetTile.text + " Key.";
                }
            }
            else if (targetTile.type == TileType::TELEPORTER) {
                p.setPos(targetTile.targetX, targetTile.targetY);
                message = "*ZWOOSH* You stepped into a teleporter and warped!";
            }
            else if (targetTile.type == TileType::HP_POT || targetTile.type == TileType::STR_POT || targetTile.type == TileType::DEF_POT) {
                if (targetTile.type == TileType::HP_POT) { p.modifyHealth(targetTile.value); message = "Drank a Health Potion! (+" + to_string(targetTile.value) + " HP)"; }
                else if (targetTile.type == TileType::STR_POT) { p.addStrength(targetTile.value); message = "Drank a Strength Potion! (+" + to_string(targetTile.value) + " STR)"; }
                else if (targetTile.type == TileType::DEF_POT) { p.addDefense(targetTile.value); message = "Drank a Defense Potion! (+" + to_string(targetTile.value) + " DEF)"; }

                activeDungeon.grid[ny][nx].type = TileType::EMPTY;
                p.setPos(nx, ny);
            }
            else if (targetTile.type == TileType::DAMAGE_FLOOR) {
                p.modifyHealth(-targetTile.value);
                p.setPos(nx, ny); // Player moves, but the tile remains a damage floor
                message = "You stepped on " + targetTile.text + " and took " + to_string(targetTile.value) + " damage!";

                if (!p.isAlive()) {
                    cout << "\n=== GAME OVER ===\nYou succumbed to the environmental hazards.\n\n";
                    return;
                }
            }
            else if (targetTile.type == TileType::ENEMY) {
                Character enemy(targetTile.hp, targetTile.str, targetTile.def);
                cout << "\n[!] BATTLE INITIATED [!]\nEnemy Stats: HP " << targetTile.hp << " | STR " << targetTile.str << " | DEF " << targetTile.def << "\n";

                bool win = conductBattle(p, enemy);
                if (win) {
                    activeDungeon.grid[ny][nx].type = TileType::EMPTY;
                    p.setPos(nx, ny);
                    message = "You defeated the enemy!";
                }
                else {
                    cout << "\n=== GAME OVER ===\nYou perished in battle.\n\n";
                    return;
                }
            }
            else if (targetTile.type == TileType::GOAL) {
                cout << "\n*** VICTORY! ***\nYou reached the Goal and escaped the dungeon!\n\n";
                return;
            }
        }
    }

    void loadDungeonMenu() {
        cout << "\n--- Select a Dungeon ---\n";
        if (dungeons.empty()) {
            cout << "No dungeons currently in memory.\n";
        } else {
            for (int i = 0; i < dungeons.size(); ++i) {
                cout << i + 1 << " - " << dungeons[i].name << "\n";
            }
        }
        cout << "L - Load from file\n";
        cout << "0 - Back\n";

        while (true) {
            string choice = getStringInput("Type the name of the dungeon, its number, 'L' to load file (or '0' for back): ");
            if (choice == "back" || choice == "0") return;
            
            if (choice == "l" || choice == "load") {
                string filepath = getRawStringInput("Enter the filename or full path to the file (with or without .dun): ");
                
                // Automatically append .dun if it's not already there
                if (filepath.length() < 4 || filepath.substr(filepath.length() - 4) != ".dun") {
                    filepath += ".dun";
                }

                Dungeon loadedDungeon;
                if (loadDungeonFromFile(filepath, loadedDungeon)) {
                    dungeons.push_back(loadedDungeon);
                    cout << "Dungeon '" << loadedDungeon.name << "' loaded successfully!\n";
                    playDungeonLoop(loadedDungeon);
                    return;
                }
                continue;
            }
            
            bool isNum = true;
            for(char c : choice) { if(!isdigit(c)) isNum = false; }
            if(isNum && !choice.empty()) {
                int idx = stoi(choice) - 1;
                if(idx >= 0 && idx < dungeons.size()) {
                    playDungeonLoop(dungeons[idx]);
                    return;
                }
            }
            
            for (int i = 0; i < dungeons.size(); ++i) {
                if (toLower(dungeons[i].name) == choice) {
                    playDungeonLoop(dungeons[i]);
                    return;
                }
            }
            cout << "Dungeon not found. Please type the name exactly, or 'L' to load.\n";
        }
    }

    void launchLevelEditor() {
        cout << "\n--- LEVEL EDITOR ---\n";

        string editModeStr = getStringInput("Type 'new' (1) to create or 'edit' (2) to load existing: ");
        
        Dungeon d;
        bool isEditingExisting = false;

        if (editModeStr == "edit" || editModeStr == "2") {
            cout << "\n--- Select Dungeon to Edit ---\n";
            if (dungeons.empty()) {
                cout << "No dungeons currently in memory.\n";
            } else {
                for (int i = 0; i < dungeons.size(); ++i) {
                    cout << i + 1 << " - " << dungeons[i].name << "\n";
                }
            }
            cout << "L - Load from file\n";
            
            bool found = false;
            while (!found) {
                string choice = getStringInput("Type dungeon name, number, or 'L' to load file: ");
                
                if (choice == "l" || choice == "load") {
                    string filepath = getRawStringInput("Enter the filename or full path to the file (with or without .dun): ");
                    
                    if (filepath.length() < 4 || filepath.substr(filepath.length() - 4) != ".dun") {
                        filepath += ".dun";
                    }

                    Dungeon loadedDungeon;
                    if (loadDungeonFromFile(filepath, loadedDungeon)) {
                        d = loadedDungeon;
                        found = true;
                        isEditingExisting = true;
                        cout << "Dungeon '" << loadedDungeon.name << "' loaded for editing!\n";
                        break;
                    }
                    continue;
                }
                
                bool isNum = true;
                for(char c : choice) { if(!isdigit(c)) isNum = false; }
                if(isNum && !choice.empty()) {
                    int idx = stoi(choice) - 1;
                    if(idx >= 0 && idx < dungeons.size()) {
                        d = dungeons[idx];
                        found = true;
                        isEditingExisting = true;
                        break;
                    }
                }
                for (int i = 0; i < dungeons.size(); ++i) {
                    if (toLower(dungeons[i].name) == choice) {
                        d = dungeons[i];
                        found = true;
                        isEditingExisting = true;
                        break;
                    }
                }
                if(!found) cout << "Not found. Please type a valid name, number, or 'L' to load.\n";
            }
        }

        if (editModeStr == "new" || editModeStr == "1") {
            d.rows = getIntInput("Enter number of rows (1-50): ");
            d.cols = getIntInput("Enter number of columns (1-50): ");
            d.grid = vector<vector<Tile>>(d.rows, vector<Tile>(d.cols));
        }

        while (true) {
            cout << "\n    ";
            for (int j = 1; j <= d.cols; ++j) cout << j % 10 << " ";
            cout << "\n";
            for (int i = 0; i < d.rows; ++i) {
                cout << (i + 1 < 10 ? " " : "") << i + 1 << "| ";
                for (int j = 0; j < d.cols; ++j) {
                    cout << tileToChar(d.grid[i][j].type) << " ";
                }
                cout << "\n";
            }

            cout << "\n--- EDITOR TOOLS ---\n"
                 << "1 - single    2 - rect\n"
                 << "3 - hollow    4 - circle\n"
                 << "5 - inspect   6 - save\n";

            string tool = getStringInput("Type tool name or number: ");

            if (tool == "save" || tool == "6") {
                int pCount = 0, gCount = 0;
                for (int i = 0; i < d.rows; i++) {
                    for (int j = 0; j < d.cols; j++) {
                        if (d.grid[i][j].type == TileType::PLAYER) pCount++;
                        if (d.grid[i][j].type == TileType::GOAL) gCount++;
                    }
                }
                if (pCount != 1) {
                    cout << "\n[!] Cannot save: Must have exactly ONE Player (@).\n";
                    continue;
                }
                if (gCount < 1) {
                    cout << "\n[!] Cannot save: Must have at least ONE Goal (G).\n";
                    continue;
                }

                string newName = getStringInput("Enter dungeon name: ");
                d.name = newName;

                bool updated = false;
                for (int i = 0; i < dungeons.size(); i++) {
                    if (toLower(dungeons[i].name) == toLower(d.name)) {
                        dungeons[i] = d;
                        updated = true;
                        cout << "Updated existing dungeon!\n";
                        break;
                    }
                }
                if (!updated) {
                    dungeons.push_back(d);
                    cout << "Saved new dungeon!\n";
                }
                string saveLocal = getStringInput("Would you also like to save this to a local .dun file? (y/n): ");
                if (saveLocal == "y" || saveLocal == "yes") {
                    this->saveDungeonToFile(d);
                }
                break;
            }

            if (tool == "inspect" || tool == "5") {
                int row = getIntInput("Enter row: ") - 1;
                int col = getIntInput("Enter col: ") - 1;
                if (row >= 0 && row < d.rows && col >= 0 && col < d.cols) {
                    Tile t = d.grid[row][col];
                    cout << "\n=== TILE INFO ===\n";
                    cout << "Location: Row " << row + 1 << " Col " << col + 1 << "\n";
                    cout << "Base Type: " << tileToChar(t.type) << "\n";
                    inspectTile(t);
                }
                continue;
            }

            if (tool == "single" || tool == "rect" || tool == "hollow" || tool == "circle" || tool == "1" || tool == "2" || tool == "3" || tool == "4") {
                cout << "\nAvailable Objects: empty (0), wall (1), player (2), goal (3), key (4), door (5), enemy (6), health (7), strength (8), defense (9), teleporter (10), damage (11)\n";
                string choice = getStringInput("Type object to place (name or number): ");
                
                Tile newTile;
                if (choice == "empty" || choice == "0") newTile.type = TileType::EMPTY;
                else if (choice == "wall" || choice == "1") newTile.type = TileType::WALL;
                else if (choice == "player" || choice == "2") newTile.type = TileType::PLAYER;
                else if (choice == "goal" || choice == "3") newTile.type = TileType::GOAL;
                else if (choice == "key" || choice == "4") newTile.type = TileType::KEY;
                else if (choice == "door" || choice == "5") newTile.type = TileType::DOOR;
                else if (choice == "enemy" || choice == "6") newTile.type = TileType::ENEMY;
                else if (choice == "health" || choice == "7") newTile.type = TileType::HP_POT;
                else if (choice == "strength" || choice == "8") newTile.type = TileType::STR_POT;
                else if (choice == "defense" || choice == "9") newTile.type = TileType::DEF_POT;
                else if (choice == "teleporter" || choice == "10") newTile.type = TileType::TELEPORTER;
                else if (choice == "damage" || choice == "11") newTile.type = TileType::DAMAGE_FLOOR;
                else {
                    cout << "Invalid object!\n";
                    continue;
                }

                // Collect custom values
                if (newTile.type == TileType::ENEMY) {
                    newTile.hp = getIntInput("Enter HP: ");
                    newTile.str = getIntInput("Enter STR: ");
                    newTile.def = getIntInput("Enter DEF: ");
                }
                if (newTile.type == TileType::HP_POT) newTile.value = getIntInput("Enter HP restore amount: ");
                if (newTile.type == TileType::STR_POT) newTile.value = getIntInput("Enter STR boost amount: ");
                if (newTile.type == TileType::DEF_POT) newTile.value = getIntInput("Enter DEF boost amount: ");
                if (newTile.type == TileType::KEY || newTile.type == TileType::DOOR) {
                    newTile.text = getStringInput("Enter lock type (e.g., blue, red): ");
                }
                if (newTile.type == TileType::DAMAGE_FLOOR) {
                    string dType = getStringInput("Enter floor type (lava/spikes/poison): ");
                    newTile.text = dType;
                    if (dType == "lava") newTile.value = 5;
                    else if (dType == "spikes") newTile.value = 3;
                    else newTile.value = 1; // poison default
                    cout << "Assigned " << newTile.value << " damage per step for " << dType << ".\n";
                }

                if (newTile.type == TileType::TELEPORTER) {
                    cout << "Teleporters must be placed in pairs.\n";
                    int r1 = getIntInput("Row for Tele A: ") - 1;
                    int c1 = getIntInput("Col for Tele A: ") - 1;
                    int r2 = getIntInput("Row for Tele B: ") - 1;
                    int c2 = getIntInput("Col for Tele B: ") - 1;
                    if(r1 >= 0 && r1 < d.rows && c1 >= 0 && c1 < d.cols && r2 >= 0 && r2 < d.rows && c2 >= 0 && c2 < d.cols) {
                        d.grid[r1][c1].type = TileType::TELEPORTER;
                        d.grid[r1][c1].targetX = c2;
                        d.grid[r1][c1].targetY = r2;

                        d.grid[r2][c2].type = TileType::TELEPORTER;
                        d.grid[r2][c2].targetX = c1;
                        d.grid[r2][c2].targetY = r1;
                        cout << "Paired!\n";
                    }
                    continue; 
                }

                auto placeTile = [&](int i, int j) {
                    if (i >= 0 && i < d.rows && j >= 0 && j < d.cols) {
                        d.grid[i][j] = newTile;
                    }
                };

                if (tool == "single" || tool == "1") {
                    int r = getIntInput("Row: ") - 1;
                    int c = getIntInput("Col: ") - 1;
                    placeTile(r, c);
                }
                else if (tool == "rect" || tool == "hollow" || tool == "2" || tool == "3") {
                    int r1 = getIntInput("Corner 1 Row: ") - 1;
                    int c1 = getIntInput("Corner 1 Col: ") - 1;
                    int r2 = getIntInput("Corner 2 Row: ") - 1;
                    int c2 = getIntInput("Corner 2 Col: ") - 1;

                    int minR = min(r1, r2), maxR = max(r1, r2);
                    int minC = min(c1, c2), maxC = max(c1, c2);

                    for (int i = minR; i <= maxR; ++i) {
                        for (int j = minC; j <= maxC; ++j) {
                            if (tool == "rect" || tool == "2") placeTile(i, j);
                            else {
                                if (i == minR || i == maxR || j == minC || j == maxC) placeTile(i, j);
                            }
                        }
                    }
                }
                else if (tool == "circle" || tool == "4") {
                    int rc = getIntInput("Center Row: ") - 1;
                    int cc = getIntInput("Center Col: ") - 1;
                    int rad = getIntInput("Radius: ");
                    for (int i = 0; i < d.rows; ++i) {
                        for (int j = 0; j < d.cols; ++j) {
                            if ((i - rc) * (i - rc) + (j - cc) * (j - cc) <= rad * rad) {
                                placeTile(i, j);
                            }
                        }
                    }
                }
            }
        }
    }

    bool loadDungeonFromFile(const string& filepath, Dungeon& outDungeon) {
        ifstream inFile(filepath);
        if (!inFile.is_open()) {
            cout << "[!] Error: Could not open file '" << filepath << "'. Ensure the path is correct.\n";
            return false;
        }

        string name;
        getline(inFile, name);
        
        int r, c;
        if (!(inFile >> r >> c)) {
            cout << "[!] Error: Invalid file format.\n";
            return false;
        }

        outDungeon.name = name;
        outDungeon.rows = r;
        outDungeon.cols = c;
        outDungeon.grid = vector<vector<Tile>>(r, vector<Tile>(c));

        for (int i = 0; i < r; ++i) {
            for (int j = 0; j < c; ++j) {
                int typeInt;
                Tile t;
                string text;
                if (!(inFile >> typeInt >> t.hp >> t.str >> t.def >> t.value >> t.targetX >> t.targetY >> text)) {
                    cout << "[!] Error: Invalid file format while reading tiles.\n";
                    return false;
                }
                t.type = static_cast<TileType>(typeInt);
                if (text == "_") {
                    t.text = "";
                } else {
                    for (int k = 0; k < text.length(); ++k) {
                        if (text[k] == '_') text[k] = ' ';
                    }
                    t.text = text;
                }
                outDungeon.grid[i][j] = t;
            }
        }
        inFile.close();
        return true;
    }

    void saveDungeonToFile(const Dungeon& d) {
        // 1. Create filename from the dungeon name with a .dun extension
        string filename = d.name + ".dun";

        // Ask the user for a path in their storage to save the file
        string path = getRawStringInput("Enter the folder path to save to storage (e.g. C:\\Dungeons) or leave blank for default: ");
        string fullPath = filename;
        if (!path.empty()) {
            if (path.back() != '\\' && path.back() != '/') {
                path += "\\";
            }
            fullPath = path + filename;
        }

        // 2. Open the file for writing
        ofstream outFile(fullPath);
        if (!outFile.is_open()) {
            cout << "[!] Error: Could not open file '" << fullPath << "' for writing. Ensure the directory exists.\n";
            return;
        }

        //3. write the dungeon name,rows,cols
        outFile << d.name << "\n";
        outFile << d.rows << " " << d.cols << "\n";

        // 4. Write all tile data for the entire grid
        for (int i = 0; i < d.rows; ++i) {
            for (int j = 0; j < d.cols; ++j) {
                const Tile& t = d.grid[i][j];
                string sanitized_text = t.text;

                // Replace spaces with underscores for easier file parsing later
                for (int k = 0; k < sanitized_text.length(); ++k) {
                    if (sanitized_text[k] == ' ') sanitized_text[k] = '_';
                }
                if (sanitized_text.empty()) sanitized_text = "_"; // placeholder for empty text

                outFile << static_cast<int>(t.type) << " "
                        << t.hp << " "
                        << t.str << " "
                        << t.def << " "
                        << t.value << " "
                        << t.targetX << " "
                        << t.targetY << " "
                        << sanitized_text << "\n";
            }
        }
        outFile.close();
        cout << "Dungeon was also saved to storage: " << fullPath << "\n";
    }

    void loadDefaultDungeons() {
        Dungeon d1;
        d1.name = "MUNYLU";
        d1.rows = 10; d1.cols = 10;
        d1.grid = vector<vector<Tile>>(d1.rows, vector<Tile>(d1.cols));

        for (int i = 0; i < d1.rows; ++i) {
            for (int j = 0; j < d1.cols; ++j) {
                if (i == 0 || i == d1.rows - 1 || j == 0 || j == d1.cols - 1)
                    d1.grid[i][j].type = TileType::WALL;
            }
        }

        d1.grid[1][1].type = TileType::PLAYER;
        d1.grid[1][8].type = TileType::GOAL;

        d1.grid[2][3].type = TileType::KEY;
        d1.grid[2][3].text = "red";

        d1.grid[3][5].type = TileType::DOOR;
        d1.grid[3][5].text = "red";

        d1.grid[4][4].type = TileType::ENEMY;
        d1.grid[4][4].hp = 20; d1.grid[4][4].str = 5; d1.grid[4][4].def = 2;

        d1.grid[3][1].type = TileType::HP_POT;
        d1.grid[3][1].value = 25;

        d1.grid[5][2].type = TileType::STR_POT;
        d1.grid[5][2].value = 3;

        d1.grid[7][4].type = TileType::DEF_POT;
        d1.grid[7][4].value = 4;

        dungeons.push_back(d1);

        Dungeon d2;
        d2.name = "Georgina's Trial";
        d2.rows = 7; d2.cols = 7;
        d2.grid = vector<vector<Tile>>(d2.rows, vector<Tile>(d2.cols));

        for (int i = 0; i < d2.rows; ++i) {
            for (int j = 0; j < d2.cols; ++j) {
                if (i == 0 || i == d2.rows - 1 || j == 0 || j == d2.cols - 1)
                    d2.grid[i][j].type = TileType::WALL;
            }
        }

        d2.grid[1][1].type = TileType::PLAYER;

        d2.grid[1][3].type = TileType::ENEMY;
        d2.grid[1][3].hp = 15; d2.grid[1][3].str = 6; d2.grid[1][3].def = 1;

        d2.grid[2][3].type = TileType::WALL;
        d2.grid[2][5].type = TileType::WALL;

        d2.grid[3][1].type = TileType::KEY;
        d2.grid[3][1].text = "blue";

        d2.grid[3][3].type = TileType::DOOR;
        d2.grid[3][3].text = "blue";

        d2.grid[3][5].type = TileType::GOAL;

        d2.grid[4][1].type = TileType::DAMAGE_FLOOR; d2.grid[4][1].text = "lava"; d2.grid[4][1].value = 5;
        d2.grid[4][2].type = TileType::DAMAGE_FLOOR; d2.grid[4][2].text = "lava"; d2.grid[4][2].value = 5;
        d2.grid[4][3].type = TileType::DAMAGE_FLOOR; d2.grid[4][3].text = "lava"; d2.grid[4][3].value = 5;

        d2.grid[5][3].type = TileType::DEF_POT;
        d2.grid[5][3].value = 5;

        d2.grid[5][5].type = TileType::STR_POT;
        d2.grid[5][5].value = 3;

        d2.grid[1][5].type = TileType::TELEPORTER;
        d2.grid[1][5].targetX = 1; d2.grid[1][5].targetY = 5;
        
        d2.grid[5][1].type = TileType::TELEPORTER;
        d2.grid[5][1].targetX = 5; d2.grid[5][1].targetY = 1;

        dungeons.push_back(d2);
    }

public:
    GameManager() {
        loadDefaultDungeons();
    }

    void run() {
        while (true) {
            cout << "\nWelcome to Magic Tower:\n";
            cout << " 1 - play\n";
            cout << " 2 - editor\n";
            cout << " 3 - exit\n";

            string choice = getStringInput("Select an option: ");
            if (choice == "play" || choice == "1") {
                loadDungeonMenu();
            }
            else if (choice == "editor" || choice == "2") {
                launchLevelEditor();
            }
            else if (choice == "exit" || choice == "3") {
                cout << "Exiting program. Goodbye!\n";
                break;
            } else {
                cout << "Invalid option. Type 'play', 'editor', 'exit' or their number.\n";
            }
        }
    }
};

// main
int main() {
    GameManager game;
    game.run();
    return 0;
}
