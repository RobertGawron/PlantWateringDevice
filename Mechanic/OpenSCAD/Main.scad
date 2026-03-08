include <MountingBoard.scad>
include <PumpHolder.scad>
//include <Potentiometer.scad>
include <ZipTieBracket.scad>

$fn = smoothness;

// The pump doesn't need to be exactly at the center; it was slightly moved
// to give space for socket connector. This offset is important for pump
// holder and zip ties
pump_x_offset = 0;
pump_y_offset = 3;

module bottom_legs_3d() {
    // should slightly higher than pump holder to avoid that
    // the pump would touch surface on which the device stands.
    support_legs_height = mainboard_z 
                            + pump_thickness_z + 1.5;

    // should be bigger than the screw that goes into it.
    support_legs_diameter = 6;
    
    linear_extrude(height = support_legs_height)
    {
        mounting_holes_2d(support_legs_diameter);
    }
}

module top_legs_3d() {
    // this should be enough space, limiting factor are 
    // tantal caps and usb connector on the PCB. 
    // Note that there is a cutout for bottom sockets so
    // theirs height is not important here.
    top_support_legs_height = 4;

    // Should be bigger than the screw that goes into it.
    // Unlike bottom legs here we are limited b PCB components
    // clearance, if it will be to big, it will collide with
    // nearby PCB components.
    support_legs_diameter = 6;
    
    translate([0, 0, -top_support_legs_height]) 
    {
        linear_extrude(height = top_support_legs_height)
        {
            mounting_holes_2d(support_legs_diameter);
        }
    }
}

// bras inserts to mount the pcb
module top_legs_inserts_3d() {
    // this should be enough space, limiting factor are 
    // tantal caps and usb connector on the PCB. 
    // Note that there is a cutout for bottom sockets so
    // theirs height is not important here.
    support_legs_height = 7;

    // Should be bigger than the screw that goes into it.
    // Unlike bottom legs here we are limited b PCB components
    // clearance, if it will be to big, it will collide with
    // nearby PCB components.
    support_legs_diameter = 3.2;
    
    translate([0, 0, -support_legs_height]) 
    {
        linear_extrude(height = support_legs_height)
        {
            mounting_holes_2d(support_legs_diameter);
        }
    }
}

module zipties_3d()
{
        rotate ([0,0,90]) { 
            ziptie_height = mainboard_z 
                                + 2*pump_thickness_z;
        
            linear_extrude(height = ziptie_height) 
            {
                x = pump_y_offset;
                y = pcb_size_y/2 + pump_x_offset;
                z = -mainboard_z;
                
                translate([x, y, z]) 
                {
                    zipties_2d();
                }
            }
        }
    }

module model_without_zipties_3d() {
    union() {
        mounting_board_3d();
        bottom_legs_3d();
        top_legs_3d();
        
        // design with directions from first version
        // I will just rotate it instead of recreating it properly
        rotate ([0,0,90])
        {
            translate([ pump_y_offset, 
                        -pcb_size_y/2 + pump_x_offset, 
                        mainboard_z]) 
            {
                pump_rails_3d();
            }
        }
    }
}

module whole_model_3d() {

    difference() 
    {

    difference() 
    {
        // the lose so far
        model_without_zipties_3d();

        // remove material for zip ties
        zipties_3d();
        }

         top_legs_inserts_3d();
            }
}

whole_model_3d();