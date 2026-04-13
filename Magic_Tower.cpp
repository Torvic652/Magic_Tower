#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <map>
#include <cmath> // Needed for circle distance calculations

using namespace std;

// ==========================================
// ENUMS & STRUCTS
// ==========================================

enum class TileType {
    EMPTY, WALL, PLAYER, GOAL, KEY, DOOR, ENEMY, HP_POT, STR_POT, DEF_POT, TELEPORTER, DAMAGE_FLOOR
};

struct Stats {
    int hp;
    int str;
    int def;
};


// CLASSES

class Character {
protected:
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
    map<string, int> keys; // Allows for colored keys
    int x, y;
    string name;
public:
    Player(int h, int s, int d) : Character(h, s, d), x(0), y(0) {}

    string getKeysDisplay() const {
        if (keys.empty()) return "None";
        string res = "";
        for (auto it = keys.begin(); it != keys.end(); ++it) {
            const string& type = it->first;
            int count = it->second;
            if (count > 0) res += type + ":" + to_string(count) + " ";
        }
        return res.empty() ? "None" : res;
    }

    void addKey(const string& type) { keys[type]++; }
    bool useKey(const string& type) {
        if (keys[type] > 0) {
            keys[type]--;
            return true;
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
    vector<vector<TileType>> grid;

    // Maps a coordinate {x, y} to its specific attributes
    map<pair<int, int>, pair<int, int>> teleporters;
    map<pair<int, int>, Stats> enemyStats;
    map<pair<int, int>, int> potionValues;
    map<pair<int, int>, string> lockTypes; // For keys and doors
    map<pair<int, int>, string> damageFloorTypes; // "lava", "spikes", "poison"
};


// GAME ENGINE

class GameManager {
private:
    vector<Dungeon> dungeons;

    // Convert tile type to display character
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

    // Get integer input with validation 
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

    // Get string input for easier command
    string getLowerStrInput(const string& prompt) {
        string val;
        cout << prompt;
        getline(cin, val);
        transform(val.begin(), val.end(), val.begin(), ::tolower);
        return val;
    }

    // Simple battle system with turn logs
    bool conductBattle(Player& p, Character& e) {
        int pDmg = max(0, p.getStats().str - e.getStats().def);
        int eDmg = max(0, e.getStats().str - p.getStats().def);

        cout << "\n--- BATTLE LOG ---\n";
        if (pDmg == 0) {
            cout << "You cannot pierce the enemy's armor! You flee the battle.\n";
            p.modifyHealth(-p.getStats().hp); // Instant death if you can't damage them (classic mechanic)
            return false;
        }

        while (p.isAlive() && e.isAlive()) {
            e.modifyHealth(-pDmg);
            cout << "> You strike the enemy for " << pDmg << " damage. Enemy HP: " << max(0, e.getStats().hp) << "\n";
            if (!e.isAlive()) {
                cout << "Enemy defeated!\n";
                return true;
            }

            p.modifyHealth(-eDmg);
            cout << "> The enemy strikes you for " << eDmg << " damage. Your HP: " << max(0, p.getStats().hp) << "\n";
            if (!p.isAlive()) return false;
        }
        return false;
    }

    // Main game loop for playing the dungeon
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
                << " | DEF: " << p.getStats().def << "\n Keys: [" << p.getKeysDisplay() << "]\n";
            cout << "========================================\n";
            cout << "Legend: @=Player #=Wall G=Goal K=Key X=Door E=Enemy\n"
                << "        H/S/D=Potions T=Teleporter ~=Damage Floor\n\n";

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

            string move = getLowerStrInput("Enter move (W/A/S/D, Up/Down/Left/Right) or Quit/Q: ");
            int dx = 0, dy = 0;

            if (move == "w" || move == "up" || move == "u") dy = -1;
            else if (move == "s" || move == "down") dy = 1;
            else if (move == "a" || move == "left" || move == "l") dx = -1;
            else if (move == "d" || move == "right" || move == "r") dx = 1;
            else if (move == "quit" || move == "q") {
                cout << "Returning to main menu...\n";
                return;
            }
            else {
                message = "Invalid input! Use W/A/S/D or directions.";
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
                string kType = activeDungeon.lockTypes.count({ nx, ny }) ? activeDungeon.lockTypes[{nx, ny}] : "default";
                p.addKey(kType);
                activeDungeon.grid[ny][nx] = TileType::EMPTY;
                p.setPos(nx, ny);
                message = "You picked up a " + kType + " Key!";
            }
            else if (targetTile == TileType::DOOR) {
                string dType = activeDungeon.lockTypes.count({ nx, ny }) ? activeDungeon.lockTypes[{nx, ny}] : "default";
                if (p.useKey(dType)) {
                    activeDungeon.grid[ny][nx] = TileType::EMPTY;
                    p.setPos(nx, ny);
                    message = "You unlocked the " + dType + " door!";
                }
                else {
                    message = "The door is locked. You need a " + dType + " Key.";
                }
            }
            else if (targetTile == TileType::TELEPORTER) {
                auto it = activeDungeon.teleporters.find({ nx, ny });
                if (it != activeDungeon.teleporters.end()) {
                    p.setPos(it->second.first, it->second.second);
                    message = "*ZWOOSH* You stepped into a teleporter and warped!";
                }
                else {
                    p.setPos(nx, ny);
                    message = "This teleporter seems broken...";
                }
            }
            else if (targetTile == TileType::HP_POT || targetTile == TileType::STR_POT || targetTile == TileType::DEF_POT) {
                int val = activeDungeon.potionValues.count({ nx, ny }) ? activeDungeon.potionValues[{nx, ny}] : (targetTile == TileType::HP_POT ? 20 : 2);

                if (targetTile == TileType::HP_POT) { p.modifyHealth(val); message = "Drank a Health Potion! (+" + to_string(val) + " HP)"; }
                else if (targetTile == TileType::STR_POT) { p.addStrength(val); message = "Drank a Strength Potion! (+" + to_string(val) + " STR)"; }
                else if (targetTile == TileType::DEF_POT) { p.addDefense(val); message = "Drank a Defense Potion! (+" + to_string(val) + " DEF)"; }

                activeDungeon.grid[ny][nx] = TileType::EMPTY;
                p.setPos(nx, ny);
            }
            else if (targetTile == TileType::DAMAGE_FLOOR) {
                string fType = activeDungeon.damageFloorTypes.count({ nx, ny }) ? activeDungeon.damageFloorTypes[{nx, ny}] : "lava";
                int dmg = 5; // Default for lava
                if (fType == "spikes") dmg = 3;
                else if (fType == "poison") dmg = 1;

                p.modifyHealth(-dmg);
                p.setPos(nx, ny); // Player moves, but tile DOES NOT turn to EMPTY
                message = "You stepped on " + fType + " and took " + to_string(dmg) + " damage!";

                if (!p.isAlive()) {
                    cout << "\n=== GAME OVER ===\nYou succumbed to the environmental hazards.\n\n";
                    return;
                }
            }
            else if (targetTile == TileType::ENEMY) {
                Stats eStats = activeDungeon.enemyStats.count({ nx, ny }) ? activeDungeon.enemyStats[{nx, ny}] : Stats{ 20, 5, 2 };
                Character enemy(eStats.hp, eStats.str, eStats.def);

                cout << "\n[!] BATTLE INITIATED [!]\nEnemy Stats: HP " << eStats.hp << " | STR " << eStats.str << " | DEF " << eStats.def << "\n";

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

    // Menu to select dungeon to play
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
        if (choice <= (int)dungeons.size()) {
            playDungeonLoop(dungeons[choice - 1]);
        }
    }

    // Level editor with tools
    void launchLevelEditor() {
        cout << "\n--- LEVEL EDITOR ---\n";

        int editMode = getIntInput("1) Create New Dungeon\n2) Edit Existing Dungeon\nSelect mode: ", 1, 2);

        Dungeon d;
        int dIndex = -1;
        int r, c;

        if (editMode == 2) {
            if (dungeons.empty()) {
                cout << "No existing dungeons found. Starting fresh.\n";
                editMode = 1;
            }
            else {
                for (size_t i = 0; i < dungeons.size(); ++i) {
                    cout << i + 1 << ") " << dungeons[i].name << "\n";
                }
                dIndex = getIntInput("Select dungeon to edit: ", 1, dungeons.size()) - 1;
                d = dungeons[dIndex];
                r = d.rows;
                c = d.cols;
            }
        }

        if (editMode == 1) {
            r = getIntInput("Enter number of rows (1-50): ", 1, 50);
            c = getIntInput("Enter number of columns (1-50): ", 1, 50);
            d.rows = r; d.cols = c;
            d.grid = vector<vector<TileType>>(r, vector<TileType>(c, TileType::EMPTY));
        }

        // Helper lambda to clear data safely when overwriting a tile
        auto eraseTileData = [&](int px, int py) {
            d.enemyStats.erase({ px, py });
            d.potionValues.erase({ px, py });
            d.lockTypes.erase({ px, py });
            d.damageFloorTypes.erase({ px, py });
            d.teleporters.erase({ px, py });
            };

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
                << "5) Inspect Object      6) SAVE DUNGEON & EXIT\n";

            int tool = getIntInput("Select a tool (1-6): ", 1, 6);

            if (tool == 6) {
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

                if (editMode == 1) {
                    cout << "Enter a name for your new dungeon: ";
                    getline(cin, d.name);
                    dungeons.push_back(d);
                }
                else {
                    cout << "Enter a name (leave blank to keep '" << d.name << "'): ";
                    string newName;
                    getline(cin, newName);
                    if (!newName.empty()) d.name = newName;
                    dungeons[dIndex] = d;
                }

                cout << "Dungeon saved successfully! Added to Play menu.\n";
                break;
            }

            if (tool == 5) { // Inspect Object
                int row = getIntInput("Enter row to inspect: ", 1, r) - 1;
                int col = getIntInput("Enter col to inspect: ", 1, c) - 1;
                TileType t = d.grid[row][col];

                cout << "\n=== TILE INFO ===\n";
                cout << "Location: Row " << row + 1 << " Col " << col + 1 << "\n";
                cout << "Base Type: " << tileToChar(t) << "\n";

                if (t == TileType::ENEMY && d.enemyStats.count({ col, row })) {
                    Stats s = d.enemyStats[{col, row}];
                    cout << "Enemy Stats -> HP: " << s.hp << " STR: " << s.str << " DEF: " << s.def << "\n";
                }
                else if ((t == TileType::KEY || t == TileType::DOOR) && d.lockTypes.count({ col, row })) {
                    cout << "Color/Type -> " << d.lockTypes[{col, row}] << "\n";
                }
                else if ((t == TileType::HP_POT || t == TileType::STR_POT || t == TileType::DEF_POT) && d.potionValues.count({ col, row })) {
                    cout << "Potion Value -> +" << d.potionValues[{col, row}] << "\n";
                }
                else if (t == TileType::DAMAGE_FLOOR && d.damageFloorTypes.count({ col, row })) {
                    cout << "Floor Type -> " << d.damageFloorTypes[{col, row}] << "\n";
                }
                else if (t == TileType::TELEPORTER && d.teleporters.count({ col, row })) {
                    cout << "Linked To -> Col " << d.teleporters[{col, row}].first + 1
                        << " Row " << d.teleporters[{col, row}].second + 1 << "\n";
                }
                else {
                    cout << "No special properties applied.\n";
                }
                continue;
            }

            cout << "\nAvailable Objects:\n"
                << "1) Empty Space   2) Wall            3) Player\n"
                << "4) Goal          5) Key             6) Locked Door\n"
                << "7) Enemy         8) Health Potion   9) Strength Potion\n"
                << "10) Defense Pot  11) Teleporter     12) Damage Floor\n\n";

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
            else if (choice == "12" || choice == "damage") selectedTile = TileType::DAMAGE_FLOOR;
            else {
                cout << "Invalid object choice!\n";
                continue;
            }

            // GATHER CUSTOM PROPERTIES
            Stats enemyStat{ 20, 5, 2 };
            if (selectedTile == TileType::ENEMY) {
                cout << "-- Enemy Configuration --\n";
                enemyStat.hp = getIntInput("Enter Enemy HP: ", 1);
                enemyStat.str = getIntInput("Enter Enemy STR: ", 0);
                enemyStat.def = getIntInput("Enter Enemy DEF: ", 0);
            }
            int potVal = 2;
            if (selectedTile == TileType::HP_POT) potVal = getIntInput("Enter HP to restore: ", 1);
            if (selectedTile == TileType::STR_POT || selectedTile == TileType::DEF_POT) potVal = getIntInput("Enter stat boost amount: ", 1);

            string lockType = "default";
            if (selectedTile == TileType::KEY || selectedTile == TileType::DOOR) {
                cout << "Enter color/type (e.g. blue, red): ";
                getline(cin, lockType);
            }
            string floorType = "lava";
            if (selectedTile == TileType::DAMAGE_FLOOR) {
                cout << "Enter damage floor type (lava, spikes, poison): ";
                getline(cin, floorType);
            }

            // TELEPORTER LOGIC (Forces Single Object placement to pair correctly)
            if (selectedTile == TileType::TELEPORTER) {
                cout << "\n[Teleporters must be placed in pairs!]\n";
                int r1 = getIntInput("Enter Row for Teleporter A: ", 1, r) - 1;
                int c1 = getIntInput("Enter Col for Teleporter A: ", 1, c) - 1;
                int r2 = getIntInput("Enter Row for Teleporter B: ", 1, r) - 1;
                int c2 = getIntInput("Enter Col for Teleporter B: ", 1, c) - 1;

                eraseTileData(c1, r1);
                eraseTileData(c2, r2);

                d.grid[r1][c1] = TileType::TELEPORTER;
                d.grid[r2][c2] = TileType::TELEPORTER;

                d.teleporters[{c1, r1}] = { c2, r2 };
                d.teleporters[{c2, r2}] = { c1, r1 };
                cout << "Teleporters linked and placed!\n";
            }
            // STANDARD SHAPE TOOLS
            else {
                auto placeTile = [&](int i, int j) {
                    eraseTileData(j, i); // Cleanup old custom properties
                    d.grid[i][j] = selectedTile;

                    if (selectedTile == TileType::ENEMY) d.enemyStats[{j, i}] = enemyStat;
                    if (selectedTile == TileType::HP_POT || selectedTile == TileType::STR_POT || selectedTile == TileType::DEF_POT) d.potionValues[{j, i}] = potVal;
                    if (selectedTile == TileType::KEY || selectedTile == TileType::DOOR) d.lockTypes[{j, i}] = lockType;
                    if (selectedTile == TileType::DAMAGE_FLOOR) d.damageFloorTypes[{j, i}] = floorType;
                    };

                if (tool == 1) { // Single Object
                    int row = getIntInput("Enter row: ", 1, r) - 1;
                    int col = getIntInput("Enter col: ", 1, c) - 1;
                    placeTile(row, col);
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
                                placeTile(i, j); // Filled
                            }
                            else {
                                if (i == minR || i == maxR || j == minC || j == maxC) {
                                    placeTile(i, j); // Hollow
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
                            if ((i - rc) * (i - rc) + (j - cc) * (j - cc) <= rad * rad) {
                                placeTile(i, j);
                            }
                        }
                    }
                }
            }
        }
    }

