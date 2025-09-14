include <Constants.scad>

head_w = 21.0 + 0.3 + manufacturer_tolerance;

// Part thickness (Z)
pump_thickness_z = 12;  

 // Rail thickness in XY
rail_t = 3;            

 // Small horizontal tabs length, those tabs are needed to hold the pump in Y dimentions
tongue_len = 2.5;      

// Drawing-based dimensions (XY)
total_h = 38.8 + 0.5 + manufacturer_tolerance;

head_h = 14.1 + 0.3 + manufacturer_tolerance;
motor_h = total_h - head_h;

module side_rail_2d() {
    // Left side only; mirrored later for the right side
    union() {
        // Bottom horizontal tab (motor cable side)
        translate([-motor_w/2 - rail_t, 0])
            square([rail_t + tongue_len, rail_t]);

        // Top horizontal tab (pipe side)
        translate([-head_w/2 - rail_t, total_h + rail_t])
            square([rail_t + tongue_len, rail_t]);

        // Horizontal step at transition motor -> head
        translate([-head_w/2 - rail_t, motor_h])
            square([(head_w - motor_w)/2, rail_t]);

        // Vertical rail along motor
        translate([-motor_w/2 - rail_t, rail_t])
            square([rail_t, motor_h]);

        // Vertical rail along head
        translate([-head_w/2 - rail_t, motor_h + rail_t])
            square([rail_t, head_h]);
    }
}

module pump_rails_3d() {
    union() {
        // Left side
        linear_extrude(height = pump_thickness_z) {
            side_rail_2d();
        }
        
        // Right side - mirror in 3D
        mirror([1, 0, 0]) {
            linear_extrude(height = pump_thickness_z) {
                side_rail_2d();
            }
        }
    }
}