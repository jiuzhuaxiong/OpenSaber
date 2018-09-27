D_FORE = 29.0;
DZ_FORE = 63.0;

D_AFT_MID = 31.75;
D_MID_OUTER = 36.8;
DZ_MID = 25.6;          // inner length with the fore/aft attached
DZ_MID_OUTER = 50.7;    // the length of the midsection on the outside

D_AFT_MID = 31.75;
DZ_AFT = 76.3;
D_AFT_RING = 33.5;  // FIXME
DZ_AFT_RING = 5.5;

DZ_THREAD = 10.5;
DZ_SPKR_HOLDER = 8;
DZ_BUTTRESS = 3;

M_AFT_BACK = DZ_THREAD - DZ_AFT;
M_AFT_FRONT = DZ_THREAD;
M_MID_BACK = M_AFT_FRONT;
M_MID_FRONT = M_MID_BACK + DZ_MID;
M_FORE_BACK = M_MID_FRONT;
M_FORE_FRONT = M_FORE_BACK + DZ_FORE;

M_MID_CENTER = (M_MID_BACK + M_MID_FRONT) / 2;

PLATE_DX = 10;
PLATE_DZ = 44;
PLATE_MOUNT = 16.8;
PLATE_MOUNT_WITH_THREAD = 24.8;
BOX_DX = 14;
BOX_Y = 5.0;

D_POWER_PORT = 8.0; // FIXME
