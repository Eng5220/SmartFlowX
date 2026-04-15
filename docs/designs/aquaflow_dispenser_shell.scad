// ============================================================
//  AquaFlow — Full Dispenser Shell  v1.0
// ============================================================
//
//  Build footprint:  220 × 200 × 250 mm  (W × D × H)
//  Bambu P1S limit:  256 × 256 × 256 mm  ✓  FITS
//
//  LAYOUT (top-view, Y axis = front-to-back)
//  ┌──────────────────────────────┐ ← rear wall (Y = 200)
//  │   Electronics Bay  (73 mm)  │   Pi, wiring, flow sensor
//  ├──────────────────────────────┤ ← partition wall (Y = 120)
//  │      Cup Bay  (120 mm)      │   pump tube drops in from top
//  └──────────────────────────────┘ ← front face (Y = 0)
//    open to user → cup slides in
//
//  FRONT FACE (Z axis = bottom-to-top)
//  ┌──────────────────┐  ← top wall
//  │    LCD window    │  at Z ≈ 220 mm
//  │                  │
//  │                  │
//  │   CUP BAY OPEN   │  Z = 3.5 → 203.5 mm (200 mm tall)
//  │                  │
//  └──────────────────┘  ← floor
//
//  HOW TO USE:
//    F5  = fast preview (orange, good for layout checks)
//    F6  = full render  (grey, export-ready)
//    File → Export → Export as STL
//
//  PRINT SETTINGS (recommended for James Watt South Bambu P1S):
//    Material:      PLA
//    Layer height:  0.2 mm
//    Infill:        20–25 % gyroid
//    Walls:         3 perimeters
//    Supports:      REQUIRED — cup bay leaves a large overhang
//    Estimated time: 16–22 h
//    Estimated mass: ~500 g
// ============================================================


// ============================================================
//  PARAMETERS  ← edit these to tune the build
// ============================================================

/* [Shell] */
body_w  = 220;      // outer width  (mm)
body_d  = 200;      // outer depth, front‑to‑back (mm)
body_h  = 250;      // outer height (mm)
wall    = 3.5;      // shell wall thickness (mm)

/* [Cup Bay — opened from front face] */
// Standard large McDonald's / KFC cup: ~107 mm top dia, ~180 mm tall
bay_w   = 150;      // bay width  (cups to ~115 mm dia with clearance)
bay_d   = 120;      // bay depth, front face → partition wall
bay_h   = 205;      // bay height (5 mm clearance over a 200 mm cup)
// NOTE: bay opens fully at front face and from the floor up to bay_h

/* [Nozzle / Tube Port] */
// 6 mm silicone tube, 1 mm wall clearance each side → 8 mm hole
tube_od = 8;
// Port is centred in X over the cup bay, emerges through the top wall
// The water tube routes from the electronics bay, up and over the partition

/* [LCD Window — 1602‑style, 16 × 2 chars] */
// Visible display area of a standard 1602 LCD: ~72 mm × 25 mm
lcd_w        = 74;  // window width  (add 2 mm tolerance)
lcd_h        = 27;  // window height (add 2 mm tolerance)
lcd_gap_above_bay = 12; // solid wall between top of cup bay and LCD bottom

/* [APDS‑9960 Gesture Sensor — board 15.5 × 20.5 mm] */
// Mounted flush in partition wall, facing the cup bay (toward front)
sns_brd_w   = 15.5; // actual board width
sns_brd_h   = 20.5; // actual board height
sns_tol     = 1.5;  // press‑fit tolerance each side
sns_pocket_w = sns_brd_w + sns_tol * 2; // = 18.5 mm
sns_pocket_h = sns_brd_h + sns_tol * 2; // = 23.5 mm
sns_pocket_d = 2.5; // pocket depth (PCB sits 2.5 mm into wall, chip faces forward)
sns_window   = 9;   // square through‑hole for sensor chip line‑of‑sight
// Sensor sits at 35 % of bay height — good cup‑detection height
sns_z_pct    = 0.35;

