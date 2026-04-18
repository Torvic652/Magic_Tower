#include <iostream>
#include <vector>
#include <string>
#include <algorithm>//transform
#include <cctype>
#include <map>
#include <cmath>
#include <thread>//for sleep
#include <chrono>//for sleep
#include <cstdlib>//random
#include <ctime>//seed random
#include <climits>//min and max

using namespace std;

// Cross-platform millisecond sleep
void sleepMs(int ms) {
    this_thread::sleep_for(chrono::milliseconds(ms));
}


//  ENUMS & STRUCTS


enum class TileType {
    EMPTY, WALL, PLAYER, GOAL, KEY, DOOR, ENEMY,
    HP_POT, STR_POT, DEF_POT, TELEPORTER,
    DAMAGE_FLOOR, SWITCH, SWITCH_WALL, SWITCH_DAMAGE_FLOOR
};

struct Stats { int hp, str, def; };


//  CLASSES

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
    map<string, int> keys;  // Colored key inventory
    int x, y;
public:
    Player(int h, int s, int d) : Character(h, s, d), x(0), y(0) {}

    string getKeysDisplay() const {
        if (keys.empty()) return "None";
        string res;
        for (const auto& kv : keys)
            if (kv.second > 0) res += kv.first + "x" + to_string(kv.second) + " ";
        return res.empty() ? "None" : res;
    }

    void addKey(const string& t) { keys[t]++; }

    // BUG FIX #5: Use find() to avoid auto-inserting zero entries
    bool useKey(const string& t) {
        auto it = keys.find(t);
        if (it != keys.end() && it->second > 0) { it->second--; return true; }
        return false;
    }

    void setPos(int nx, int ny) { x = nx; y = ny; }
    int getX() const { return x; }
    int getY() const { return y; }
};

struct Dungeon {
    string name;
    int rows, cols;
    vector<vector<TileType>> grid;

    // Per-tile attribute maps — all keyed by {col, row} = {x, y}
    map<pair<int, int>, pair<int, int>> teleporters;
    map<pair<int, int>, Stats>         enemyStats;
    map<pair<int, int>, int>           potionValues;
    map<pair<int, int>, string>        lockTypes;        // keys and doors
    map<pair<int, int>, string>        damageFloorTypes; // "lava","spikes","poison"

    // Switch system
    map<pair<int, int>, int> switchIndices;
    map<pair<int, int>, int> switchWallIndices;
    map<pair<int, int>, int> switchDamageFloorIndices;
    map<int, bool>          switchStates;   // true = active (walls block / floors damage)
};


//  GAME MANAGER

class GameManager {
private:
    vector<Dungeon> dungeons;


    char tileToChar(TileType t) const {
        switch (t) {
        case TileType::EMPTY:               return ' ';
        case TileType::WALL:                return '#';
        case TileType::PLAYER:              return '@';
        case TileType::GOAL:                return 'G';
        case TileType::KEY:                 return 'K';
        case TileType::DOOR:                return 'X';
        case TileType::ENEMY:               return 'E';
        case TileType::HP_POT:              return 'H';
        case TileType::STR_POT:             return 'S';
        case TileType::DEF_POT:             return 'D';
        case TileType::TELEPORTER:          return 'T';
        case TileType::DAMAGE_FLOOR:        return '~';
        case TileType::SWITCH:              return 'O';
        case TileType::SWITCH_WALL:         return 'W';
        case TileType::SWITCH_DAMAGE_FLOOR: return 'F';
        default:                            return '?';
        }
    }

    // Render the dungeon grid, respecting switch states
    void printGrid(const Dungeon& d, const Player& p) const {
        for (int i = 0; i < d.rows; i++) {
            for (int j = 0; j < d.cols; j++) {
                if (p.getX() == j && p.getY() == i) { cout << '@'; continue; }
                TileType t = d.grid[i][j];
                if (t == TileType::SWITCH_WALL && !isSwitchWallActive(d, j, i))
                    cout << ' ';
                else if (t == TileType::SWITCH_DAMAGE_FLOOR && !isSwitchFloorActive(d, j, i))
                    cout << ' ';
                else
                    cout << tileToChar(t);
            }
            cout << '\n';
        }
    }

    // Input helpers 

    // This prevents aliases like "play","dungeon","enter" from inflating the valid range.
    int getChoiceInput(const string& prompt, const map<string, int>& options) {
        int maxNum = 0;
        for (const auto& kv : options) maxNum = max(maxNum, kv.second);

        string input;
        while (true) {
            cout << prompt;
            getline(cin, input);
            transform(input.begin(), input.end(), input.begin(), ::tolower);

            try {
                size_t pos;
                int val = stoi(input, &pos);
                if (pos == input.length() && val >= 1 && val <= maxNum)
                    return val;
            }
            catch (...) {}

            auto it = options.find(input);
            if (it != options.end()) return it->second;

            cout << "Invalid input. Enter a number (1-" << maxNum << ") or a keyword.\n";
        }
    }

    int getIntInput(const string& prompt, int minVal = INT_MIN, int maxVal = INT_MAX) {
        string input;
        while (true) {
            cout << prompt;
            getline(cin, input);
            try {
                size_t pos;
                int val = stoi(input, &pos);
                if (pos == input.length() && val >= minVal && val <= maxVal)
                    return val;
            }
            catch (...) {}
            cout << "Invalid input. Please enter a number";
            if (minVal != INT_MIN || maxVal != INT_MAX)
                cout << " between " << minVal << " and " << maxVal;
            cout << ".\n";
        }
    }

    string getLowerStrInput(const string& prompt) {
        string val;
        cout << prompt;
        getline(cin, val);
        transform(val.begin(), val.end(), val.begin(), ::tolower);
        return val;
    }

    // ----- Game logic helpers -----

    int getDamageForFloor(const string& fType) const {
        if (fType == "spikes") return 3;
        if (fType == "poison") return 1;
        return 5; // lava (default)
    }

