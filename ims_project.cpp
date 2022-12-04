/**
 * \file ims_project.cpp
 * \authors Tomas Matus <xmatus37@stud.fit.vutbr.cz>, Adam Zvara <xzvara01@stud.fit.vutbr.cz>
 * \brief Simulation of ships passing through panama canal
 * \date 2022-12
 */

#include <ctime>
#include <unistd.h>
#include <string>
#include "simlib.h"

#define DEBUG 0
#define dprint(...) {if(DEBUG==1)Print(__VA_ARGS__);}

/** Simulation parameters */
#define TRAVEL_TIME    Normal(10*60, 60)    // Canal main passage time in minutes
#define TIME_IN_LOCK   105      // Single lock passage time in minutes
#define SIMDAYS        31       // Simulation days
#define CANAL_CAPACITY 20       // Maximum ships in canal at single time
#define SMALL_CAPACITY 28700    // Panamax ship cargo tonnage

/** Global variables */
int pacificShips;
int atlanticShips;
int shipCounter;
int overallTEU;
bool priorityExit = false;
bool emptyQueues = false;

Store AtlanticLocks("Atlantic Locks", 2);
Store PacificLocks("Pacific Locks", 2);
Store CanalCapacity("Overall Capacity", CANAL_CAPACITY);

Histogram Table("Transit time", 11, 1, 10);

/** Ship class containing information about each ship */
class Ship: public Process {
public:
    int capacity;
    double arrivalTime;
    bool fromAtlantic;
};

/** Simulate passage of single ship through panama lock */
void lockPassage(Process *p, Store &lock) {
    p->Enter(lock, 1);
    p->Wait(TIME_IN_LOCK);
    p->Leave(lock, 1);
}

/** Locking and unlocking canal capacity */
void lockCanal(Process *p) {p->Enter(CanalCapacity, 1);}
void unlockCanal(Process *p) {p->Leave(CanalCapacity, 1);}

/** Simulate ship entering panama canal */
int shipEnter(Ship *ship, Store &s, int &ship_counter) {
    // CanalCapacity is okay, continue with simulation
    lockCanal(ship); // Add new ship to canals current capacity
    dprint("New in canal: %d\n", CanalCapacity.Used());
    lockPassage(ship, s); // Ship passage of entry lock
    ship_counter++;
    return 0;
}

/** Simulate ship exiting canal */
void shipExit(Ship *ship, Store &s) {
    lockPassage(ship, s); // Ship passage of exit lock
    unlockCanal(ship); // Unlock canal capacity

    dprint("Leaving canal: %d\n", CanalCapacity.Used());
    overallTEU += ship->capacity;
}

/** Function simulating passage of single ship through whole panama canal */
void shipPassage(Ship *ship, bool fromAtlantic) {
    if (fromAtlantic) {
        if (shipEnter(ship, AtlanticLocks, atlanticShips) < 0) {
            return;
        }
    } else {
        if (shipEnter(ship, PacificLocks, pacificShips) < 0) {
            return;
        }
    }

    ship->arrivalTime = Time - TIME_IN_LOCK; // Set arrival time after ship actually went into entry lock
    shipCounter++;
    ship->Wait(TRAVEL_TIME); // Traveling through main part of canal

    if (fromAtlantic) {
        shipExit(ship, PacificLocks);
    } else {
        shipExit(ship, AtlanticLocks);
    }
}

class PanamaxShip: public Ship {
    void Behavior() {
        shipPassage(this, fromAtlantic);
        Table((Time-arrivalTime)/60); // Total time in canal (in hours)
    }
public:
    PanamaxShip(int c, bool fa) {
        capacity = c;
        fromAtlantic = fa;
        Activate();
    }
};

class Failure: public Process {
    void Behavior() {
        Table((Time-arrivalTime)/60); // Total time in canal (in hours)
    }
public:
    Failure() {
        Activate();
    }
};


class PanamaxShipGenerator: public Event {
    void Behavior() {
        // Each ship is randomly generated at pacific or atlantic side
        new PanamaxShip(SMALL_CAPACITY, Random() < 0.5);
        Activate(Time+55); // Each hour new panamax ship arrives
    }
public:
    PanamaxShipGenerator() {
        Activate();
    }
};

class LockFailure: public Process {
    void Behavior() {

    }
};

class LockBlockage: public Event {
    void Behavior() {
        // Generate blockage of canal lock
        new LockFailure();
        Activate(Time+55); // Each hour new panamax ship arrives
    }
public:
    LockBlockage() {
        Activate();
    }
};


void printStat() {
    AtlanticLocks.Output();
    PacificLocks.Output();
    CanalCapacity.Output();
    Table.Output();
    Print("------------------------------------------------\n");
    Print("Pacific side ships:\t\t%d\n", pacificShips);
    Print("Atlanitic side ships:\t\t%d\n", atlanticShips);
    Print("Overall ships:\t\t\t%d\n", shipCounter);
    Print("Ships per day:\t\t\t%d\n", shipCounter/SIMDAYS);
    Print("Overall TEU:\t\t\t%d\n", overallTEU);
    Print("------------------------------------------------\n");
}

void validate_model() {
    SetOutput("simulation.out");
    Print("Validate panama simulation model\n");
    // Initialize simulation
    Init(0, 60*24*SIMDAYS);
    // Create generators
    new PanamaxShipGenerator();
    // Run simulation
    Run();
    // Print statistics
    printStat();
}

void help() {
    std::string help =
        "Usage: ims_project [OPTION]\n"
        "Simulation of panama canal traffic\n\n"
        "OPTIONS:\n"
        "\t-h print this message\n"
        "\t-v validation of current model\n"
        "\t-e 1 first experiment\n";
    Print("%s", help.c_str());
}

int main(int argc, char *argv[]) {
    RandomSeed(time(NULL)); // Randomize results

    int option;
    while ((option = getopt(argc, argv, "hve:")) != -1) {
        switch (option) {
        case 'h':
            help();
            break;

        case 'v':
            validate_model();
            break;

        default:
            break;
        }
    }
}