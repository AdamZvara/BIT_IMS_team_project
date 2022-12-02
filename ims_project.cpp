/**
 * \file ims_project.cpp
 * \authors Tomas Matus <xmatus37@stud.fit.vutbr.cz>, Adam Zvara <xzvara01@stud.fit.vutbr.cz>
 * \brief Simulation of ships passing through panama canal
 * \date 2022-12
 */

#include <ctime>
#include "simlib.h"

#define DEBUG 0
#define dprint(...) {if(DEBUG==1)Print(__VA_ARGS__);}

/** Simulation parameters */
#define TRAVEL_TIME    11*60    // Canal main passage time in minutes
#define TIME_IN_LOCK   110      // Single lock passage time in minutes
#define SIMDAYS        31       // Simulation days
#define SIMMONTHS      1        // Simulation months (used for simulation of whole year)
#define CANAL_CAPACITY 40       // Maximum ships in canal at single time
#define SMALL_CAPACITY 50000    // Panamax ship tonnage capacity
#define LARGE_CAPACITY 14000    // Neopanamax ship TEU capacity

/** Global variables */
int pacificShips;
int atlanticShips;
int shipCounter;
bool priorityExit = false;
bool emptyQueues = false;
bool priorityReached = false;

Store AtlanticLocks("Atlantic Locks", 2);
Store PacificLocks("Pacific Locks", 2);
Store CanalCapacity("Overall Capacity", CANAL_CAPACITY);

Queue AtlanticQueue("Atlantic Queue");
Queue PacificQueue("Pacific Queue");

Histogram Table("Transit time", 8, 1, 30);

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

void emptyQueue(Queue &q) {
    while (!q.empty()) {
        Entity *tmp = q.GetFirst();
        tmp->Activate();
    }
}

/** Locking and unlocking canal capacity */
void lockCanal(Process *p) { p->Enter(CanalCapacity, 1);}
void unlockCanal(Process *p) { p->Leave(CanalCapacity, 1);}

/** Set state of panama canal
 *  When maximum capacity of canal is reached, prioritize ships in canal (which want to leave)
 *  When capacity gets under half of the capacity, allow more ships to come in
*/
void updateCanalState() {
    if (!priorityExit && (CanalCapacity.Used() == CanalCapacity.Capacity())) {
        dprint("Canal capacity reached!\n")
        priorityExit = true;
    } else if (priorityExit && (CanalCapacity.Used() < (CanalCapacity.Capacity() / 2))) {
        dprint("Canal capacity at 50%, ships can go in now!\n");
        priorityExit = false;
        emptyQueues = true;
    }
}

/** Simulate ship entering panama canal */
int shipEnter(Ship *ship, Queue &q, Store &s, int &ship_counter, char *queue_name) {
    if (priorityExit) {
        // If canal current capacity is above 50%, wait in queue
        if (q.Length() >= 5) {
            // If queue capacity is high, canal can be flooded, therefore queue capacity is limited
            dprint("Queue too long, leaving system!\n");
            ship->Cancel();
            return -1;
        }
        q.Insert(ship);
        dprint("New in %s: %d\n", queue_name, q.Length())
        Passivate(ship);
    }
    // CanalCapacity is okay, continue with simulation
    lockCanal(ship); // Add new ship to canals current capacity
    dprint("New in canal: %d\n", CanalCapacity.Used());
    lockPassage(ship, s); // Ship passage of entry lock
    ship_counter++;
    return 0;
}

/** Simulate ship exiting canal */
void shipExit(Ship *ship, Queue &q, Store &s) {
    lockPassage(ship, s); // Ship passage of exit lock
    unlockCanal(ship); // Unlock canal capacity

    dprint("Leaving canal: %d\n", CanalCapacity.Used());

    if (emptyQueues) {
        // If canal capacity was at max value and dropped under 50%, activate ships in queues
        dprint("Emptying queues\n");
        emptyQueue(PacificQueue);
        emptyQueue(AtlanticQueue);
        emptyQueues = false;
    }
}

/** Function simulating passage of single ship through whole panama canal */
void shipPassage(Ship *ship, bool fromAtlantic) {
    updateCanalState();

    if (fromAtlantic) {
        if (shipEnter(ship, AtlanticQueue, AtlanticLocks, atlanticShips, "AtlanticQueue") < 0) {
            return;
        }
    } else {
        if (shipEnter(ship, PacificQueue, PacificLocks, pacificShips, "PacificQueue") < 0) {
            return;
        }
    }

    ship->arrivalTime = Time - TIME_IN_LOCK; // Set arrival time after ship actually went into entry lock
    shipCounter++;
    ship->Wait(TRAVEL_TIME); // Traveling through main part of canal

    if (fromAtlantic) {
        shipExit(ship, AtlanticQueue, PacificLocks);
    } else {
        shipExit(ship, PacificQueue, AtlanticLocks);
    }
}

class PanamaxShip: public Ship {
    void Behavior() {
        shipPassage(this, fromAtlantic);
        Table((Time-arrivalTime)/60); // total time in canal (in hours)
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
        // each ship is randomly generated at pacific or atlantic side
        new PanamaxShip(SMALL_CAPACITY, Random() < 0.5);
        Activate(Time+60);
    }
public:
    PanamaxShipGenerator() {
        Activate();
    }
};

void printShipInfo() {
    Print("------------------------------\n");
    Print("Pacific side ships:\t%d\n", pacificShips);
    Print("Atlanitic side ships:\t%d\n", atlanticShips);
    Print("Overall ships:\t\t%d\n", shipCounter);
    Print("Ships per day:\t\t%d\n", shipCounter/SIMDAYS);
    Print("------------------------------\n");
}

int main() {
    SetOutput("simulation.out");
    Print("Starting panama simulation\n");
    RandomSeed(time(NULL)); // randomize results

    // Change i for multiple simulations (e.g year simulation)
    for(int i=1; i<=SIMMONTHS; i++)  {
        // initialize simulation
        Init(0, 60*24*SIMDAYS);

        AtlanticLocks.Clear();
        PacificLocks.Clear();
        CanalCapacity.Clear();
        Table.Clear();

        // create generators
        new PanamaxShipGenerator();

        // run simulation
        Run();

        // print information
        AtlanticLocks.Output();
        PacificLocks.Output();
        CanalCapacity.Output();
        Table.Output();
        printShipInfo();
    }
}