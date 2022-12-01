/**
 * \file ims_project.cpp
 * \authors Tomas Matus <xmatus37@stud.fit.vutbr.cz>, Adam Zvara <xzvara01@stud.fit.vutbr.cz>
 * \brief Simulation of ships passing through panama canal
 * \date 2022-12
 */

#include "simlib.h"

#define SMALL_CAPACITY 7000
#define LARGE_CAPACITY 100000
#define TRAVEL_TIME    12*60

int shipCounter;

Store AtlanticLocks("Atlantic Locks", 2);
Store PacificLocks("Pacific Locks", 2);

Histogram Table("Transit time", 700, 30, 20);
// TStat TimeStat("Time statistic");

void shipBehavior(Process *s, bool fromAtlantic) {
    if (fromAtlantic) {
        // ship is from atlantic ocean, start with atlantic lock
        s->Enter(AtlanticLocks, 1);
        s->Wait(Exponential(45));
        s->Leave(AtlanticLocks, 1);
    } else {
        // ship is from pacific ocean, start with atlantic lock
        s->Enter(PacificLocks, 1);
        s->Wait(Exponential(45));
        s->Leave(PacificLocks, 1);
    }
    s->Wait(TRAVEL_TIME);
    if (fromAtlantic) {
        // ship is from pacific ocean, end with atlantic lock
        s->Enter(PacificLocks, 1);
        s->Wait(Exponential(45));
        s->Leave(PacificLocks, 1);
    } else {
        // ship is from atlantic ocean, end with atlantic lock
        s->Enter(AtlanticLocks, 1);
        s->Wait(Exponential(45));
        s->Leave(AtlanticLocks, 1);
    }
}

class SmallShip: public Process {
    int capacity;
    double arrivalTime;
    bool fromAtlantic;
    void Behavior() {
        arrivalTime = Time;
        shipBehavior(this, fromAtlantic);
        Table(Time-arrivalTime);
    }
public:
    SmallShip(int c, bool fa) {
        capacity = c;
        fromAtlantic = fa;
        Activate();
    }
};


class LargeShip: public Process {
    int capacity;
    double arrivalTime;
    bool fromAtlantic;
    void Behavior() {
        arrivalTime = Time;
        shipBehavior(this, fromAtlantic);
        Table(Time-arrivalTime);
    }
public:
    LargeShip(int c, bool fa) {
        capacity = c;
        fromAtlantic = fa;
        Activate();
    }
};

class SmallShipGenerator: public Event {
    void Behavior() {
        // todo: different capacities for small boats
        new LargeShip(SMALL_CAPACITY, Random() < 0.5);
        shipCounter++;
        Activate(Time+Exponential(40)); // Every hour new small ship arrives
    }
public:
    SmallShipGenerator() {
        Activate();
    }
};

class LargeShipGenerator: public Event {
    void Behavior() {
        // todo: different capacities for small boats
        new LargeShip(LARGE_CAPACITY, Random() < 0.5);
        shipCounter++;
        Activate(Time+Exponential(40)); // Every hour new small ship arrives
    }
public:
    LargeShipGenerator() {
        Activate();
    }
};

int main() {
    SetOutput("simulation.out");
    Print("Starting panama simulation\n");
    // initialize simulation
    Init(0, 60*24*31); // Simulation time in minutes per day?
    // create generators
    new LargeShipGenerator();
    new SmallShipGenerator();
    // run simulation
    Run();
    AtlanticLocks.Output();
    PacificLocks.Output();
    Table.Output();
    SIMLIB_statistics.Output();
    Print(shipCounter/31);
}