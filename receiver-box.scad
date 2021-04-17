DEBUG = false;

disp_size = [32, 17.6, 3];
board_size = [51.5, 25.2, 1.2];

disp_on_board = [7.2, 3.8];

disp_support_x = [- 2, 1.2];

inner_box = [110, 90, 35];

switch_left_hole_dia = 10;
switch_hole_dia = 5.8;

cinch_hole_dia = 8.5;
cinch_panel_size = [50.35, 22.4];
cinch_hole_dist = cinch_panel_size.y - cinch_hole_dia;

relay_size = [42.5, 43.5, 19];

converter_thorn_dia = 4;
converter_thorn_dist = 45.6 + 2 * converter_thorn_dia / 2;

input_cable_hole_dia = 5;
siren_cable_hole_dia = 5;

// ---------------------------------------------

fatness = 1.2;
disp_border_fatness = 0.8;
disp_supports_fatness = 0.6;
inset = .2;

round_prec = 40;

// ---------------------------------------------

box = [inner_box.x + 2 * fatness, inner_box.y + 2 * fatness, inner_box.z + 2 * fatness];

disp_hole = [disp_size.x + 2 * inset, disp_size.y + 2 * inset, disp_size.z + .01];
disp_hole_pos = [box.x / 2 - disp_hole.x / 2 - disp_border_fatness + inset, 10 - disp_border_fatness + 2 * inset];

cinch_panel_pos = [box.x / 2 - cinch_panel_size.x / 2, inner_box.y - cinch_panel_size.y / 2];
disp_support_height = inner_box.z - board_size.z;

echo("Box size ", box.x, " x ", box.y, " x ", box.z);
echo("Inner box size ", inner_box.x, " x ", inner_box.y, " x ", inner_box.z);
echo("Display support height ", disp_support_height);
echo("Display hole ", disp_hole.x, " x ", disp_hole.y, " x ", disp_hole.z);

// ---------------------------------------------

module Board(x, y, z) {
    translate([x, y, z]) union() {
        color("gray") cube(board_size);
        color("black")translate([disp_on_board.x, board_size.y - disp_size.y - disp_on_board.y])  cube(disp_size);

        color("green") translate([disp_on_board.x + disp_support_x[0] - disp_supports_fatness, 0, - 3])
            cube([disp_supports_fatness, board_size.y, 3]);
        color("green") translate([disp_on_board.x + disp_size.x + disp_support_x[1], 0, - 3])
            cube([disp_supports_fatness, board_size.y, 3]);
    }
}

module Switch() {
    switch_size = [13, 13, 16];
    translate([- switch_size.x / 2, - switch_size.y / 2, fatness]) {
        color("black") cube(switch_size);
    }
}

module CinchPanel() {
    size_top = [55, 32, 14];
    size_bottom = [45, 24.5, 20];
    color("black") union() {
        translate([- (size_top.x - cinch_panel_size.x) / 2, - cinch_hole_dist / 2 - size_top.y / 2, inner_box.z - size_top.z + fatness])
            cube(size_top);
        translate([- (size_bottom.x - cinch_panel_size.x) / 2, - cinch_hole_dist / 2 - size_bottom.y / 2, fatness]) {
            cube(size_bottom);
            translate([- 5, size_bottom.y / 2 - 10 / 2]) cube([size_bottom.x + 10, 10, 2]);
        }
    }
}

module RelayModule() {
    translate([0, 0, 0]) {
        color("black") cube(relay_size);
    }
}