    void loadDefaultDungeons() {
        Dungeon d2;
        d2.name = "Georgina's Trial";
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
        d2.enemyStats[{3, 1}] = { 15, 6, 1 }; // Slightly weaker enemy

        d2.grid[1][5] = TileType::TELEPORTER;

        d2.grid[2][3] = TileType::WALL;
        d2.grid[2][5] = TileType::WALL;

        d2.grid[3][1] = TileType::KEY;
        d2.lockTypes[{1, 3}] = "blue"; // Blue key

        d2.grid[3][3] = TileType::DOOR;
        d2.lockTypes[{3, 3}] = "blue"; // Blue door

        d2.grid[3][5] = TileType::GOAL;

        // Lava river
        d2.grid[4][1] = TileType::DAMAGE_FLOOR;
        d2.grid[4][2] = TileType::DAMAGE_FLOOR;
        d2.grid[4][3] = TileType::DAMAGE_FLOOR;
        d2.damageFloorTypes[{1, 4}] = "lava";
        d2.damageFloorTypes[{2, 4}] = "lava";
        d2.damageFloorTypes[{3, 4}] = "lava";

        d2.grid[5][1] = TileType::TELEPORTER;
        d2.grid[5][3] = TileType::DEF_POT;
        d2.potionValues[{3, 5}] = 5; // Extra strong def potion
        d2.grid[5][5] = TileType::STR_POT;
        d2.potionValues[{5, 5}] = 3;

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
            cout << " 2) Level Editor\n";
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