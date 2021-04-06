magnifier_d = [50.5, 40];
magnifier_f = 29.5;
magnifier_ring = 0.5;

photodiode_d = 5.1;
control_diode_d = 4.7;

connector_hole_dia = 4;

screwholder_size = [10, 15, 4];
screw_hole_dia = 3.5;

fatness = 1.4;
inset = .3;

round_prec = 40;

inner_box = [50, magnifier_d[0] + inset, magnifier_d[0] + inset];
box = [inner_box.x + 2 * fatness, inner_box.y + 2 * fatness, inner_box.z + 2 * fatness];


echo("Box size ", box.x, " x ", box.y, " x ", box.z);
echo("Inner box size ", inner_box.x, " x ", inner_box.y, " x ", inner_box.z);

// ---------------------------------------------

module ScrewHolder(x, y) {
    translate([x, y - screwholder_size.y / 2]) difference() {
        cube([screwholder_size.x, screwholder_size.y, screwholder_size.z]);
        translate([screwholder_size.x / 2, screwholder_size.y / 2]) rotate([0, 0, 90]) {
            cylinder(h = screwholder_size.z, d = screw_hole_dia, $fn = round_prec);
            translate([0, 0, screwholder_size.z - 1]) color("yellow") cylinder(h = 1, d = screw_hole_dia + 2, $fn = round_prec);
        }
    }
}

// magnifier holder
union() {
    difference() {
        cube([box.x, box.y, box.z / 2]);
        translate([fatness, fatness, fatness]) {
            cube([inner_box.x, inner_box.y, inner_box.z]);
        }

        h = (fatness - magnifier_ring) / 2;
        center_z = magnifier_d[0] / 2 + fatness;
        dia = magnifier_d[1] + 2;

        translate([box.x - h, box.y / 2, center_z]) rotate([0, 90]) {
            color("red") cylinder(h = 1, d = dia, $fn = round_prec);
        }
        translate([box.x - fatness - inset / 2 - 1, box.y / 2, center_z]) rotate([0, 90]) {
            color("red") cylinder(h = 1 + h, d = dia, $fn = round_prec);
        }
        translate([box.x - fatness, box.y / 2, center_z]) rotate([0, 90]) {
            color("blue") cylinder(h = fatness, d = dia - 1.6, $fn = round_prec);
        }

        color("green") translate([fatness / 2 - .05, fatness / 2 - .05, box.z / 2 - 10]) {
            cube([box.x - fatness + .05, box.y - fatness + .05, 10]);
        }

        // hole for control LED
        translate([box.x - fatness, fatness + 5, fatness + 5]) rotate([0, 90]) {
            cylinder(h = fatness, d = 5.5, $fn = round_prec);
        }

        // hole for wire
        translate([0, box.y / 2, fatness + connector_hole_dia / 2]) rotate([0, 90]) {
            cylinder(h = fatness, d = connector_hole_dia, $fn = round_prec);
        }
    }

    led_column_width = 8;

    // actual diode holder
    translate([box.x - fatness - magnifier_f - 2, box.y / 2 - led_column_width / 2, 0]) {
        color("yellow")
            difference() {
                union() {
                    cube([2, led_column_width, magnifier_d[0] / 2 + 4 + fatness]);
                    translate([- 3, led_column_width / 2 - 1]) cube([8, 2, 6]);
                }
                translate([- .1, led_column_width / 2, magnifier_d[0] / 2]) rotate([0, 90]) {
                    color("red") cylinder(h = 2 + 0.2, d = control_diode_d, $fn = round_prec);
                }
            }
    }

    ScrewHolder(box.x / 2 - screwholder_size.x / 2, - screwholder_size.y / 2 + .01);
    ScrewHolder(box.x / 2 - screwholder_size.x / 2, box.y + screwholder_size.y / 2 - .01);
}


// cover
translate([0, -75, 0])
    difference() {
        union() {
            difference() {
                cube([box.x, box.y, box.z / 2]);
                translate([fatness, fatness, fatness]) {
                    cube([inner_box.x, inner_box.y, inner_box.z]);
                }

                h = (fatness - magnifier_ring) / 2;
                center_z = magnifier_d[0] / 2 + fatness;
                dia = magnifier_d[1] + 2;

                translate([box.x - h, box.y / 2, center_z]) rotate([0, 90]) {
                    color("red") cylinder(h = 1, d = dia, $fn = round_prec);
                }
                translate([box.x - fatness - inset / 2 - 1, box.y / 2, center_z]) rotate([0, 90]) {
                    color("red") cylinder(h = 1 + h, d = dia, $fn = round_prec);
                }
                translate([box.x - fatness, box.y / 2, center_z]) rotate([0, 90]) {
                    color("blue") cylinder(h = fatness, d = dia - 1.6, $fn = round_prec);
                }
            }

            color("green") translate([fatness / 2 + .1, fatness / 2 + .1, box.z / 2 - .1]) {
                difference() {
                    cube([box.x - fatness - .2, box.y - fatness - .2, 10]);
                    translate([fatness / 2 - .1, fatness / 2 - .1, - .1]) {
                        cube([inner_box.x, inner_box.y, inner_box.z]);
                    }
                }
            }
        }

        // cut the hole for magnifier
        color("red") translate([box.x - fatness - .2, box.y / 2 - magnifier_d[1] / 2 - 2, box.z / 2 - .1]) {
            cube([fatness, magnifier_d[1] + 4, 10 + .1]);
        }
    }