    bool isSwitchWallActive(const Dungeon& d, int col, int row) const {
        auto it = d.switchWallIndices.find({ col, row });
        if (it == d.switchWallIndices.end()) return true;
        auto sit = d.switchStates.find(it->second);
        return (sit == d.switchStates.end()) ? true : sit->second;
    }

    bool isSwitchFloorActive(const Dungeon& d, int col, int row) const {
        auto it = d.switchDamageFloorIndices.find({ col, row });
        if (it == d.switchDamageFloorIndices.end()) return true;
        auto sit = d.switchStates.find(it->second);
        return (sit == d.switchStates.end()) ? true : sit->second;
    }

    // Turn-by-turn battle with full log
    bool conductBattle(Player& p, Character& e) {
        int pDmg = max(0, p.getStats().str - e.getStats().def);
        int eDmg = max(0, e.getStats().str - p.getStats().def);

        cout << "\n--- BATTLE LOG ---\n";
        if (pDmg == 0) {
            cout << "> You cannot pierce the enemy's armor!\n";
            cout << "> You attempt to flee but take lethal damage!\n";
            p.modifyHealth(-(p.getStats().hp + 1));
            return false;
        }

        while (p.isAlive() && e.isAlive()) {
            e.modifyHealth(-pDmg);
            cout << "> You strike the enemy for " << pDmg
                << " damage.  Enemy HP: " << max(0, e.getStats().hp) << "\n";
            if (!e.isAlive()) { cout << "Enemy defeated!\n"; return true; }

            p.modifyHealth(-eDmg);
            cout << "> Enemy strikes you for " << eDmg
                << " damage.  Your HP: " << max(0, p.getStats().hp) << "\n";
            if (!p.isAlive()) return false;
        }
        return false;
    }

    // Inspect a tile — used in both gameplay and editor
    void displayTileInfo(const Dungeon& d, int col, int row) const {
        TileType t = d.grid[row][col];
        cout << "\n====================================\n";
        cout << "     OBJECT INSPECTION REPORT\n";
        cout << "====================================\n";
        cout << "Location : Row " << row + 1 << " | Col " << col + 1 << "\n";
        cout << "Symbol   : '" << tileToChar(t) << "'\n";
        cout << "--- DETAILS ---\n";

        switch (t) {
        case TileType::EMPTY:
            cout << "Empty space. Freely passable.\n"; break;
        case TileType::WALL:
            cout << "Wall. Impassable.\n"; break;
        case TileType::GOAL:
            cout << "Goal. Reach this to complete the dungeon!\n"; break;

        case TileType::KEY: {
            string kType = d.lockTypes.count({ col,row }) ? d.lockTypes.at({ col,row }) : "default";
            cout << "Key (" << kType << "). Opens " << kType << " doors.\n"; break;
        }
        case TileType::DOOR: {
            string dType = d.lockTypes.count({ col,row }) ? d.lockTypes.at({ col,row }) : "default";
            cout << "Locked Door. Requires a " << dType << " key.\n"; break;
        }
        case TileType::ENEMY: {
            if (d.enemyStats.count({ col,row })) {
                Stats s = d.enemyStats.at({ col,row });
                cout << "Enemy.  HP: " << s.hp << " | STR: " << s.str << " | DEF: " << s.def << "\n";
            }
            else {
                cout << "Enemy. (default stats: HP 20 | STR 5 | DEF 2)\n";
            }
            break;
        }
        case TileType::HP_POT: {
            int v = d.potionValues.count({ col,row }) ? d.potionValues.at({ col,row }) : 20;
            cout << "Health Potion. Restores +" << v << " HP.\n"; break;
        }
        case TileType::STR_POT: {
            int v = d.potionValues.count({ col,row }) ? d.potionValues.at({ col,row }) : 2;
            cout << "Strength Potion. Boosts +" << v << " STR.\n"; break;
        }
        case TileType::DEF_POT: {
            int v = d.potionValues.count({ col,row }) ? d.potionValues.at({ col,row }) : 2;
            cout << "Defense Potion. Boosts +" << v << " DEF.\n"; break;
        }
        case TileType::TELEPORTER: {
            if (d.teleporters.count({ col,row })) {
                auto dest = d.teleporters.at({ col,row });
                cout << "Teleporter. Warps to Row " << dest.second + 1
                    << " | Col " << dest.first + 1 << ".\n";
            }
            else {
                cout << "Teleporter. (unlinked — broken)\n";
            }
            break;
        }
        case TileType::DAMAGE_FLOOR: {
            string ft = d.damageFloorTypes.count({ col,row }) ? d.damageFloorTypes.at({ col,row }) : "lava";
            cout << "Damaging Floor (" << ft << "). Deals " << getDamageForFloor(ft)
                << " damage per step. Does NOT disappear.\n"; break;
        }
        case TileType::SWITCH: {
            if (d.switchIndices.count({ col,row })) {
                int idx = d.switchIndices.at({ col,row });
                auto sit = d.switchStates.find(idx);
                bool active = (sit == d.switchStates.end()) ? true : sit->second;
                cout << "Switch (Signal #" << idx << "). Linked objects: "
                    << (active ? "ACTIVE" : "INACTIVE") << ".\n";
            }
            else {
                cout << "Switch. (no signal assigned)\n";
            }
            break;
        }
        case TileType::SWITCH_WALL: {
            if (d.switchWallIndices.count({ col,row })) {
                int idx = d.switchWallIndices.at({ col,row });
                auto sit = d.switchStates.find(idx);
                bool active = (sit == d.switchStates.end()) ? true : sit->second;
                cout << "Switch-Controlled Wall (Signal #" << idx << "). Currently: "
                    << (active ? "BLOCKING" : "OPEN / PASSABLE") << ".\n";
            }
            else {
                cout << "Switch-Controlled Wall. (no signal)\n";
            }
            break;
        }
        case TileType::SWITCH_DAMAGE_FLOOR: {
            if (d.switchDamageFloorIndices.count({ col,row })) {
                int idx = d.switchDamageFloorIndices.at({ col,row });
                auto sit = d.switchStates.find(idx);
                bool active = (sit == d.switchStates.end()) ? true : sit->second;
                string ft = d.damageFloorTypes.count({ col,row }) ? d.damageFloorTypes.at({ col,row }) : "lava";
                cout << "Switch-Controlled Floor (" << ft << " " << getDamageForFloor(ft)
                    << " dmg, Signal #" << idx << "). Currently: "
                    << (active ? "DANGEROUS" : "SAFE") << ".\n";
            }
            else {
                cout << "Switch-Controlled Damage Floor. (no signal)\n";
            }
            break;
        }
        default:
            cout << "Unknown tile.\n";
        }
        cout << "====================================\n";
    }


