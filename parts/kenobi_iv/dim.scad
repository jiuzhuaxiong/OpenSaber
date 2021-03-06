D_FORE = 29.0 - 0.4;    // bring the plastic in just a touch
DZ_FORE = 63.0;

D_MID_OUTER = 36.8;

DZ_THREAD = 10.5;
DZ_MID_OUTER = 50.7;                    // the length of the midsection on the outside
DZ_MID = DZ_MID_OUTER - DZ_THREAD * 2;  // inner length with the fore/aft attached

D_AFT_MID = 31.75;
DZ_AFT = 77;
D_AFT_RING = 33.5;
DZ_AFT_RING = 5.5;

DZ_SPKR_HOLDER = 8.5;
DZ_BUTTRESS = 3;

M_AFT_BACK = DZ_THREAD - DZ_AFT;
M_AFT_FRONT = DZ_THREAD;
M_MID_BACK = M_AFT_FRONT;
M_MID_FRONT = M_MID_BACK + DZ_MID - 1.5;
M_FORE_BACK = M_MID_FRONT;
M_FORE_FRONT = M_FORE_BACK + DZ_FORE;

M_MID_CENTER = (M_MID_BACK + M_MID_FRONT) / 2;

PLATE_DX = 10;
PLATE_DZ = 44;
PLATE_MOUNT = 16.8;
PLATE_MOUNT_WITH_THREAD = 24.8;
BOX_DX = 14;
BOX_Y = 5.0;

D_POWER_PORT = 8.0;

D_ROD = 3.6;
D_TUBE = 6;
CAP_D_INNER = 18;
CAP_THETA = 30.0;
CAP_DZ = 4;
CAP_BRASS = 1.0;                // fixme