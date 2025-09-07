include <Constants.scad>

// ---------------- Parameters ----------------
pump_thickness_z = 12; // part thickness (Z)
rail_t           = 3;  // rail thickness in XY
tongue_len       = 2.5; // small horizontal tabs length

// Drawing-based dimensions (XY)
total_h = 38.8 + 0.5 + manufacturer_tolerance;

head_w  = 21.0 + 0.3 + manufacturer_tolerance;
head_h  = 14.1 + 0.3 + manufacturer_tolerance;

motor_w = 15.3 + 0.2 + manufacturer_tolerance;
motor_h = total_h - head_h;

// ---------------- 2D geometry ----------------
module side_rail_2d() {
  // Left side only; mirrored later for the right side
  union() {
    // bottom horizontal tab (motor cable side)
    translate([-motor_w/2 - rail_t, 0])
      square([rail_t + tongue_len, rail_t]);

    // top horizontal tab (pipe side)
    translate([-head_w/2- rail_t, total_h + rail_t])
      square([rail_t + tongue_len, rail_t]);

    // horizontal step at transition motor -> head
    translate([-head_w/2- rail_t, motor_h])
      square([(head_w - motor_w)/2, rail_t]);

    // vertical rail along motor
    translate([-motor_w/2 - rail_t, rail_t])
      square([rail_t, motor_h]);

    // vertical rail along head
    translate([-head_w/2- rail_t, motor_h + rail_t])
      square([rail_t, head_h]);
  }
}

module rails_2d() {
  union() {
    side_rail_2d();                 // left
    mirror([1,0,0]) side_rail_2d(); // right
  }
}


module pump_rails_3d ()
{
  linear_extrude(height = pump_thickness_z)
    rails_2d();

}
// ---------------- 3D extrusion ----------------
//module holder_3d() {
//  linear_extrude(height = pump_thickness_z)
//    rails_2d();
//}
 //rails_2d();
//holder_3d();