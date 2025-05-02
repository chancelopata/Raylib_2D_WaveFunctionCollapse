/*
WaveFunctionCollapse:
This program creates a grid of tiles that represents a 2d map using the algorithm wave function collapse. The raylib game library is used for rendering to the screen.
Edit the WORLD_WIDTH, WORLD_HEIGHT, TILE_SIZE, and COLORS variables to create your own map!

The COLORS variable is the list of rules used to generate tiles. A color can only be drawn next to one of the colors in an adjacent index.
For example: colors[] = {DARKBLUE,BLUE,YELLOW,GREEN,DARKGREEN} means that yellow tiles will only be drawn next to BLUE and GREEN tiles, while BLUE and GREEN tiles will
never touch. Adjacent tiles in this program do not count corners.
*/

/* 
BUGS:
- does not work for rectangle worlds
- occasionally tiles will be missing (more obvious in larger worlds)
Possible todo:
- Add const to all that can be applied to.
- add proper .h file and #DEFINE statements?
- Seperate the world creation algorithm from the world, so that a world can specify the genertion algorithm used and only need the color ID returned.
- Add weights to COLOR rules.
- User a better random algorithm.
*/

#include <iostream>
#include <string>
#include <random>
#include <raylib.h>
#include <vector>

using namespace std;


/**************************************************
 * Edit these to create your own world generation.
 **************************************************/
// Information that controls the world and tile size
const int WORLD_WIDTH = 32;
const int WORLD_HEIGHT = 32;
const int TILE_SIZE = 8;
const int SCREEN_WIDTH = TILE_SIZE*WORLD_WIDTH;
const int SCREEN_HEIGHT = TILE_SIZE*WORLD_HEIGHT;
// Wave function collapse rule set
const Color COLORS[] = {DARKBLUE,BLUE,YELLOW,GREEN,DARKGREEN}; // First testing colors ever used!

// Fancier set of colors, creates islands of forests, snowy mountains, and volcanic terrain.
// const Color COLORS[] = {BROWN,WHITE,LIGHTGRAY // Snowy Mountains
//     ,BLUE,DARKBLUE,BLUE //Ocean
//     ,BEIGE,GREEN,DARKGREEN,GREEN,BEIGE // Forest
//     ,BLUE,DARKBLUE,BLUE // Ocean
//     ,LIGHTGRAY,GRAY,DARKGRAY,MAROON // Volcanic
//     };
// const Color COLORS[] = {BROWN,WHITE,LIGHTGRAY // Snowy Mountains
//      ,BEIGE,GREEN,DARKGREEN,GREEN,BEIGE // Forest
//     ,LIGHTGRAY,GRAY,DARKGRAY,MAROON // Volcanic
//     };
int collapsedTiles = 0;
int selectedX, selectedY;


const int ALL_TILE_OPTIONS = sizeof(COLORS)/sizeof(COLORS[0]); // Number of different possible tiles that a tile can be. Ex: Water 0, sand 1, grass 2.

/*************************************************************************************************
 * struct Range
 * Used to represent a range of numbers by only holding the upper and lower bounds of the range
 * so that the numbers within the range can be inferred without being stored. Inclusive.
 *************************************************************************************************/
struct Range {
    int upper = ALL_TILE_OPTIONS-1;
    int lower = 0;

    Range(){}
    Range(int theLower, int theUpper){
        upper = theUpper;
        lower = theLower;
    }

    bool operator== (const Range &rhs) const {
        return (upper == rhs.upper) && (lower == rhs.lower);
    }
    bool operator!= (const Range &rhs) const {
        return !((upper == rhs.upper) && (lower == rhs.lower));
    }

};

// Represents a single uncollapsed cell. ID of -1 means there is no set ID yet.
/*************************************************************************************************
 * struct Tile
 * Represents a single cell on the world grid. Contains the possible states it can collapse into,
 * as well as a function that returns the possible of states an adjacent tile can have. Can return
 * its location and wether or not it is collapsed.
 *************************************************************************************************/
