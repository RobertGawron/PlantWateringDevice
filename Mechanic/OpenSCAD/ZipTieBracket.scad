include <Constants.scad>

ziptie_hole_width = 3.2;
ziptie_hole_height = 4;

// Arbitrary values; ensure they are vertically aligned with pump holder
// when the whole device is built
ziptie_1_y = 10 - pcb_size_x;
ziptie_2_y = ziptie_1_y + 10;

module ziptie() {
    square([ziptie_hole_width, ziptie_hole_height]);
}

module zipties_2d() {
    x_right = -motor_w/2 - ziptie_hole_width;
    x_left = motor_w/2;
  
    translate([x_right, ziptie_1_y, 0]) {
        ziptie();
    }
    
    translate([x_right, ziptie_2_y, 0]) {
        ziptie();
    }      

    translate([x_left, ziptie_1_y, 0]) {
        ziptie();
    }
    
    translate([x_left, ziptie_2_y, 0]) {
        ziptie();
    }      
}
