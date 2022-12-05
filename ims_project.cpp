/**
 * \file ims_project.cpp
 * \authors Tomas Matus <xmatus37@stud.fit.vutbr.cz>, Adam Zvara <xzvara01@stud.fit.vutbr.cz>
 * \brief Simulation of ships passing through panama canal
 * \date 2022-12
 */

#include <ctime>
#include <unistd.h>
#include <string>
#include <cstring>
#include "simlib.h"

#define DEBUG 0
#define dprint(...) {if(DEBUG==1)Print(__VA_ARGS__);}

/** Simulation parameters */
#define TRAVEL_TIME    11*60    // Canal main passage time in minutes
int TIME_IN_LOCK   = 105;      // Single lock passage time in minutes
int SIMDAYS        = 31;       // Simulation days
int CANAL_CAPACITY = 20;       // Maximum ships in canal at single time
int SHIP_CAPACITY  = 28700;    // Panamax ship cargo tonnage
int ACCIDENT_HRS   = 7*24;     // Duration of accident in hours
int ACCIDENT_CNT   = 1;        // Number of accidents in simulation


/** Global variables */
int pacificShips;
int atlanticShips;
int shipCounter;
int overallTEU;
bool priorityExit = false;
bool emptyQueues = false;

Facility AtlanticLock1("Atlantic Lock 1");  // primary exit lock
Facility AtlanticLock2("Atlantic Lock 2");  // primary entry lock
Facility PacificLock1("Pacific Lock 1");    // primary exit lock
Facility PacificLock2("Pacific Lock 2");    // primary entry lock

Store CanalCapacity("Overall Capacity", CANAL_CAPACITY);

Histogram Table("Transit time", 13, 1, 10);

/** Ship class containing information about each ship */
class Ship: public Process {
public:
    int capacity;
    double arrivalTime;
    bool fromAtlantic;
    bool interrupted = false;
};

/** Locking and unlocking canal capacity */
void lockCanal(Process *p) {p->Enter(CanalCapacity, 1);}
void unlockCanal(Process *p) {p->Leave(CanalCapacity, 1);}

/** Simulate ship picking entry lock
 *  Ships can use both locks, if they are availiable, otherwise they wait primarily
 *  in either AtlanticLock2 for atlantic side or PacificLock2 for pacific side
*/
void entryLocks(Ship *s, Facility **lock) {
    if (s->fromAtlantic) {
        if (!AtlanticLock1.Busy()) {
            s->Seize(AtlanticLock1);
            *lock = AtlanticLock1;
            dprint("Ship from atlantic entering alock1\n");
        } else {
            s->Seize(AtlanticLock2);
            *lock = AtlanticLock2;
            dprint("Ship from atlantic entering alock2\n");
        }
    } else {
        if (!PacificLock1.Busy()) {
            s->Seize(PacificLock1);
            *lock = PacificLock1;
            dprint("Ship from atlantic entering plock1\n");
        } else {
            s->Seize(PacificLock2);
            *lock = PacificLock2;
            dprint("Ship from atlantic entering plock2\n");
        }
    }
}

/** Simulate ship picking exit lock
 *  Ships can use both locks, if they are availiable, otherwise they wait primarily
 *  in either AtlanticLock1 for pacific side or PacificLock2 for atlantic side
*/
void exitLocks(Ship *s, Facility **lock) {
    if (s->fromAtlantic) {
        if (!PacificLock2.Busy()) {
            s->Seize(PacificLock2);
            *lock = PacificLock2;
            dprint("Ship from atlantic leaving plock1\n");
        } else {
            s->Seize(PacificLock1);
            *lock = PacificLock1;
            dprint("Ship from atlantic leaving plock2\n");
        }
    } else {
        if (!AtlanticLock2.Busy()) {
            s->Seize(AtlanticLock2);
            *lock = AtlanticLock2;
            dprint("Ship from atlantic leaving alock1\n");
        } else {
            s->Seize(AtlanticLock1);
            *lock = AtlanticLock1;
            dprint("Ship from atlantic leaving alock2\n");
        }
    }
}