struct Tile {
    bool isCollapsed = false;
    int ID = -1;

    // Represents ange of possible states that this tile can collapse into.
    Range IDRange = Range(0, ALL_TILE_OPTIONS-1);

    // index on world grid, default is non existing.
    int x = -1;
    int y = -1;

    Tile(){}
    Tile(int theX, int theY) {
        x = theX;
        y = theY;
    }

    // update this tiles possible states based on a neighboring tiles ID range.
    void updatePossibleIDs(Range theIDRange) {
        if (IDRange.lower < theIDRange.lower)
            IDRange.lower = theIDRange.lower;
        if (IDRange.upper > theIDRange.upper)
            IDRange.upper = theIDRange.upper; 
        if (IDRange.upper < IDRange.lower) {
            IDRange = Range(IDRange.upper,IDRange.lower);
        }

        // Check if collapsed
        if (IDRange.lower == IDRange.upper){
            ID = IDRange.lower;
            isCollapsed = true;
            collapsedTiles += 1;
        }
    }

    // Can return range that does not exist such as -1 but the current logic will never conflict with updatePossibleIDs
    Range getPossibleConnectors() const {
        return Range(IDRange.lower-1,IDRange.upper+1);
    }
};


Tile World[WORLD_HEIGHT][WORLD_WIDTH];

// Confirm coordinate is within world bounds and is also uncollapsed
bool isExistingUncollapsedTile(int x, int y) {
    return (y < WORLD_WIDTH && x < WORLD_HEIGHT) && (x >= 0 && y >= 0) && !World[x][y].isCollapsed;
}

// Confirm coordinate is within world bounds
bool isExistingTile(int x, int y) {
    return (y < WORLD_WIDTH && x < WORLD_HEIGHT) && (x >= 0 && y >= 0);
}

// Confirm tile ID exists.
bool isValidTileID(int i) {
    if (i < ALL_TILE_OPTIONS && i >= 0)
        return true;
    return false;
}
void collapse(Tile &t);
// Given a tile return the tiles adjacent to it. Does not count corners as adjacent.
vector<reference_wrapper<Tile>> getAdjacentTiles(Tile &t) {

    vector<reference_wrapper<Tile>> tiles;
    // Get all existing neighbor tiles
    selectedX = t.x;
    selectedY = t.y-1;
    if (isExistingTile(selectedX,selectedY))
        tiles.push_back(World[selectedX][selectedY]);
    
    selectedX = t.x;
    selectedY = t.y+1;
    if (isExistingTile(selectedX,selectedY))
        tiles.push_back(World[selectedX][selectedY]);

    selectedX = t.x-1;
    selectedY = t.y;
    if (isExistingTile(selectedX,selectedY))
        tiles.push_back(World[selectedX][selectedY]);

    selectedX = t.x+1;
    selectedY = t.y;
    if (isExistingTile(selectedX,selectedY))
        tiles.push_back(World[selectedX][selectedY]);

    return tiles;

}

// Updates this tiles possible states based off of adjacent tiles. Cascades changes to adjacent tiles if there is a change on this tile.
void updateAndCascadeTile(Tile &t) {
    // Get adjacent tiles and update the current tiles possible tile IDs.
 
    vector<reference_wrapper<Tile>> adjacent = getAdjacentTiles(t);
    vector<reference_wrapper<Tile>>::iterator it = adjacent.begin();
    Range OriginalRange = t.IDRange;

    // Update this Tile according to the possible states of the adjacent tiles.
    while(!t.isCollapsed && it != adjacent.end()) {
        t.updatePossibleIDs(it->get().getPossibleConnectors());
        ++it;
    }

    // If the possible states of this tile changed, cascade to the adjacent tiles.
    if(t.IDRange != OriginalRange) {
        it = adjacent.begin();
        while (it != adjacent.end() && !it->get().isCollapsed) {
            updateAndCascadeTile(*it);
            ++it;
        }
    }

    if (!t.isCollapsed)
        collapse(t);

    return;
}

