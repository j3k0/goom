#include "GTTestGroup.h"
#include "GTPreferences.h"
#include <stdlib.h>

#ifdef kAliVersionNumber
# define kGroupVersion kAliVersionNumber
#else
# define kGroupVersion "1"
#endif

#define kCurrentGroup "_USER_GROUP_" kGroupVersion
#define kDeprecatedGroup "_USER_GROUP_0"

static float frand() { return ((double)rand() / (double)RAND_MAX); }

namespace gametools {

static int gGroup = -1;

Testgroup GTGetTestGroup()
{
    if (gGroup < 0) {
        int oldgroup = GetIntPreference(kDeprecatedGroup, -1);
        int group    = GetIntPreference(kCurrentGroup, -1);

        // If a current group is set, use it.
        if (group >= 0) {
            gGroup = group;
        }
        // If a deprecated group is set, use no group.
        else if (oldgroup >= 0) {
            gGroup = 'X';
        }
        // No group, generate one.
        else if (group < 0) {
            group = TESTGROUP_A + 6 * frand();
            SetIntPreference(kDeprecatedGroup, group);
            SetIntPreference(kCurrentGroup,    group);
            gGroup = group;
        }
    }
    
    switch (gGroup) {
        case 'A': return TESTGROUP_A;
        case 'B': return TESTGROUP_B;
        case 'C': return TESTGROUP_C;
        case 'D': return TESTGROUP_D;
        case 'E': return TESTGROUP_E;
        case 'F': return TESTGROUP_F;
    }
    return TESTGROUP_NONE;
}

void GTSetTestGroup(Testgroup t) {
    switch (t) {
        case TESTGROUP_A: gGroup = 'A'; break;
        case TESTGROUP_B: gGroup = 'B'; break;
        case TESTGROUP_C: gGroup = 'C'; break;
        case TESTGROUP_D: gGroup = 'D'; break;
        case TESTGROUP_E: gGroup = 'E'; break;
        case TESTGROUP_F: gGroup = 'F'; break;
        default: gGroup = 'X';
    }
    SetIntPreference(kDeprecatedGroup, gGroup);
    SetIntPreference(kCurrentGroup,    gGroup);
}

}
