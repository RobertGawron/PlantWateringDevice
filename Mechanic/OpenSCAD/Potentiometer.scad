include <Constants.scad>

// Parameters
clearance_extra = 3;  // Extra clearance for wires, solder joints, and mounting tolerance

// Potentiometer body dimensions (from datasheet with added clearance)
pot_body_width = 17; 
pot_body_height = pot_body_width/2+13 ;//+ clearance_extra;  
pot_thickness_no_shaft = 9.2;
// Support structure dimensions
support_thickness = 3;
support_height = pot_body_width +1;   // Height of extrusion in z axis plus extra space
side_support_length = pot_thickness_no_shaft;              // Depth of bracing perpendicular to main support

// Cutout for potentiometer shaft
shaft_diameter = 7.2 + manufacturer_tolerance;

module potentiometer_holder_without_mounting_hole_3d() {
    linear_extrude(height = support_height) {
        // Main horizontal strip that holds potentiometer body
        translate([0, support_thickness/2, 0]) {
            square([pot_body_width, support_thickness], center = true);
        }
        
        // Right side support
        translate([
            pot_body_width/2 + support_thickness/2,
            side_support_length/2,
            0
        ]) {
            square([support_thickness, side_support_length], center = true);
        }
        
        // Left side support (mirrored)
        translate([
            -pot_body_width/2 - support_thickness/2,
            side_support_length/2,
            0
        ]) {
            square([support_thickness, side_support_length], center = true);
        }
    }
}

module potentiometer_mounting_hole_2d() {
    translate([0, support_height/2 + support_height/4, 0]) {
        square([shaft_diameter, support_height/2], center = true);
    }
    
    translate([0, support_height/2, 0]) {
        circle(d = shaft_diameter);
    }
}

module potentiometer_mounting_hole_3d() {
    translate([0, support_thickness, 0]) {
        rotate([90, 0, 0]) {
            linear_extrude(height = support_thickness) {
                potentiometer_mounting_hole_2d();
            }
        }
    }
}

module potentiometer_holder_3d() {
    difference() {
        potentiometer_holder_without_mounting_hole_3d();
        potentiometer_mounting_hole_3d();
    }
}

// Uncomment to test:
// potentiometer_mounting_hole_2d();
// potentiometer_holder_3d();