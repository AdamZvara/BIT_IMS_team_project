/**
 * \file ims_project.cpp
 * \authors Tomas Matus <xmatus37@stud.fit.vutbr.cz>, Adam Zvara <xzvara01@stud.fit.vutbr.cz>
 * \brief Simulation of ships passing through panama canal
 * \date 2022-12
 */

#include "simlib.h"

#define SMALL_CAPACITY 50000     // panamax ship tonnage capacity
#define LARGE_CAPACITY 14000    // neopanamax ship TEU capacity
#define TRAVEL_TIME    11*60
#define TIME_IN_LOCK   90

int pacificShips;
int atlanticShips;
int shipCounter;

Store AtlanticLocks("Atlantic Locks", 2);
Store PacificLocks("Pacific Locks", 2);

Histogram Table("Transit time", 8, 1, 18);

/** Function simulating passage of single ship through one panama lock */
void lockPassage(Process *p, Store &lock) {
    p->Enter(lock, 1);
    p->Wait(TIME_IN_LOCK);
    p->Leave(lock, 1);
}

/** Function simulating passage of single ship through whole panama canal */
void shipPassage(Process *ship, bool fromAtlantic) {
    if (fromAtlantic) {
        atlanticShips++;
        lockPassage(ship, AtlanticLocks); // ship is from atlantic ocean, start with atlantic lock
    } else {
        pacificShips++;
        lockPassage(ship, PacificLocks); // ship is from pacific ocean, start with pacific lock
    }
    ship->Wait(TRAVEL_TIME); // traveling through main part of canal
    if (fromAtlantic) {
        lockPassage(ship, PacificLocks); // ship is from pacific ocean, end with atlantic lock
    } else {
        lockPassage(ship, AtlanticLocks); // ship is from atlantic ocean, end with atlantic lock
    }
}

class SmallShip: public Process {
    int capacity;
    double arrivalTime;
    bool fromAtlantic;

    void Behavior() {
        arrivalTime = Time;
        shipPassage(this, fromAtlantic);
        Table((Time-arrivalTime)/60); // total time in canal (in hours)
    }
public:
    SmallShip(int c, bool fa) : capacity(c), fromAtlantic(fa) {
        Activate();
    }
};

class LargeShip: public Process {
    int capacity;
    double arrivalTime;
    bool fromAtlantic;
    void Behavior() {
        arrivalTime = Time;
        shipPassage(this, fromAtlantic);
        Table(Time-arrivalTime);
    }
public:
    LargeShip(int c, bool fa) {
        capacity = c;
        fromAtlantic = fa;
        Activate();
    }
};

class PanamaxShipGenerator: public Event {
    void Behavior() {
        // each ship is randomly generated at pacific or atlantic side
        new SmallShip(SMALL_CAPACITY, Random() < 0.5);
        shipCounter++;
        Activate(Time+Normal(50, 3)); // Every hour new small ship arrives
    }
public:
    PanamaxShipGenerator() {
        Activate();
    }
};

class NeoPanamaxShipGenerator: public Event {
    void Behavior() {
        // each ship is randomly generated at pacific or atlantic side
        new LargeShip(LARGE_CAPACITY, Random() < 0.5);
        shipCounter++;
        Activate(Time+Exponential(40)); // Every hour new small ship arrives
    }
public:
    NeoPanamaxShipGenerator() {
        Activate();
    }
};

void printShipInfo() {
    Print("------------------------------\n", shipCounter);
    Print("Pacific side ships:\t%d\n", pacificShips);
    Print("Atlanitic side ships:\t%d\n", atlanticShips);
    Print("Overall ships:\t\t%d\n", shipCounter/31);
    Print("------------------------------\n", shipCounter);
}

int main() {
    SetOutput("simulation.out");
    Print("Starting panama simulation\n");

    // initialize simulation
    Init(0, 60*24*365); // Simulation time in minutes for one month

    // create generators
    // new NeoPanamaxShipGenerator();
    new PanamaxShipGenerator();

    // run simulation
    Run();

    // print information
    AtlanticLocks.Output();
    PacificLocks.Output();
    Table.Output();
    printShipInfo();
}