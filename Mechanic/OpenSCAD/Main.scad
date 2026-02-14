include <MountingBoard.scad>
include <PumpHolder.scad>
include <Potentiometer.scad>
include <ZipTieBracket.scad>

$fn = smoothness;

// The pump doesn't need to be exactly at the center; it was slightly moved
// to give space for socket connector. This offset is important for pump
// holder and zip ties
pump_x_offset = 2;
pump_y_offset = 3;

module model_without_zipties_3d() {
    union() {
        mounting_board_3d();
        
        translate([pump_x_offset, -pcb_size/2 + pump_y_offset, mainboard_thickness]) {
            pump_rails_3d();
        }

        translate([pcb_size/2, 0, mainboard_thickness]) {
            rotate([0, 0, 90]) {
                potentiometer_holder_3d();
            }
        }
    }
}

module whole_model_3d() {
    difference() {
        model_without_zipties_3d();
        
       // ziptie_extrude_height = mainboard_thickness + pump_thickness_z;
      /*  linear_extrude(height = ziptie_extrude_height) {
            translate([pump_x_offset, 0, 0]) {
                zipties_2d();
            }
        }*/
    }
}

whole_model_3d();