    //  MAIN GAME LOOP  — returns true on victory, false on death/quit

    bool playDungeonLoop(Dungeon activeDungeon, bool autoMode = false, int autoSpeed = 500) {
        Player p(50, 5, 2);

        // Initialise all switch states to ACTIVE
        auto initState = [&](int idx) {
            if (!activeDungeon.switchStates.count(idx))
                activeDungeon.switchStates[idx] = true;
            };
        for (auto& kv : activeDungeon.switchIndices)            initState(kv.second);
        for (auto& kv : activeDungeon.switchWallIndices)        initState(kv.second);
        for (auto& kv : activeDungeon.switchDamageFloorIndices) initState(kv.second);

        // Find player starting position
        for (int i = 0; i < activeDungeon.rows; i++)
            for (int j = 0; j < activeDungeon.cols; j++)
                if (activeDungeon.grid[i][j] == TileType::PLAYER) {
                    p.setPos(j, i);
                    activeDungeon.grid[i][j] = TileType::EMPTY;
                }

        // Print legend once on entry (feedback: show what symbols mean)
        if (!autoMode) {
            cout << "\n=== DUNGEON: " << activeDungeon.name << " ===\n";
            cout << "LEGEND:\n";
            cout << "  @ = You (Player)    # = Wall          G = Goal (win!)\n";
            cout << "  K = Key             X = Locked Door   E = Enemy\n";
            cout << "  H = Health Potion   S = Str Potion    D = Def Potion\n";
            cout << "  T = Teleporter      ~ = Damage Floor  O = Switch\n";
            cout << "  W = Switch Wall (W=active/blocking)   F = Switch Floor (F=active/dangerous)\n";
            cout << "CONTROLS: W=Up  A=Left  S=Down  D=Right  |  I=Inspect  Q=Quit\n\n";
        }

        string message = "You entered the dungeon!";

        while (true) {
            // HUD
            cout << "\n========================================\n";
            cout << " HP: " << p.getStats().hp
                << " | STR: " << p.getStats().str
                << " | DEF: " << p.getStats().def << "\n";
            cout << " Keys: [" << p.getKeysDisplay() << "]\n";
            cout << "========================================\n";

            printGrid(activeDungeon, p);
            cout << "\n> " << message << "\n\n";
            message = "";

            // Input 
            string move;
            int dx = 0, dy = 0;

            if (autoMode) {
                sleepMs(autoSpeed);
                // Random direction
                const int dirs[4][2] = { {0,-1},{0,1},{-1,0},{1,0} };
                int d2 = rand() % 4;
                dx = dirs[d2][0]; dy = dirs[d2][1];
                const char* names[] = { "Up","Down","Left","Right" };
                cout << "[AUTOPLAY] Moving: " << names[d2] << "\n";
            }
            else {
                move = getLowerStrInput("Move (W/A/S/D) | I=Inspect | Q=Quit: ");

                if (move == "w" || move == "up" || move == "u") dy = -1;
                else if (move == "s" || move == "down")                 dy = 1;
                else if (move == "a" || move == "left" || move == "l") dx = -1;
                else if (move == "d" || move == "right" || move == "r") dx = 1;
                else if (move == "q" || move == "quit") {
                    cout << "Returning to main menu...\n";
                    return false;
                }
                else if (move == "i" || move == "inspect") {
                    // Inspect current tile or any specific tile
                    cout << "Inspect: C = current tile,  or enter row number to pick a tile.\n";
                    string sub = getLowerStrInput("Choice (C / row number): ");
                    if (sub == "c" || sub == "current") {
                        displayTileInfo(activeDungeon, p.getX(), p.getY());
                    }
                    else {
                        try {
                            int ir = stoi(sub) - 1;
                            int ic = getIntInput("Enter col to inspect: ", 1, activeDungeon.cols) - 1;
                            if (ir >= 0 && ir < activeDungeon.rows)
                                displayTileInfo(activeDungeon, ic, ir);
                            else
                                cout << "Row out of range.\n";
                        }
                        catch (...) {
                            cout << "Invalid row.\n";
                        }
                    }
                    message = "Inspection complete.";
                    continue;
                }
                else {
                    message = "Invalid input! Use W/A/S/D, I to inspect, or Q to quit.";
                    continue;
                }
            }

            int nx = p.getX() + dx;
            int ny = p.getY() + dy;

            // Boundary check
            if (nx < 0 || nx >= activeDungeon.cols || ny < 0 || ny >= activeDungeon.rows) {
                message = "You bumped into the edge of the world!";
                continue;
            }

            TileType target = activeDungeon.grid[ny][nx];

            //  Tile interactions 

            // Switch-controlled wall
            if (target == TileType::SWITCH_WALL) {
                if (isSwitchWallActive(activeDungeon, nx, ny)) {
                    message = "A switch-controlled wall blocks your path! (Signal #"
                        + to_string(activeDungeon.switchWallIndices[{nx, ny}]) + ")";
                }
                else {
                    p.setPos(nx, ny);
                    message = "You walked through an inactive switch wall.";
                }
                continue;
            }

            // Switch-controlled damage floor
            if (target == TileType::SWITCH_DAMAGE_FLOOR) {
                p.setPos(nx, ny);
                if (isSwitchFloorActive(activeDungeon, nx, ny)) {
                    string ft = activeDungeon.damageFloorTypes.count({ nx,ny })
                        ? activeDungeon.damageFloorTypes[{nx, ny}] : "lava";
                    int dmg = getDamageForFloor(ft);
                    p.modifyHealth(-dmg);
                    message = "You stepped on active " + ft + " and took " + to_string(dmg) + " damage!";
                    if (!p.isAlive()) {
                        cout << "\n=== GAME OVER ===\nYou were destroyed by a hazard.\n\n";
                        return false;
                    }
                }
                else {
                    message = "You walked over an inactive hazard floor (safe for now).";
                }
                continue;
            }

            // Standard tiles
            if (target == TileType::WALL) {
                message = "You bumped into a wall!";
            }
            else if (target == TileType::EMPTY) {
                p.setPos(nx, ny);
                message = "Moved.";
            }
            // Switch — toggle linked objects
            else if (target == TileType::SWITCH) {
                p.setPos(nx, ny); // Switch is NOT destroyed
                if (activeDungeon.switchIndices.count({ nx,ny })) {
                    int idx = activeDungeon.switchIndices[{nx, ny}];
                    activeDungeon.switchStates[idx] = !activeDungeon.switchStates[idx];
                    bool nowActive = activeDungeon.switchStates[idx];
                    message = "You toggled Switch #" + to_string(idx) + "! Linked objects now "
                        + (nowActive ? "ACTIVE." : "INACTIVE.");
                }
                else {
                    message = "You stepped on a switch... nothing happened.";
                }
            }
            // Key — BUG FIX #4: disappears on pickup
            else if (target == TileType::KEY) {
                string kType = activeDungeon.lockTypes.count({ nx,ny })
                    ? activeDungeon.lockTypes[{nx, ny}] : "default";
                p.addKey(kType);
                activeDungeon.grid[ny][nx] = TileType::EMPTY; // Key consumed
                p.setPos(nx, ny);
                message = "You picked up a " + kType + " key!";
            }
            // Door — BUG FIX #4: disappears when unlocked
            else if (target == TileType::DOOR) {
                string dType = activeDungeon.lockTypes.count({ nx,ny })
                    ? activeDungeon.lockTypes[{nx, ny}] : "default";
                if (p.useKey(dType)) {
                    activeDungeon.grid[ny][nx] = TileType::EMPTY;
                    p.setPos(nx, ny);
                    message = "You used a " + dType + " key and unlocked the door!";
                }
                else {
                    message = "The door is locked. You need a " + dType + " key.";
                }
            }
            // Teleporter
            else if (target == TileType::TELEPORTER) {
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
            // Potions — BUG FIX #4: disappear on use
            else if (target == TileType::HP_POT || target == TileType::STR_POT || target == TileType::DEF_POT) {
                int val = activeDungeon.potionValues.count({ nx,ny })
                    ? activeDungeon.potionValues[{nx, ny}]
                    : (target == TileType::HP_POT ? 20 : 2);
                activeDungeon.grid[ny][nx] = TileType::EMPTY; // Potion consumed

                if (target == TileType::HP_POT) {
                    p.modifyHealth(val);
                    message = "Drank a Health Potion! (+" + to_string(val) + " HP)";
                }
                else if (target == TileType::STR_POT) {
                    p.addStrength(val);
                    message = "Drank a Strength Potion! (+" + to_string(val) + " STR)";
                }
                else {
                    p.addDefense(val);
                    message = "Drank a Defense Potion! (+" + to_string(val) + " DEF)";
                }
                p.setPos(nx, ny);
            }
            // Damage floor — tile persists
            else if (target == TileType::DAMAGE_FLOOR) {
                string ft = activeDungeon.damageFloorTypes.count({ nx,ny })
                    ? activeDungeon.damageFloorTypes[{nx, ny}] : "lava";
                int dmg = getDamageForFloor(ft);
                p.modifyHealth(-dmg);
                p.setPos(nx, ny); // Floor does NOT disappear
                message = "You stepped on " + ft + " and took " + to_string(dmg) + " damage!";
                if (!p.isAlive()) {
                    cout << "\n=== GAME OVER ===\nYou succumbed to environmental hazards.\n\n";
                    return false;
                }
            }
            // Enemy — BUG FIX #3: removed from grid after defeat
            else if (target == TileType::ENEMY) {
                Stats es = activeDungeon.enemyStats.count({ nx,ny })
                    ? activeDungeon.enemyStats[{nx, ny}] : Stats{ 20, 5, 2 };
                Character enemy(es.hp, es.str, es.def);

                cout << "\n[!] BATTLE INITIATED [!]\n";
                cout << "Enemy Stats: HP " << es.hp << " | STR " << es.str << " | DEF " << es.def << "\n";

                if (conductBattle(p, enemy)) {
                    activeDungeon.grid[ny][nx] = TileType::EMPTY; // Enemy defeated & removed
                    p.setPos(nx, ny);
                    message = "You defeated the enemy!";
                }
                else {
                    cout << "\n=== GAME OVER ===\nYou perished in battle.\n\n";
                    return false;
                }
            }
            // Goal
            else if (target == TileType::GOAL) {
                cout << "\n*** VICTORY! ***\nYou reached the Goal and escaped the dungeon!\n\n";
                return true;
            }

            if (!p.isAlive()) {
                cout << "\n=== GAME OVER ===\nYou have died.\n\n";
                return false;
            }
        }
    }


    //  DUNGEON SELECTION MENU


    void loadDungeonMenu() {
        if (dungeons.empty()) { cout << "No dungeons available!\n"; return; }

        cout << "\n--- Select a Dungeon ---\n";
        map<string, int> opts;
        for (size_t i = 0; i < dungeons.size(); ++i) {
            cout << i + 1 << ") " << dungeons[i].name << "\n";
            string key = dungeons[i].name;
            transform(key.begin(), key.end(), key.begin(), ::tolower);
            opts[key] = (int)i + 1;
        }
        cout << dungeons.size() + 1 << ") Back (or 'back')\n";
        opts["back"] = (int)dungeons.size() + 1;

        int choice = getChoiceInput("Enter selection: ", opts);
        if (choice <= (int)dungeons.size())
            playDungeonLoop(dungeons[choice - 1]);
    }


    //  AUTOPLAY MODE  (Extra Credit 2)


    void autoplayMenu() {
        if (dungeons.empty()) { cout << "No dungeons available!\n"; return; }

        cout << "\n--- AUTOPLAY MODE ---\n";
        cout << "The AI will move randomly until it wins or loses.\n\n";

        // Dungeon selection
        map<string, int> dungeonOpts;
        for (size_t i = 0; i < dungeons.size(); ++i) {
            cout << i + 1 << ") " << dungeons[i].name << "\n";
            string key = dungeons[i].name;
            transform(key.begin(), key.end(), key.begin(), ::tolower);
            dungeonOpts[key] = (int)i + 1;
        }
        cout << dungeons.size() + 1 << ") Back\n";
        dungeonOpts["back"] = (int)dungeons.size() + 1;

        int dChoice = getChoiceInput("Select dungeon: ", dungeonOpts);
        if (dChoice > (int)dungeons.size()) return;

        // Speed selection
        cout << "\nSelect autoplay speed:\n";
        cout << "1) Fast   (200 ms)\n";
        cout << "2) Normal (500 ms)\n";
        cout << "3) Slow   (1000 ms)\n";
        cout << "4) Custom\n";
        map<string, int> speedOpts = { {"fast",1},{"normal",2},{"slow",3},{"custom",4} };
        int sChoice = getChoiceInput("Speed: ", speedOpts);

        int ms = 500;
        if (sChoice == 1) ms = 200;
        else if (sChoice == 2) ms = 500;
        else if (sChoice == 3) ms = 1000;
        else    ms = getIntInput("Enter delay in milliseconds: ", 50, 10000);

        // Run (restart loop on loss)
        while (true) {
            cout << "\n[AUTOPLAY] Starting dungeon: " << dungeons[dChoice - 1].name << "\n";
            bool won = playDungeonLoop(dungeons[dChoice - 1], true, ms);

            if (won) {
                cout << "[AUTOPLAY] The AI WON! Returning to menu.\n";
                break;
            }
            string retry = getLowerStrInput("[AUTOPLAY] The AI lost. Try again? (yes/no): ");
            if (retry != "yes" && retry != "y") break;
        }
    }


    //  LEVEL EDITOR


    void launchLevelEditor() {
        cout << "\n--- LEVEL EDITOR ---\n";

        map<string, int> modeOpts = { {"create",1},{"new",1},{"edit",2},{"load",2} };
        int editMode = getChoiceInput("1) Create New Dungeon\n2) Edit Existing Dungeon\nSelect mode: ", modeOpts);

        Dungeon d;
        int dIndex = -1;
        int r, c;

        if (editMode == 2) {
            if (dungeons.empty()) {
                cout << "No existing dungeons. Starting fresh.\n";
                editMode = 1;
            }
            else {
                cout << "\n--- Select Dungeon to Edit ---\n";
                map<string, int> editOpts;
                for (size_t i = 0; i < dungeons.size(); ++i) {
                    cout << i + 1 << ") " << dungeons[i].name << "\n";
                    string key = dungeons[i].name;
                    transform(key.begin(), key.end(), key.begin(), ::tolower);
                    editOpts[key] = (int)i + 1;
                }
                dIndex = getChoiceInput("Select: ", editOpts) - 1;
                d = dungeons[dIndex];
                r = d.rows; c = d.cols;
            }
        }

        if (editMode == 1) {
            r = getIntInput("Rows (1-50): ", 1, 50);
            c = getIntInput("Cols (1-50): ", 1, 50);
            d.rows = r; d.cols = c;
            d.grid = vector<vector<TileType>>(r, vector<TileType>(c, TileType::EMPTY));
            // Auto-border
            for (int i = 0; i < r; ++i)
                for (int j = 0; j < c; ++j)
                    if (i == 0 || i == r - 1 || j == 0 || j == c - 1)
                        d.grid[i][j] = TileType::WALL;
        }

        // Clear all custom data when overwriting a tile
        auto eraseTileData = [&](int px, int py) {
            d.enemyStats.erase({ px,py });
            d.potionValues.erase({ px,py });
            d.lockTypes.erase({ px,py });
            d.damageFloorTypes.erase({ px,py });
            d.teleporters.erase({ px,py });
            d.switchIndices.erase({ px,py });
            d.switchWallIndices.erase({ px,py });
            d.switchDamageFloorIndices.erase({ px,py });
            };

        while (true) {
            // Print grid with row/col headers
            cout << "\n    ";
            for (int j = 1; j <= c; ++j) cout << j % 10 << " ";
            cout << "\n";
            for (int i = 0; i < r; ++i) {
                cout << (i + 1 < 10 ? " " : "") << i + 1 << "| ";
                for (int j = 0; j < c; ++j)
                    cout << tileToChar(d.grid[i][j]) << " ";
                cout << "\n";
            }

            cout << "\n--- EDITOR TOOLS ---\n"
                << "1) Single Object       2) Filled Rectangle\n"
                << "3) Hollow Rectangle    4) Circle\n"
                << "5) Inspect Object      6) Save & Exit\n";

            map<string, int> toolOpts = {
                {"single",1},{"object",1},{"rectangle",2},{"filled",2},
                {"hollow",3},{"circle",4},{"inspect",5},{"save",6},{"exit",6}
            };
            int tool = getChoiceInput("Select tool: ", toolOpts);

            //  SAVE 
            if (tool == 6) {
                int pCount = 0, gCount = 0;
                for (const auto& row : d.grid)
                    for (auto tile : row) {
                        if (tile == TileType::PLAYER) pCount++;
                        if (tile == TileType::GOAL)   gCount++;
                    }
                if (pCount != 1) { cout << "[!] Need exactly one Player start (@).\n"; continue; }
                if (gCount < 1) { cout << "[!] Need at least one Goal (G).\n"; continue; }

                if (editMode == 1) {
                    cout << "Enter dungeon name: ";
                    getline(cin, d.name);
                    int existIdx = -1;
                    for (size_t i = 0; i < dungeons.size(); ++i)
                        if (dungeons[i].name == d.name) { existIdx = (int)i; break; }

                    if (existIdx >= 0) {
                        string ow = getLowerStrInput("Name exists. Overwrite? (yes/no): ");
                        if (ow == "yes" || ow == "y") { dungeons[existIdx] = d; cout << "Dungeon updated.\n"; }
                        else { cout << "Save cancelled.\n"; continue; }
                    }
                    else {
                        dungeons.push_back(d);
                        cout << "Dungeon '" << d.name << "' saved!\n";
                    }
                }
                else {
                    cout << "Enter new name (blank to keep '" << d.name << "'): ";
                    string newName; getline(cin, newName);
                    if (!newName.empty()) {
                        int existIdx = -1;
                        for (size_t i = 0; i < dungeons.size(); ++i)
                            if (dungeons[i].name == newName && i != (size_t)dIndex) { existIdx = (int)i; break; }

                        if (existIdx >= 0) {
                            string ow = getLowerStrInput("Name exists. Overwrite? (yes/no): ");
                            if (ow == "yes" || ow == "y") {
                                d.name = newName;
                                dungeons[existIdx] = d;
                                dungeons.erase(dungeons.begin() + dIndex);
                                cout << "Dungeon saved as '" << newName << "'.\n";
                            }
                            else { cout << "Save cancelled.\n"; continue; }
                        }
                        else {
                            d.name = newName;
                            dungeons[dIndex] = d;
                            cout << "Dungeon '" << newName << "' saved.\n";
                        }
                    }
                    else {
                        dungeons[dIndex] = d;
                        cout << "Dungeon '" << d.name << "' saved.\n";
                    }
                }
                break;
            }

            //  INSPECT 
            if (tool == 5) {
                int ri = getIntInput("Row to inspect: ", 1, r) - 1;
                int ci = getIntInput("Col to inspect: ", 1, c) - 1;
                displayTileInfo(d, ci, ri);
                continue;
            }

            //  OBJECT SELECTION 
            cout << "\nAvailable Objects:\n"
                << " 1) Empty Space       2) Wall              3) Player\n"
                << " 4) Goal              5) Key               6) Locked Door\n"
                << " 7) Enemy             8) Health Potion     9) Strength Potion\n"
                << "10) Defense Potion   11) Teleporter       12) Damage Floor\n"
                << "13) Switch           14) Switch Wall      15) Switch Dmg Floor\n\n";

            string objStr = getLowerStrInput("Select object: ");
            TileType sel = TileType::EMPTY;

            if (objStr == "1" || objStr == "empty")         sel = TileType::EMPTY;
            else if (objStr == "2" || objStr == "wall")          sel = TileType::WALL;
            else if (objStr == "3" || objStr == "player")        sel = TileType::PLAYER;
            else if (objStr == "4" || objStr == "goal")          sel = TileType::GOAL;
            else if (objStr == "5" || objStr == "key")           sel = TileType::KEY;
            else if (objStr == "6" || objStr == "door")          sel = TileType::DOOR;
            else if (objStr == "7" || objStr == "enemy")         sel = TileType::ENEMY;
            else if (objStr == "8" || objStr == "health")        sel = TileType::HP_POT;
            else if (objStr == "9" || objStr == "strength")      sel = TileType::STR_POT;
            else if (objStr == "10" || objStr == "defense")       sel = TileType::DEF_POT;
            else if (objStr == "11" || objStr == "teleporter")    sel = TileType::TELEPORTER;
            else if (objStr == "12" || objStr == "damage")        sel = TileType::DAMAGE_FLOOR;
            else if (objStr == "13" || objStr == "switch")        sel = TileType::SWITCH;
            else if (objStr == "14" || objStr == "switch wall")   sel = TileType::SWITCH_WALL;
            else if (objStr == "15" || objStr == "switch floor")  sel = TileType::SWITCH_DAMAGE_FLOOR;
            else { cout << "Invalid object choice!\n"; continue; }

            // ----- GATHER PROPERTIES -----
            Stats enemyStat{ 20, 5, 2 };
            if (sel == TileType::ENEMY) {
                cout << "-- Enemy Configuration --\n";
                enemyStat.hp = getIntInput("HP:  ", 1);
                enemyStat.str = getIntInput("STR: ", 0);
                enemyStat.def = getIntInput("DEF: ", 0);
            }

            int potVal = 2;
            if (sel == TileType::HP_POT)
                potVal = getIntInput("HP to restore: ", 1);
            else if (sel == TileType::STR_POT || sel == TileType::DEF_POT)
                potVal = getIntInput("Stat boost amount: ", 1);

            string lockType = "default";
            if (sel == TileType::KEY || sel == TileType::DOOR) {
                cout << "Enter color/type (e.g. blue, red): ";
                getline(cin, lockType);
                if (lockType.empty()) lockType = "default";
            }

            string floorType = "lava";
            if (sel == TileType::DAMAGE_FLOOR || sel == TileType::SWITCH_DAMAGE_FLOOR) {
                cout << "Enter floor type (lava=5 dmg, spikes=3 dmg, poison=1 dmg): ";
                getline(cin, floorType);
                if (floorType.empty()) floorType = "lava";
            }

            // ----- SPECIAL: TELEPORTER -----
            if (sel == TileType::TELEPORTER) {
                cout << "[Teleporters must be placed in pairs!]\n";
                int r1 = getIntInput("Row for Teleporter A: ", 1, r) - 1;
                int c1 = getIntInput("Col for Teleporter A: ", 1, c) - 1;
                int r2 = getIntInput("Row for Teleporter B: ", 1, r) - 1;
                int c2 = getIntInput("Col for Teleporter B: ", 1, c) - 1;
                eraseTileData(c1, r1); eraseTileData(c2, r2);
                d.grid[r1][c1] = TileType::TELEPORTER;
                d.grid[r2][c2] = TileType::TELEPORTER;
                d.teleporters[{c1, r1}] = { c2, r2 };
                d.teleporters[{c2, r2}] = { c1, r1 };
                cout << "Teleporters linked!\n";
                continue;
            }

            // ----- SPECIAL: SWITCH (Extra Credit 1) -----
            if (sel == TileType::SWITCH) {
                int switchIdx = getIntInput("Enter switch signal index (0-99): ", 0, 99);
                if (!d.switchStates.count(switchIdx)) d.switchStates[switchIdx] = true;

                int swR = getIntInput("Row for switch: ", 1, r) - 1;
                int swC = getIntInput("Col for switch: ", 1, c) - 1;
                eraseTileData(swC, swR);
                d.grid[swR][swC] = TileType::SWITCH;
                d.switchIndices[{swC, swR}] = switchIdx;
                cout << "Switch placed at Row " << swR + 1 << " Col " << swC + 1
                    << " (Signal #" << switchIdx << ")\n";

                // Link walls and floors
                cout << "\nLink objects to this switch (they will toggle when switch is touched).\n";
                while (true) {
                    cout << "1) Add Switch Wall\n"
                        << "2) Add Switch Damage Floor\n"
                        << "3) Link existing Wall to this switch\n"
                        << "4) Link existing Damage Floor to this switch\n"
                        << "5) Done\n";
                    map<string, int> linkOpts = {
                        {"add wall",1},{"wall",1},
                        {"add floor",2},{"floor",2},
                        {"link wall",3},{"link floor",4},
                        {"done",5},{"finish",5}
                    };
                    int lc = getChoiceInput("Choose: ", linkOpts);
                    if (lc == 5) break;

                    int lr = getIntInput("Row: ", 1, r) - 1;
                    int lcol = getIntInput("Col: ", 1, c) - 1;

                    if (lc == 1) {
                        eraseTileData(lcol, lr);
                        d.grid[lr][lcol] = TileType::SWITCH_WALL;
                        d.switchWallIndices[{lcol, lr}] = switchIdx;
                        cout << "Switch Wall added at Row " << lr + 1 << " Col " << lcol + 1 << ".\n";
                    }
                    else if (lc == 2) {
                        string sft = "lava";
                        cout << "Floor type (lava/spikes/poison): ";
                        getline(cin, sft);
                        if (sft.empty()) sft = "lava";
                        eraseTileData(lcol, lr);
                        d.grid[lr][lcol] = TileType::SWITCH_DAMAGE_FLOOR;
                        d.switchDamageFloorIndices[{lcol, lr}] = switchIdx;
                        d.damageFloorTypes[{lcol, lr}] = sft;
                        cout << "Switch Floor added at Row " << lr + 1 << " Col " << lcol + 1 << ".\n";
                    }
                    else if (lc == 3) {
                        if (d.grid[lr][lcol] == TileType::WALL) {
                            d.grid[lr][lcol] = TileType::SWITCH_WALL;
                            d.switchWallIndices[{lcol, lr}] = switchIdx;
                            cout << "Wall at Row " << lr + 1 << " Col " << lcol + 1
                                << " linked to Switch #" << switchIdx << ".\n";
                        }
                        else { cout << "No Wall found there.\n"; }
                    }
                    else if (lc == 4) {
                        if (d.grid[lr][lcol] == TileType::DAMAGE_FLOOR) {
                            d.grid[lr][lcol] = TileType::SWITCH_DAMAGE_FLOOR;
                            d.switchDamageFloorIndices[{lcol, lr}] = switchIdx;
                            cout << "Damage Floor at Row " << lr + 1 << " Col " << lcol + 1
                                << " linked to Switch #" << switchIdx << ".\n";
                        }
                        else { cout << "No Damage Floor found there.\n"; }
                    }
                }
                continue;
            }

            // ----- STANDARD SHAPE PLACEMENT -----
            auto placeTile = [&](int i, int j) {
                eraseTileData(j, i);
                d.grid[i][j] = sel;
                if (sel == TileType::ENEMY)
                    d.enemyStats[{j, i}] = enemyStat;
                if (sel == TileType::HP_POT || sel == TileType::STR_POT || sel == TileType::DEF_POT)
                    d.potionValues[{j, i}] = potVal;
                if (sel == TileType::KEY || sel == TileType::DOOR)
                    d.lockTypes[{j, i}] = lockType;
                if (sel == TileType::DAMAGE_FLOOR)
                    d.damageFloorTypes[{j, i}] = floorType;
                // Switch Wall / Switch Floor need an index
                if (sel == TileType::SWITCH_WALL) {
                    int idx = getIntInput("Signal index for this Switch Wall: ", 0, 99);
                    d.switchWallIndices[{j, i}] = idx;
                    if (!d.switchStates.count(idx)) d.switchStates[idx] = true;
                }
                if (sel == TileType::SWITCH_DAMAGE_FLOOR) {
                    int idx = getIntInput("Signal index for this Switch Floor: ", 0, 99);
                    d.switchDamageFloorIndices[{j, i}] = idx;
                    d.damageFloorTypes[{j, i}] = floorType;
                    if (!d.switchStates.count(idx)) d.switchStates[idx] = true;
                }
                };

            if (tool == 1) {
                int row = getIntInput("Row: ", 1, r) - 1;
                int col = getIntInput("Col: ", 1, c) - 1;
                placeTile(row, col);
            }
            else if (tool == 2 || tool == 3) {
                int r1 = getIntInput("Corner 1 Row: ", 1, r) - 1;
                int c1 = getIntInput("Corner 1 Col: ", 1, c) - 1;
                int r2 = getIntInput("Corner 2 Row: ", 1, r) - 1;
                int c2 = getIntInput("Corner 2 Col: ", 1, c) - 1;
                int minR = min(r1, r2), maxR = max(r1, r2);
                int minC = min(c1, c2), maxC = max(c1, c2);
                for (int i = minR; i <= maxR; ++i)
                    for (int j = minC; j <= maxC; ++j)
                        if (tool == 2 || i == minR || i == maxR || j == minC || j == maxC)
                            placeTile(i, j);
            }
            else if (tool == 4) {
                int rc = getIntInput("Center Row: ", 1, r) - 1;
                int cc = getIntInput("Center Col: ", 1, c) - 1;
                int rad = getIntInput("Radius: ", 0, max(r, c));
                for (int i = 0; i < r; ++i)
                    for (int j = 0; j < c; ++j)
                        if ((i - rc) * (i - rc) + (j - cc) * (j - cc) <= rad * rad)
                            placeTile(i, j);
            }
        }
    }


    //  DEFAULT DUNGEONS


    void loadDefaultDungeons() {
        // ---- DUNGEON 1: MUNYLU ----
        Dungeon d1;
        d1.name = "MUNYLU";
        d1.rows = 10; d1.cols = 10;
        d1.grid = vector<vector<TileType>>(d1.rows, vector<TileType>(d1.cols, TileType::EMPTY));
        for (int i = 0; i < d1.rows; ++i)
            for (int j = 0; j < d1.cols; ++j)
                if (i == 0 || i == d1.rows - 1 || j == 0 || j == d1.cols - 1)
                    d1.grid[i][j] = TileType::WALL;

        d1.grid[1][1] = TileType::PLAYER;
        d1.grid[1][8] = TileType::GOAL;

        // Correct {col, row} keys for lockTypes
        d1.grid[2][3] = TileType::KEY;
        d1.lockTypes[{3, 2}] = "red";   // key at row=2, col=3 → {col,row} = {3,2}

        d1.grid[3][5] = TileType::DOOR;
        d1.lockTypes[{5, 3}] = "red";   // door at row=3, col=5 → {col,row} = {5,3}

        d1.grid[4][4] = TileType::ENEMY;
        d1.enemyStats[{4, 4}] = { 20, 5, 2 };

        d1.grid[3][1] = TileType::HP_POT;
        d1.potionValues[{1, 3}] = 25;

        d1.grid[5][2] = TileType::STR_POT;
        d1.potionValues[{2, 5}] = 3;

        d1.grid[7][4] = TileType::DEF_POT;
        d1.potionValues[{4, 7}] = 4;



        Dungeon d2;
        d2.name = "Georgina's Trial";
        d2.rows = 7; d2.cols = 7;
        d2.grid = vector<vector<TileType>>(d2.rows, vector<TileType>(d2.cols, TileType::EMPTY));
        for (int i = 0; i < d2.rows; ++i)
            for (int j = 0; j < d2.cols; ++j)
                if (i == 0 || i == d2.rows - 1 || j == 0 || j == d2.cols - 1)
                    d2.grid[i][j] = TileType::WALL;

        d2.grid[1][1] = TileType::PLAYER;
        d2.grid[1][3] = TileType::ENEMY;
        d2.enemyStats[{3, 1}] = { 15, 6, 1 };

        d2.grid[1][5] = TileType::TELEPORTER;
        d2.grid[2][3] = TileType::WALL;
        d2.grid[2][5] = TileType::WALL;

        d2.grid[3][1] = TileType::KEY;
        d2.lockTypes[{1, 3}] = "blue";

        d2.grid[3][3] = TileType::DOOR;
        d2.lockTypes[{3, 3}] = "blue";

        d2.grid[3][5] = TileType::GOAL;

        // Lava river
        for (int j = 1; j <= 3; ++j) {
            d2.grid[4][j] = TileType::DAMAGE_FLOOR;
            d2.damageFloorTypes[{j, 4}] = "lava";
        }

        d2.grid[5][1] = TileType::TELEPORTER;
        d2.grid[5][3] = TileType::DEF_POT;
        d2.potionValues[{3, 5}] = 5;
        d2.grid[5][5] = TileType::STR_POT;
        d2.potionValues[{5, 5}] = 3;

        d2.teleporters[{5, 1}] = { 1, 5 };
        d2.teleporters[{1, 5}] = { 5, 1 };

        dungeons.push_back(d1);
        dungeons.push_back(d2);
    }

public:
    GameManager() {
        srand(static_cast<unsigned>(time(nullptr)));
        loadDefaultDungeons();
    }

    void run() {
        while (true) {
            cout << "\n====================================\n";
            cout << "       Welcome to Magic Tower!\n";
            cout << "===================================\n";
            cout << " 1) Enter a Dungeon  (or 'play')\n";
            cout << " 2) Level Editor     (or 'edit')\n";
            cout << " 3) Autoplay Mode    (or 'autoplay')\n";
            cout << " 4) Exit             (or 'quit')\n";

            map<string, int> mainOpts = {
                {"play",1},{"dungeon",1},{"enter",1},
                {"edit",2},{"editor",2},
                {"autoplay",3},{"auto",3},
                {"quit",4},{"exit",4}
            };
            int choice = getChoiceInput("Select an option: ", mainOpts);

            if (choice == 1) loadDungeonMenu();
            else if (choice == 2) launchLevelEditor();
            else if (choice == 3) autoplayMenu();
            else { cout << "Exiting. Goodbye!\n"; break; }
        }
    }
};


//  ENTRY POINT


int main() {
    GameManager game;
    game.run();
    return 0;
}