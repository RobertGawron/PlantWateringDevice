// ================= General =================================
smoothness = 50;        // Circle resolution
manufacturer_tolerance = 0.4; // Tolerance of 3D prints from printing factory

// =============== PCB related dimensions ====================

// 5cm square PCB
pcb_size = 50;           

// Useful for computation of offsets of holes and sockets that need
// to be aligned with the PCB
pcb_half_size = pcb_size / 2;  

// Hole for mounting screws that hold PCB
hole_diameter = 3.5;  

// Cut-out for display module needs to be aligned with
// potentiometer holder
display_module_width = 10;   

// ===================== MainBoard =========================== 
mainboard_thickness = 4;

// ================== Pump Holder =============================
// Pump holder constants used for holder and for holes for zipties
motor_w = 15.3 + 0.2 + manufacturer_tolerance;