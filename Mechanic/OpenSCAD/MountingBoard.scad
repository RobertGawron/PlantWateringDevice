include <Constants.scad>

hole_distance = 3.5;       // from hole center to PCB edge

// PCB base shape (2D)
module pcb_base() 
{
    square([pcb_size_x, pcb_size_y], center = true);
}

// Single mounting hole (2D)
module mounting_hole_2d(x, y, hole_diameter) 
{
    translate([x, y, 0])
    {
        circle(d = hole_diameter);
    }
}

// All mounting holes (2D)
module mounting_holes_2d(hole_diameter) 
{
    // Calculate the absolute X and Y positions once
    dx = (pcb_size_x / 2) - hole_distance;
    dy = (pcb_size_y / 2) - hole_distance;

    // Loop through multipliers -1 (left/bottom) and 1 (right/top)
    for (x_mult = [-1, 1]) 
    {
        for (y_mult = [-1, 1]) 
        {
            x = x_mult * dx;
            y = y_mult * dy;

            mounting_hole_2d(x, y, hole_diameter);
        }
    }
}

module connector_slot_2d() 
{
    // bottom left pcb corner is the reference point
    x = -(pcb_size_x / 2);
    y  = -(pcb_size_y / 2);

    // taken from dimensions measurement on kicad pcb
    x_offset = 6.5;
    y_offset = 0;

    dx = pcb_size_x - (2 * x_offset);
    dy = 11.65;
    
    translate([x + x_offset, y + y_offset, 0])
    { 
        square([dx, dy]);
    }
}

// Main 2D mounting board
module mounting_board_2d() 
{
    difference() 
    {
        pcb_base();
        connector_slot_2d();
    }
}

// 3D extruded version
module mounting_board_3d() 
{
    linear_extrude(height = mainboard_z) 
    {
        mounting_board_2d();
    }
}
