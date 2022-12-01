/**
 * \file ims_project.cpp
 * \authors Tomas Matus <xmatus37@stud.fit.vutbr.cz>, Adam Zvara <xzvara01@stud.fit.vutbr.cz>
 * \brief Simulation of ships passing through panama canal
 * \date 2022-12
 */

#include "simlib.h"

int main() {
    SetOutput("test.out");
    Print("print into file");
    Init(0, 1000);
    Run();
}