include <Constants.scad>

// Cutout for PCB to make space for electronic components on display module
display_module_length = 22.75;    // Length (longer side)

// Mounting hole parameters from product description
mounting_hole_diameter = 2 + manufacturer_tolerance;      // 2mm post diameter
mounting_hole_distance = 1.75;   // 1.75mm from border to post center

// Base rectangle
module base_rectangle() {
    square([display_module_length, display_module_width], center = true);
}

// Single mounting post
module mounting_post(x, y) {
    translate([x, y, 0]) 
        circle(d = mounting_hole_diameter);
}

// Both posts on shorter sides
module mounting_posts() {
    // Posts on shorter sides, positioned at mounting_hole_distance from edge
    x_position = display_module_length / 2 + mounting_hole_distance;
    
    // Left side post
    mounting_post(-x_position, 0);
    
    // Right side post
    mounting_post(x_position, 0);
}

module display_holder_2d() {
    // Position vertically with left side at x=0 for easier placement
    translate([-display_module_width/2, 0, 0]) {
        rotate([0, 0, 90]) {
            union() {
                base_rectangle();
                mounting_posts();
            }
        }
    }
}