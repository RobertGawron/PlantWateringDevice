

include <MountingBoard.scad>
include <PumpHolder.scad>
include <Display.scad>
include <Potentiometer.scad>

$fn = smoothness;

difference()
{
    mounting_board_3d();
    
    translate([pcb_size/2 +lcd_holder_width, 0, 0]) {
       // rotate([0, 0, 90]) {
            display_holder_3d();
       // }
    }
}

// Position on mainboard with thickness, roughly centered to save space
arbitrary_y_offset_for_fitting = 3;
translate([0, -pcb_size/2 + arbitrary_y_offset_for_fitting, mainboard_thickness]) {
    pump_rails_3d();
}

translate([pcb_size/2/*+lcd_holder_width*/,0 , mainboard_thickness])
{
rotate([0, 0, 90]) {
    potentiometer_holder_3d();
}
}