/* [YF‑S401 Flow Sensor mounting channel — 58 × 35 × 27 mm body] */
// The flow sensor sits inline in the water tube inside the electronics bay.
// A clip rail is printed on the electronics bay floor to locate it.
fs_w = 60;  // rail slot width  (sensor body 58 mm + 2 mm clearance)
fs_h = 10;  // clip rail height

/* [Raspberry Pi — standard mounting hole pattern Pi 2/3/4] */
// Hole‑to‑hole distances: 58 mm (X) × 49 mm (Y)
pi_hole_d  = 2.7;   // M2.5 clearance diameter
pi_x_span  = 58;    // mounting hole X span
pi_y_span  = 49;    // mounting hole Y span
pi_post_d  = 7;     // pillar outer diameter
pi_post_h  = 6;     // pillar height (PCB standoff)
// Pi centred in electronics bay width; 12 mm behind partition wall
// Electronics bay: Y = (bay_d + wall)  to  Y = (body_d − wall)
//   depth = 200 − 3.5 − 123.5 = 73 mm  → Pi Y‑span 49 mm + 12 + some margin ✓

/* [Rear Inlet — external reservoir/pump tube entry] */
// The JT80SL pump sits submerged in an external water bottle/reservoir.
// The outlet tube enters the rear wall of the dispenser here.
inlet_d = 10;   // tube entry hole (6 mm tube, generous clearance)
inlet_z = 25;   // height from base floor (near bottom, below electronics)

/* [Rear Access Panel] */
// A large opening in the back wall for wiring, Pi access, flow sensor, etc.
// A matching cut‑out cover plate can be printed separately.
access_border = 18; // solid border kept around the access panel

/* [Rubber Feet] */
foot_d   = 14;  // diameter
foot_h   = 7;   // height (elevates shell, protects countertop)
foot_off = 18;  // inset from corner


// ============================================================
//  DERIVED VALUES  (do not edit)
// ============================================================

inner_w = body_w - wall * 2;
inner_d = body_d - wall * 2;

// Cup bay left/right edges (centred in X)
bay_x0  = (body_w - bay_w) / 2;

// LCD window position
lcd_z0  = wall + bay_h + lcd_gap_above_bay;
lcd_x0  = (body_w - lcd_w) / 2;

// Sensor position on partition wall
sns_z0  = wall + bay_h * sns_z_pct;
sns_x0  = (body_w - sns_pocket_w) / 2;

// Pi mounting posts — centred in width, 12 mm behind partition
pi_x0   = (body_w - pi_x_span) / 2;
pi_y0   = bay_d + wall + 12;

// Flow sensor clip rail: centred in width, on electronics bay floor
fs_x0   = (body_w - fs_w) / 2;
fs_y0   = bay_d + wall + 5;


// ============================================================
//  MODULES
// ============================================================

// --- 1. Outer shell: all four walls + floor, open top -------------
module shell() {
    difference() {
        cube([body_w, body_d, body_h]);
        // Hollow interior
        translate([wall, wall, wall])
            cube([inner_w, inner_d, body_h]);
    }
}

// --- 2. Partition wall: separates cup bay from electronics bay ----
module partition_wall() {
    translate([wall, bay_d, wall])
        cube([inner_w, wall, body_h - 2 * wall]);
}

// --- 3. Wire/tube pass-through at the top of the partition --------
//   16 mm wide × 35 mm tall slot so the water tube and signal
//   wires can cross from the electronics bay into the cup bay.
module partition_passthrough() {
    translate([body_w / 2 - 8, bay_d - 0.5, wall + bay_h - 35])
        cube([16, wall + 1, 35]);
}

// --- 4. Cup bay void: hollows out the front section ---------------
module cup_bay_void() {
    // Removes: front wall (Y 0→bay_d) and interior bay space
    // Leaves floor (Z starts at wall), leaves side walls of bay
    translate([bay_x0, 0, wall])
        cube([bay_w, bay_d, bay_h]);
}

// --- 5. LCD window: through‑hole in front wall, above cup bay -----
module lcd_window() {
    translate([lcd_x0, -1, lcd_z0])
        cube([lcd_w, wall + 2, lcd_h]);
}