/** Simulate passage of single ship through panama lock (can be entry or exit lock) */
void lockPassage(Ship *s, bool exiting) {
    Facility *lock;
    if (!exiting) {
        entryLocks(s, &lock);
    } else {
        exitLocks(s, &lock);
    }
    s->Wait(TIME_IN_LOCK);
    if (s->interrupted) {
        // Ship has been interrupted by accident inside lock
        // Just leave the system (e.g ship is damaged and is tugged away ...)
        dprint("Ship has been interrupted\n");
        unlockCanal(s);
        s->Cancel();
    }
    s->Release(*lock);
}

/** Simulate ship entering panama canal */
void shipEnter(Ship *ship) {
    lockCanal(ship); // Add new ship to canals current capacity
    lockPassage(ship, false); // Ship passage of entry lock
    dprint("New in canal: %d\n", CanalCapacity.Used());
}

/** Simulate ship exiting canal */
void shipExit(Ship *ship) {
    lockPassage(ship, true); // Ship passage of exit lock
    if (CanalCapacity.Used()) {
        unlockCanal(ship); // Unlock canal capacity
    }

    dprint("Leaving canal: %d\n", CanalCapacity.Used());
    overallTEU += ship->capacity;
}

/** Function simulating passage of single ship through whole panama canal */
void shipPassage(Ship *ship, bool fromAtlantic) {
    shipEnter(ship);

    ship->arrivalTime = Time - TIME_IN_LOCK; // Set arrival time after ship actually went into entry lock
    shipCounter++;
    ship->Wait(TRAVEL_TIME); // Traveling through main part of canal

    (fromAtlantic ? atlanticShips : pacificShips) += 1;
    shipExit(ship);
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

class PanamaxShipGenerator: public Event {
    void Behavior() {
        // Each ship is randomly generated at pacific or atlantic side
        new PanamaxShip(SHIP_CAPACITY, Random() < 0.5);
        Activate(Time+55); // Each hour new panamax ship arrives
    }
public:
    PanamaxShipGenerator() {
        Activate();
    }
};

class RepairLock: public Process {
    Facility &lock;
    bool exit;
    void Behavior() {
        dprint("Incident occured, repair in progress\n");
        Ship *s = (Ship *)lock.In();
        Seize(lock, 1);
        if (s != nullptr) {
            // Set interrupted flag to ship and activate it
            s->interrupted = true;
            s->Activate();
        }
        Wait(60*ACCIDENT_HRS);
        Release(lock);
    }
public:
    RepairLock(Facility &lock, bool exit): lock(lock), exit(exit) {
        Activate();
    }
};

class LockAccicentGenerator: public Event {
    Facility &lock;
    bool isExitLock;
    void Behavior() {
        // Generate blockage of canal lock
        new RepairLock(lock, isExitLock);
        // Activate(Time+(SIMDAYS/ACCIDENT_CNT)*60*24);
    }
public:
    LockAccicentGenerator(Facility &blockedLock, bool exitLock): lock(blockedLock), isExitLock(exitLock) {
        Activate(Time+60*24*Normal(15, 5)); // Generate single canal blockage
    }
};

void printStat() {
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
    AtlanticLock1.Output();
    AtlanticLock2.Output();
    PacificLock1.Output();
    PacificLock2.Output();
    printStat();
}

void experiment1() {
    SetOutput("simulation.out");
    Print("Experiment 1 - Accident in exit canal\n");
    // Initialize simulation
    Init(0, 60*24*SIMDAYS);
    // Create generators
    new PanamaxShipGenerator();
    new LockAccicentGenerator(AtlanticLock1, false);
    // Run simulation
    Run();
    // Print statistics
    printStat();
}

void experiment2() {
    SetOutput("simulation.out");
    Print("Experiment 2 - Accident in entry canal\n");
    // Initialize simulation
    Init(0, 60*24*SIMDAYS);
    // Create generators
    new PanamaxShipGenerator();
    new LockAccicentGenerator(AtlanticLock2, false);
    new LockAccicentGenerator(AtlanticLock1, false);
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

        case 'e':
            if (!strcmp(optarg, "1")) {
                experiment1();
            } else if (!strcmp(optarg, "2")) {
                experiment2();
            }
            break;

        default:
            break;
        }
    }
}