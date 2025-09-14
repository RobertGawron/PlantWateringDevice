include <Constants.scad>

clearance_extra = 3;
pot_body_width = 17; 
pot_body_height = pot_body_width/2 + 13;
pot_thickness_no_shaft = 9.2;

support_thickness = 3;
support_height = pot_body_width + 1;
side_support_length = pot_thickness_no_shaft;

shaft_diameter = 7.2 + manufacturer_tolerance;

module potentiometer_holder_3d() {
    difference() {
        // Main body
        union() {
            // Main horizontal strip
            translate([0, support_thickness/2, support_height/2]) {
                cube([pot_body_width, support_thickness, support_height], center = true);
            }
            
            // Right side support
            translate([pot_body_width/2 + support_thickness/2, side_support_length/2, support_height/2]) {
                cube([support_thickness, side_support_length, support_height], center = true);
            }
            
            // Left side support
            translate([-(pot_body_width/2 + support_thickness/2), side_support_length/2, support_height/2]) {
                cube([support_thickness, side_support_length, support_height], center = true);
            }
        }
        
        // Shaft hole with open top - combining cylinder and rectangular slot
        translate([0, support_thickness/2, 0]) {
            rotate([90, 0, 0]) {
                linear_extrude(height = support_thickness + 0.2, center = true) {
                    union() {
                        // Circular hole for shaft
                        translate([0, support_height/2, 0]) {
                            circle(d = shaft_diameter);
                        }
                        
                        // Rectangular slot from top to allow insertion
                        translate([0, support_height/2 + support_height/4, 0]) {
                            square([shaft_diameter, support_height/2], center = true);
                        }
                    }
                }
            }
        }
    }
}