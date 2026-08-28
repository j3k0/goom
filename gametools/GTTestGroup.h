#ifndef GT_TESTGROUP_H
#define GT_TESTGROUP_H

// Put the player into 1 of 6 test groups... Useful to experiment a feature on a subset of the players.

namespace gametools {

enum Testgroup {
    TESTGROUP_A = 'A',
    TESTGROUP_B,
    TESTGROUP_C,
    TESTGROUP_D,
    TESTGROUP_E,
    TESTGROUP_F,
    TESTGROUP_NONE
};

Testgroup GTGetTestGroup();
void GTSetTestGroup(Testgroup t);

}

#endif