// Updates this tiles possible states based off of adjacent tiles.. Forces to cascade even if the tile does not have a change in its possible states.
// Should only be used after manually collapsing a tile.
void updateAndCascadeTileForced(Tile &t) {

    // Get adjacent tiles and update the current tiles possible tile IDs.
    vector<reference_wrapper<Tile>> adjacent = getAdjacentTiles(t);
    vector<reference_wrapper<Tile>>::iterator it = adjacent.begin();

    // Update this Tile according to the possible states of the adjacent tiles.
    while(!t.isCollapsed && it != adjacent.end()) {
        t.updatePossibleIDs(it->get().getPossibleConnectors());
        ++it;
    }

    // Force cascade changes to the adjacent tiles.
    it = adjacent.begin();
    while (it != adjacent.end()) {
        updateAndCascadeTile(*it);
        ++it;
    }
    return;
}

void collapse(Tile &t) {
    if (!t.isCollapsed) {
        t.isCollapsed = true;
        collapsedTiles++;
    }

    // Pick a random state from available states, or just become the only possible state.
    if (t.IDRange.upper != t.IDRange.lower){ 
        t.ID = t.IDRange.lower + (rand() % (t.IDRange.upper - t.IDRange.lower + 1));
    }
    else
        t.ID = t.IDRange.lower;
    
    t.IDRange.lower = t.ID;
    t.IDRange.upper = t.ID;
    
    // Update adjacent tiles, then collapse until entire world is collapsed.
    vector<reference_wrapper<Tile>> adjacent;
    vector<reference_wrapper<Tile>>::iterator it;
    updateAndCascadeTileForced(t);
    adjacent = getAdjacentTiles(t);
    it = adjacent.begin();
    // Collapse nearby tiles
    for (it = adjacent.begin(); it != adjacent.end(); ++it){
        if (!it->get().isCollapsed){
            collapse(it->get());
        }
    }
    
    return;
}


int main() {

    /***********************
        Create the world!
    ************************/

    // Fill world with uncollapsed tiles.
    for (int i = 0; i < WORLD_HEIGHT; i++)
        for (int j = 0; j < WORLD_WIDTH; j++)
            World[i][j] = Tile(i,j);

    // Collapse first tile at random.
    srand(time(0));
    int x = rand()%WORLD_HEIGHT;
    int y = rand()%WORLD_WIDTH;
    collapse(World[x][y]);

    cout << "-------------DEBUG---------------" << endl;
    cout << "first collapse at " << x << "," << y << endl;
    cout << "After first collapse, int collapsedTiles:" << collapsedTiles << " of " << WORLD_HEIGHT*WORLD_WIDTH << endl;
    // Catch all remaining uncollapsed tiles.
    for (int h = 0; h < WORLD_HEIGHT; h+=1) {
        for (int w = 0; w < WORLD_WIDTH; w+=1) {
            if (collapsedTiles == WORLD_HEIGHT*WORLD_WIDTH)
                break;
            if (!World[h][w].isCollapsed)
                updateAndCascadeTile(World[h][w]);
        }
    }
   //DEBUG
   cout << "After cleanup loop, int collapsedTiles:" << collapsedTiles << " of " << WORLD_HEIGHT*WORLD_WIDTH << endl;
    /**********************
        GAME LOOP
    ***********************/

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "WaveFunctionCollapse");
    while(!WindowShouldClose()) {
        
        // update

        // Draw
        BeginDrawing();

        ClearBackground(PINK);
        // DrawRectangle(SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 34, 34, BLUE);
        // Draw initial world
        

        for (int h = 0; h < WORLD_HEIGHT; h++) {
            for (int w = 0; w < WORLD_WIDTH; w++) {
                DrawRectangle(w*TILE_SIZE, h*TILE_SIZE, TILE_SIZE, TILE_SIZE, COLORS[World[h][w].ID]);
            }
        }

        EndDrawing();

    }

    return 0;
}