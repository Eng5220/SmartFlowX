// ============================================================
// AquaFlow — APDS-9960 Gesture Sensor Wall-Mount Bracket
// ============================================================
// The APDS-9960 board is 15.5mm wide x 20.5mm tall.
// This bracket clips it flush against the back wall of the
// dispenser, facing outward into the cup bay.
//
// HOW TO USE:
//   Press F5 to preview. Press F6 to fully render.
//   File → Export → Export as STL when done.
// ============================================================


// --- Board Dimensions (do not change) ---
board_w   = 15.5;   // APDS board width  (mm)
board_h   = 20.5;   // APDS board height (mm)
board_thk = 1.6;    // standard PCB thickness

// --- Bracket Parameters (adjust to taste) ---
wall_thk   = 2.5;   // bracket wall thickness
lip_depth  = 2.0;   // how far the lip grips the board edge
lip_height = 3.0;   // height of the retaining lips

// --- Mounting screw holes (M3) ---
screw_d    = 3.2;   // M3 clearance hole
screw_head = 6.0;   // M3 countersink diameter
screw_off  = 4.0;   // distance from corner to screw centre

// --- Derived outer dimensions ---
outer_w = board_w + (wall_thk * 2);
outer_h = board_h + (wall_thk * 2);

// ============================================================
// Main bracket body
// ============================================================
difference() {
    union() {
        // Base plate (the part that screws to the back wall)
        cube([outer_w, wall_thk, outer_h]);

        // Left retaining lip (grips left edge of PCB)
        translate([0, wall_thk, wall_thk])
            cube([wall_thk, lip_depth, board_h]);

        // Right retaining lip (grips right edge of PCB)
        translate([outer_w - wall_thk, wall_thk, wall_thk])
            cube([wall_thk, lip_depth, board_h]);

        // Bottom retaining lip (keeps board from sliding down)
        translate([0, wall_thk, 0])
            cube([outer_w, lip_depth, wall_thk]);

        // Top retaining lip (locks board in completely)
        translate([0, wall_thk, outer_h - wall_thk])
            cube([outer_w, lip_depth, wall_thk]);
    }

    // --- Sensor window: hollow cutout so the sensor faces forward ---
    // Slightly smaller than the board so the sensor chip peeks through
    translate([wall_thk + 2, -1, wall_thk + 2])
        cube([board_w - 4, wall_thk + 2, board_h - 4]);

    // --- Mounting screw holes in the base plate ---
    // Bottom-left
    translate([screw_off, wall_thk / 2, screw_off])
        rotate([90, 0, 0]) cylinder(d = screw_d, h = wall_thk + 1, $fn = 30);

    // Bottom-right
    translate([outer_w - screw_off, wall_thk / 2, screw_off])
        rotate([90, 0, 0]) cylinder(d = screw_d, h = wall_thk + 1, $fn = 30);

    // Top-left
    translate([screw_off, wall_thk / 2, outer_h - screw_off])
        rotate([90, 0, 0]) cylinder(d = screw_d, h = wall_thk + 1, $fn = 30);

    // Top-right
    translate([outer_w - screw_off, wall_thk / 2, outer_h - screw_off])
        rotate([90, 0, 0]) cylinder(d = screw_d, h = wall_thk + 1, $fn = 30);
}
