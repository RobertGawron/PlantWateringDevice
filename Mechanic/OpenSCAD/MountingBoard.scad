include <Constants.scad>
include <ConnectorCutout.scad>
include <Display.scad>

hole_distance = 5;       // 5mm from PCB edge

// PCB base shape (2D)
module pcb_base() {
    square([pcb_size, pcb_size], center = true);
}

// Extra area for LCD and potentiometer mounting
module lcd_base() {
    translate([pcb_size/2 + display_module_width/2, 0, 0]) 
        square([display_module_width, pcb_size], center = true);
}

// Single mounting hole (2D)
module mounting_hole_2d(x, y) {
    actual_diameter = hole_diameter + manufacturer_tolerance;
    translate([x, y, 0]) 
        circle(d = actual_diameter);
}

// All mounting holes (2D)
module mounting_holes_2d() {
    pcb_half_size = pcb_size / 2;
    
    // PCB mounting holes
    mounting_hole_2d(-pcb_half_size + hole_distance, -pcb_half_size + hole_distance);  // Bottom-left
    mounting_hole_2d(pcb_half_size - hole_distance, -pcb_half_size + hole_distance);   // Bottom-right
    mounting_hole_2d(pcb_half_size - hole_distance, pcb_half_size - hole_distance);    // Top-right
    mounting_hole_2d(-pcb_half_size + hole_distance, pcb_half_size - hole_distance);   // Top-left
}

// LCD holder with mounting holes
module board_for_display_holder_2d() {   
    difference() {
        lcd_base();
        // Outside holes on the LCD side
        mounting_hole_2d(pcb_half_size + display_module_width - hole_distance, pcb_half_size - hole_distance);
        mounting_hole_2d(pcb_half_size + display_module_width - hole_distance, -pcb_half_size + hole_distance);
    }
}

// Main 2D mounting board
module mounting_board_2d() {
    difference() {
        union() {
            difference() {
                pcb_base();
                mounting_holes_2d();
                connector_cutout_2d();
            }
            board_for_display_holder_2d();
        } 
        translate([pcb_size/2 + display_module_width, 0, 0]) {
            display_holder_2d();
        }
    }
}

// 3D extruded version
module mounting_board_3d() {
    linear_extrude(height = mainboard_thickness) {
        mounting_board_2d();
    }
}