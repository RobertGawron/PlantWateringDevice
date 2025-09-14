include <Constants.scad>

// All measurements come from the KiCad project of this device

// Oversized to ensure compatibility with hand-made boards
socket_cutout_x_start = 6.2;
socket_cutout_x_end = 13;
socket_cutout_y_start = 2.6 + pcb_half_size;
socket_cutout_y_end = 14.4 + pcb_half_size;
socket_cutout_width = socket_cutout_x_end - socket_cutout_x_start;
socket_cutout_height = socket_cutout_y_end - socket_cutout_y_start;

// These should match perfectly with no oversize
wire_cutout_x_start = 0;
wire_cutout_x_end = 4;
wire_cutout_y_start = 10;
wire_cutout_y_end = 18;
wire_cutout_width = wire_cutout_x_end - wire_cutout_x_start;
wire_cutout_height = wire_cutout_y_end - wire_cutout_y_start;

// Generic module to align cutout with PCB dimensions
module rectangle_connector_cutout_2d(x_start, y_start, width, height) {
    translate([x_start - pcb_half_size, y_start - pcb_half_size, 0])
        square([width, height]);
}

// Small PCB mounted on the back with two rectangular sockets for the connector 
// and two holes for mounting
module pcb_to_pcb_connector_cutout_2d() {
    rectangle_connector_cutout_2d(
        socket_cutout_x_start,
        socket_cutout_y_start,
        socket_cutout_width,
        socket_cutout_height
    );

    // Two holes to mount connector PCB; placement is not critical
    // as holes will be drilled manually on the PCB to fit the 3D printed part
    hole_x_offset = 3;
    hole_x = socket_cutout_x_start - pcb_half_size - hole_x_offset;
    lower_hole_y = socket_cutout_y_start - pcb_half_size + hole_diameter / 2;
    upper_hole_y = socket_cutout_y_start - pcb_half_size + socket_cutout_height - hole_diameter / 2;

    translate([hole_x, lower_hole_y, 0]) {
        circle(d = hole_diameter);
    }

    translate([hole_x, upper_hole_y, 0]) {
        circle(d = hole_diameter);
    }
}

module connector_cutout_2d() {
    union() {
        // Cutout at the edge of the PCB for pump wires
        rectangle_connector_cutout_2d(
            wire_cutout_x_start,
            wire_cutout_y_start,
            wire_cutout_width,
            wire_cutout_height
        );
            
        pcb_to_pcb_connector_cutout_2d();
    }
}