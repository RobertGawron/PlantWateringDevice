include <Constants.scad>

// Parameters
hole_distance = 5;       // 5mm from PCB edge
hole_diameter = 3.5;     // 3.5mm hole diameter

// Edge cutout parameters
cutout_x_start = 0;
cutout_x_end = 4;
cutout_y_start = 10;
cutout_y_end = 18;
cutout_width = cutout_x_end - cutout_x_start;
cutout_height = cutout_y_end - cutout_y_start;

// PCB base shape (2D)
module pcb_base() {
    square([pcb_size, pcb_size], center = true);
}

// Extra area for LCD and potentiometer mounting
module lcd_base() {
    translate([pcb_size/2 + lcd_holder_width/2, 0, 0]) 
        square([lcd_holder_width, pcb_size], center = true);
}

// Single mounting hole (2D)
module mounting_hole_2d(x, y) {
    actual_diameter = hole_diameter + hole_tolerance;
    translate([x, y, 0]) 
        circle(d = actual_diameter);
}

// All mounting holes (2D)
module mounting_holes_2d() {
    half_size = pcb_size / 2;
    
    // PCB mounting holes
    mounting_hole_2d(-half_size + hole_distance, -half_size + hole_distance);  // Bottom-left
    mounting_hole_2d(half_size - hole_distance, -half_size + hole_distance);   // Bottom-right
    mounting_hole_2d(half_size - hole_distance, half_size - hole_distance);    // Top-right
    mounting_hole_2d(-half_size + hole_distance, half_size - hole_distance);   // Top-left
}

// Cutout for pump wires
module pump_wires_cutout_2d() {
    half_size = pcb_size / 2;
    
    translate([cutout_x_start - half_size, cutout_y_start - half_size, 0])
        square([cutout_width, cutout_height]);
}

// LCD holder with mounting holes
module lcd_holder_2d() {
    half_size = pcb_size / 2;
    
    difference() {
        lcd_base();
        // Outside holes on the LCD side
        mounting_hole_2d(half_size + lcd_holder_width - hole_distance, half_size - hole_distance);
        mounting_hole_2d(half_size + lcd_holder_width - hole_distance, -half_size + hole_distance);
    }
}

// Main 2D mounting board
module mounting_board_2d() {

    union ()
    {
  difference() {
        pcb_base();
        mounting_holes_2d();
        pump_wires_cutout_2d();
    }
    lcd_holder_2d();
    }
  
}

// 3D extruded version
module mounting_board_3d() {
    linear_extrude(height = mainboard_thickness) {
        mounting_board_2d();
    }
}

// Uncomment one of the following to render:
// mounting_board_2d();  // For 2D view
// mounting_board_3d();  // For 3D extruded version