// --- 6. Nozzle port: tube exits top of shell above cup bay --------
//   Water falls from this port down into the waiting cup.
module nozzle_port() {
    translate([body_w / 2, bay_d / 2, body_h - 0.5])
        cylinder(d = tube_od, h = wall + 1, $fn = 30);
}

// --- 7. Sensor pocket in partition wall (PCB press‑fit) -----------
module sensor_recess() {
    // a) PCB pocket: board is pressed from the cup‑bay side
    translate([sns_x0, bay_d - sns_pocket_d, sns_z0])
        cube([sns_pocket_w, sns_pocket_d + 0.5, sns_pocket_h]);

    // b) Sensor through‑window: centred in the pocket, passes fully
    //    through the partition wall so the sensor chip can see the cup
    translate([
        body_w / 2 - sns_window / 2,
        bay_d - wall - 0.5,
        sns_z0 + (sns_pocket_h - sns_window) / 2
    ])
        cube([sns_window, wall + 1, sns_window]);
}

// --- 8. Rear access panel ----------------------------------------
//   Large opening in back wall. Keep access_border of solid wall.
module rear_access() {
    translate([
        access_border,
        body_d - 0.5,
        access_border
    ])
        cube([
            body_w - access_border * 2,
            wall + 1,
            body_h - access_border * 2
        ]);
}

// --- 9. Rear tube inlet: external reservoir hose entry hole -------
module rear_tube_inlet() {
    translate([body_w / 2, body_d - 0.5, inlet_z])
        rotate([90, 0, 0])
            cylinder(d = inlet_d, h = wall + 1, $fn = 30);
}

// --- 10. Raspberry Pi mounting post (one pillar) ------------------
module pi_post(x, y) {
    translate([x, y, wall]) {
        difference() {
            cylinder(d = pi_post_d, h = pi_post_h, $fn = 20);
            // Blind M2.5 screw hole at top (3 mm deep)
            translate([0, 0, pi_post_h - 3])
                cylinder(d = pi_hole_d, h = 3.5, $fn = 20);
        }
    }
}

// --- 11. Flow sensor clip rail (holds YF‑S401 inline in tube) -----
//   Two small walls form a slot on the electronics bay floor.
module flow_sensor_rail() {
    translate([fs_x0, fs_y0, wall]) {
        // Left clip
        cube([2.5, 30, fs_h]);
        // Right clip
        translate([fs_w - 2.5, 0, 0])
            cube([2.5, 30, fs_h]);
    }
}

// --- 12. Rubber foot (extends below base plate) -------------------
module foot(x, y) {
    translate([x, y, -foot_h])
        cylinder(d = foot_d, h = foot_h, $fn = 32);
}


// ============================================================
//  ASSEMBLY
// ============================================================

difference() {
    union() {
        shell();                            // outer shell with walls + floor

        partition_wall();                   // dry/wet divider

        // --- Raspberry Pi mounting posts (electronics bay) ---
        pi_post(pi_x0,              pi_y0);
        pi_post(pi_x0 + pi_x_span,  pi_y0);
        pi_post(pi_x0,              pi_y0 + pi_y_span);
        pi_post(pi_x0 + pi_x_span,  pi_y0 + pi_y_span);

        // --- Flow sensor clip rail (electronics bay floor) ---
        flow_sensor_rail();
    }

    // ---- Subtractions (all the holes and voids) ----

    cup_bay_void();             // 1. hollow cup bay from front face
    lcd_window();               // 2. LCD window above cup bay
    nozzle_port();              // 3. tube port in top wall
    sensor_recess();            // 4. APDS‑9960 pocket + through‑window
    partition_passthrough();    // 5. tube/wire slot at top of partition
    rear_access();              // 6. electronics access panel in rear wall
    rear_tube_inlet();          // 7. external reservoir tube entry (rear, low)
}

// --- Feet: added outside difference() so they aren't subtracted ---
foot(foot_off,           foot_off);
foot(body_w - foot_off,  foot_off);
foot(foot_off,           body_d - foot_off);
foot(body_w - foot_off,  body_d - foot_off);
