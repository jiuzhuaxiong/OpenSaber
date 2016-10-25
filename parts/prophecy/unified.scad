use <../threads.scad>
use <../shapes.scad>
include <dim.scad>

M_DOTSTAR_EDGE = M_DOTSTAR - X_DOTSTAR / 2;

INCHES_TO_MM = 25.4;
MM_TO_INCHES = 1 / 25.4;
$fn = 90;
EPS = 0.01;
EPS2 = EPS * 2;

H_HEAT_SINK_THREAD = 0.37 * INCHES_TO_MM;
H_VOL			   = 16;

module innerTube()
{
    translate([0, 0, M_WAY_BACK]) {
        cylinder(d=D_AFT, h=M_TRANSITION - M_WAY_BACK);
    }
    translate([0, 0, M_TRANSITION]) {
        cylinder(d=D_FORWARD, h = H_FAR);
    }
}

// LED / button positive (above axis)
// thread negative (below axis)
//
module switch()
{
    D_OUTER_TOP = 13.8;
    D_INNER_TOP = 11.0;
    H_TOP       =  1.5;
    H_BODY      = 13;   // approx. connections below.

    color("yellow") {
        translate([0, 0, M_SWITCH_CENTER]) {
            rotate([-90, 0, 0]) {
                translate([0, 0, Y_SWITCH]) {
                    cylinder(h = H_TOP, r1 = D_OUTER_TOP / 2, r2 = D_INNER_TOP / 2);
                    translate([0, 0, -H_BODY]) {
                        cylinder(h = H_BODY, d=D_SWITCH);
                    }            
                }
            }

        }
    }
}

module ledHolder()
{
    translate([0, 0, M_LED_HOLDER_BACK]) {
        H = M_LED_HOLDER_FRONT - M_LED_HOLDER_BACK;
        difference() {
            cylinder(h=H, d=D_FORWARD);
            translate([0, 0, -0.1]) {
                cylinder(h=H_HEAT_SINK_THREAD + 0.2, r = 0.8 * INCHES_TO_MM / 2);
            }
        }
    }    
}

module switchAndPortHolder()
{
    POWER_X         = 11;
    POWER_Y         = 14.5;
    POWER_Z         = 10;
    POWER_OFFSET_Y  = 0.2;
    POWER_OFFSET_X  = -1;

    H = (M_LED_HOLDER_BACK - M_TRANSITION + T_TRANSITION_RING);
    T = 3;

    intersection() {
        innerTube();
        difference() {
            translate([-10, Y_SWITCH - T, M_TRANSITION - T_TRANSITION_RING]) {
                cube(size=[20, T, H]);
            }

            // switch
            translate([0, 0, M_SWITCH_CENTER]) {
                rotate([-90, 0, 0]) {
                    cylinder(h=20, d=D_SWITCH);
                }        
            }

            // Port
            translate([0, 0, M_PORT_CENTER + PORT_CENTER_OFFSET]) {
                rotate([-90, 0, 0]) {
                    cylinder(h=20, d=D_PORT);
                }
            }

        }
    }
}

module dotstars(y)
{
    translate([-X_DOTSTAR/2, -y, 0]) {
        for(i=[0:3]) {
            translate([0, 0, DOTSTAR_SPACE * i]) {
                cube(size=[X_DOTSTAR, y, X_DOTSTAR]);
            }
        }
    }   
}


module dotstarHolder() {
    intersection()
    {
        innerTube();

        //M = M_DOTSTAR_EDGE - 2;
        M = M_TRANSITION - T_TRANSITION_RING;

        difference()
        {
            translate([-10, -R_FORWARD, M]) {
                cube(size=[20, Y_DOTSTAR + 1, M_LED_HOLDER_BACK - M]);
            }
            translate([0, -10, M_DOTSTAR_EDGE]) {
                dotstars(10);
            }
        }
    } 
}

module transitionRing()
{
    translate([0, 0, M_TRANSITION - T_TRANSITION_RING]) {
        tube(T_TRANSITION_RING, D_FORWARD/2, D_AFT/2);
    }
}

ledHolder();
switch();
switchAndPortHolder();
dotstarHolder();
transitionRing();