// bottom
difference() {
    union() {
        difference() {
            cube([box.x, box.y, box.z * .65]);
            translate([fatness + inset / 2, fatness + inset / 2, fatness]) {
                cube([inner_box.x, inner_box.y, box.z]);
            }

            if (DEBUG) {
                translate([- 1, 1, fatness]) cube([box.x + 2, box.y - 2, 50]);
                translate([1, - 1, fatness]) cube([box.x - 2, box.y + 2, 50]);
            }
        }

        // display supports
        disp_support_width_overlap = 2;
        disp_support_width = board_size.y + 2 * disp_support_width_overlap;
        translate([disp_hole_pos.x + 2 * inset, disp_hole_pos.y / 2]) {
            if (DEBUG) {
                Board(- disp_on_board.x + disp_supports_fatness, disp_support_width_overlap, disp_support_height - board_size.z);
            }

            off = 23;
            translate([disp_support_x[0], 0, off]) {
                difference() {
                    cube([disp_supports_fatness, disp_support_width, disp_support_height - off]);
                    translate([0, disp_support_width_overlap, disp_support_height - off - board_size.z])
                        cube([disp_supports_fatness, board_size.y, board_size.z]);
                }
            }

            translate([disp_support_x[1] + disp_size.x + disp_supports_fatness, 0]) {
                difference() {
                    cube([disp_supports_fatness, disp_support_width, disp_support_height]);
                    translate([0, disp_support_width_overlap, disp_support_height - board_size.z])
                        cube([disp_supports_fatness, board_size.y, board_size.z]);
                }
            }
        }

        // relay house
        difference() {
            size_inner = [relay_size.x + 2, relay_size.y, relay_size.z + 2];
            size_outer = [size_inner.x + 2 * fatness, size_inner.y + 2 * fatness, size_inner.z + 2 * fatness];
            color("orange") cube(size_outer);
            color("blue") translate([fatness, fatness + .01, fatness]) cube(size_inner);
            color("blue") translate([fatness, size_outer.y - 11, fatness]) cube([size_inner.x, 20, 50]);
        }

        // converter thorns
        translate([cinch_panel_pos.x, cinch_panel_pos.y, fatness]) {
            translate([(cinch_panel_size.x - converter_thorn_dist) / 2, - cinch_hole_dist / 2]) {
                for (i = [0 : 1]) {
                    translate([i * converter_thorn_dist, 0])
                        rotate([0, 0, 90]) color("blue") cylinder(h = 2.5, d = converter_thorn_dia, $fn = round_prec);
                }
            }
        }

        if (DEBUG) {
            translate(cinch_panel_pos) CinchPanel();
            translate([fatness + 1, fatness + 1, fatness]) RelayModule();
        }

        color("green") translate([fatness / 2 + .2, fatness / 2 + .2, box.z * .65 - .1]) {
            difference() {
                echo("Closing inner", box.x - fatness - .4, box.y - fatness - .4);
                cube([box.x - fatness - .4, box.y - fatness - .4, 5]);
                translate([fatness / 2 - .1, fatness / 2 - .1, - .1]) {
                    cube([inner_box.x - .2, inner_box.y - .2, inner_box.z]);
                }

                if (DEBUG) {
                    translate([- 10, 10, - 10]) cube([box.x + 20, box.y - 20, 50]);
                    translate([10, - 10, - 10]) cube([box.x - 20, box.y + 20, 50]);
                }
            }
        }
    }

    // hole for siren cable
    translate([- 0.01, box.y * .75, 10]) rotate([0, 90]) {
        cylinder(h = fatness + 1, d = siren_cable_hole_dia, $fn = round_prec);
    }

    // hole for INPUT cable
    translate([box.x - fatness - .01, box.y * .75, 10]) rotate([0, 90]) {
        cylinder(h = fatness + 1, d = input_cable_hole_dia, $fn = round_prec);
    }

}


// cover
translate([0, - (inner_box.y + 15), 0])
//    rotate([180, 0, 180]) translate([- box.x, inner_box.y + 15, - box.z])
    difference() {
        switch_left_pos = [(box.x - disp_size.x) / 4, 10 + disp_border_fatness + disp_size.y / 2, - .01];
        switch_pos = [(box.x - disp_size.x) / 4, 10 + disp_border_fatness + disp_size.y / 2, - .01];

        union() {
            difference() {
                cube([box.x, box.y, box.z * .35]);
                translate([fatness, fatness, fatness]) {
                    cube([inner_box.x, inner_box.y, inner_box.z]);
                }

                if (DEBUG) {
                    translate([- 1, 1, fatness]) cube([box.x + 2, box.y - 2, 50]);
                    translate([1, - 1, fatness]) cube([box.x - 2, box.y + 2, 50]);
                }

                color("green") translate([fatness / 2 - inset / 2, fatness / 2 - inset / 2, box.z * .35 - 5]) {
                    echo("Closing outer", box.x - fatness + inset, box.y - fatness + inset);
                    cube([box.x - fatness + inset, box.y - fatness + inset, 10]);
                }
            }

            translate(disp_hole_pos) color("red") {
                cube([disp_hole.x + 2 * disp_border_fatness, disp_hole.y + 2 * disp_border_fatness, disp_size.z]);
            }

            if (DEBUG) {
                translate(switch_pos) Switch();
            }
        }

        // hole for display
        translate(disp_hole_pos) {
            color("blue") translate([disp_border_fatness, disp_border_fatness, - .01])
                cube([disp_hole.x, disp_hole.y, disp_hole.z + .02]);
        }

        // holes for cinch panel
        color("red") translate(cinch_panel_pos) {
            for (col = [0 : 3]) {
                for (row = [0 : 1]) {
                    translate([cinch_hole_dia / 2 + col * cinch_hole_dist, - row * cinch_hole_dist, - .01]) {
                        rotate([0, 0, 90]) cylinder(h = fatness + .02, d = cinch_hole_dia + .3, $fn = round_prec);
                    }
                }
            }
        }

        // hole for switch
        translate(switch_pos) {
            color("blue") rotate([0, 0, 90]) cylinder(h = fatness + .02, d = switch_hole_dia, $fn = round_prec);
        }
    }
