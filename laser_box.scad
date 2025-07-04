DEBUG = false;

laser_dia = 12;
main_length = 35.8;

heatsink_size_v1 = [31.7, 20, 30.2];
laser_z_in_heatsink_v1 = 10.7;

heatsink_size_v2 = [50.5, 20.2, 31.5];
laser_z_in_heatsink_v2 = 11;


heatsink_size = heatsink_size_v2;
fan_size = [7.8, 30, 30];

fan_hole_pos = [3, 3];
fan_hole_dia = 4.5;

laser_length = 42;
laser_board_length = 10.4;
laser_z_in_heatsink = laser_z_in_heatsink_v2;
laser_pos_offset = abs(laser_length - heatsink_size.x - 10);
echo("laser_pos_offset", laser_pos_offset);
length_space = 5 + laser_pos_offset;
ray_hole_laser_dia = 5;
screw_hole_laser_dia = 3.5;
cable_hole_width = 5.5;

screwholder_size = [20, 8, 3];

partition_fatness = .8;
fatness = 1.2;
cover_side_fat = 0.8;
inset = 0.4;
round_prec = 20;

// ---

inner_height = max(fan_size.z, heatsink_size.z);
//inner_height = fan_size.z;

echo("Inner height ", inner_height);

base_inner = [laser_length + laser_board_length + length_space + fan_size.x + inset, fan_size.y + inset, fan_size.z + inset];
echo("Base inner box size ", base_inner.x, " x ", base_inner.y);

base_outer = [base_inner.x + cover_side_fat * 2, base_inner.y + cover_side_fat * 2, inner_height / 2];
echo("Base outer box size ", base_outer.x, " x ", base_outer.y);

inner_box = [base_outer.x + inset / 2, base_outer.y + inset / 2, inner_height];
echo("Inner box size ", inner_box.x, " x ", inner_box.y);

box = [inner_box.x + fatness * 2, inner_box.y + fatness * 2, inner_height + fatness];
echo("Box size ", box.x, " x ", box.y, " x ", box.z);


// ---------------------------------------------

module Heatsink(x, y, z) {
    translate([x, y, z]) union() {
        color("black") cube(heatsink_size);
    }
}

module Laser(x, y, z) {
    translate([x, y, z]) union() {
        color("gray") rotate([0, 90]) cylinder(h = laser_length, d = laser_dia, $fn = 40);
        color("gray") translate([laser_length, - laser_dia / 2, - 1]) cube([laser_board_length, laser_dia, 1]);
        color("green") translate([- 30, 0, 0]) rotate([0, 90]) cylinder(h = laser_length, d = ray_hole_laser_dia, $fn = 40);
    }
}

module Fan(x, y, z) {
    color("maroon") translate([x, y, z]) union() {
        difference() {
            cube(fan_size);
            for (r = [0:1])for (c = [0:1]) {
                translate([- .1,
                        fan_hole_pos.x + c * (fan_size.y - 2 * fan_hole_pos.x),
                        fan_hole_pos.y + r * (fan_size.z - 2 * fan_hole_pos.y)]
                ) color("yellow")  rotate([0, 90]) cylinder(h = fan_size.x + 10 + .01, d = fan_hole_dia, $fn = round_prec);
            }
        }
    }
}

module ScrewHolder(x, y) {
    color("orange") translate([x, y - screwholder_size.y / 2]) difference() {
        cube([screwholder_size.x, screwholder_size.y, screwholder_size.z]);
        translate([screwholder_size.x / 2, screwholder_size.y / 2, - .01]) rotate([0, 0, 90]) {
            cylinder(h = screwholder_size.z + 2, d = screw_hole_laser_dia, $fn = round_prec);
            translate([0, 0, screwholder_size.z - 1]) color("yellow") cylinder(h = 2, d = screw_hole_laser_dia + 2, $fn = round_prec);
        }
    }
}

// cover
//translate([0, - 15.4, fatness + box.z + .05]) rotate([180, 0, 0])
union() {
    difference() {
        cube(box);

        translate([fatness, fatness, fatness + .01])
            cube(inner_box);

        // for debugging (ceiling)
        // translate([fatness, fatness, - .01]) cube(inner_box);

        // ray hole
        translate([- 0.2, box.y / 2, box.z - laser_dia / 2 - laser_z_in_heatsink]) rotate([0, 90]) {
            cylinder(h = fatness + 0.4, d = ray_hole_laser_dia, $fn = round_prec);
        }

        // cable hole
        translate([box.x - fatness - .01, box.y - cable_hole_width - fatness - cover_side_fat - inset / 2, box.z - 6]) {
            cube([4, cable_hole_width, 100]);
        }

        // side air flow holes
        translate([fatness + 9.2, - .01, fatness + 3]) {
            for (i = [0: 1]) {
                translate([i * 13, 0, 0]) cube([5, 10, 20]);
                translate([i * 13, inner_box.y, 0]) cube([5, 10, 20]);
            }
        }

        // back air flow holes
        translate([box.x - fatness - .01, fatness, 3]) {
            width = 7;
            translate([0, 1, 0]) cube([fatness + .2, width, 20]);
            translate([0, inner_box.y - width - 1, 0]) cube([fatness + .2, width, 20]);
        }

        // screw holders space
        translate([-.01, (box.y - screwholder_size.y) / 2 - .2, box.z - screwholder_size.z + fatness - .2]) {
            cube([fatness + .02, screwholder_size.y + .4, screwholder_size.z - fatness + .4]);
        }
        translate([box.x - fatness - .01, (box.y - screwholder_size.y) / 2 - .2, box.z - screwholder_size.z + fatness - .2]) {
            cube([fatness + .02, screwholder_size.y + .4, screwholder_size.z - fatness + .4]);
        }

        // for debugging (back side)
        // translate([box.x - fatness - .01, fatness, fatness]) cube([100, inner_box.y, 100]);
    }

    // fan ceiling
    translate([box.x - fan_size.x - fatness, 0, fatness]) {
        color("orange") cube([fan_size.x, box.y, inner_height - fan_size.z - .4]);
    }
}


// base
translate([0, - 50]) union() {
    union() {
        cube([box.x, box.y, fatness]);

        difference() {
            translate([fatness + inset / 2, fatness + inset / 2, fatness - .1]) difference() {
                color("blue") cube(base_outer);

                translate([cover_side_fat, cover_side_fat, .01]) {
                    color("red") cube(base_inner);
                }
            }

            // remove back side (almost)
            translate([fatness, fatness + inset / 2]) {
                translate([base_outer.x - cover_side_fat - .01, cover_side_fat, fatness]) {
                    color("green") cube([5, cable_hole_width, 20]);
                }
                translate([base_outer.x - cover_side_fat - .01, cover_side_fat, fatness + 2]) {
                    color("red") cube([5, base_inner.y, 20]);
                }
            }

            // ray hole
            translate([fatness + inset / 2 - .01, fatness + inset / 2]) translate([0, base_outer.y / 2 - ray_hole_laser_dia / 2, fatness])
                {
                    color("green") cube([5, ray_hole_laser_dia, 20]);
                }

            // ventilation holes
            for (y = [0 : 1]) {
                translate([fatness + inset / 2 + 8, y * inner_box.y]) {
                    color("green") cube([20, ray_hole_laser_dia, 20]);
                }
            }
        }

        if (DEBUG) translate([fatness + inset / 2 + cover_side_fat, fatness + inset / 2 + cover_side_fat, fatness + inset / 2]) {
            Heatsink(0, (base_inner.y - heatsink_size.y) / 2, 0);
            Laser(laser_pos_offset, base_inner.y / 2, laser_z_in_heatsink + laser_dia / 2);
            Fan(base_inner.x - fan_size.x, inset / 2, 0);
        }


        // heatsink partitions
        color("yellow") {
            translate([fatness + inset / 2 + cover_side_fat + heatsink_size.x, fatness + inset / 2 + cover_side_fat, fatness - .1]) {
                cube([partition_fatness, base_inner.y, 2]);
            }
            for (i = [1 : 2]) {
                translate([fatness + inset / 2 + cover_side_fat + i * 10, fatness + inset / 2 + cover_side_fat, fatness - .1]) {
                    difference() {
                        cube([partition_fatness, base_inner.y, 2]);
                        /* not debug only!! */ Heatsink(- .01, (base_inner.y - heatsink_size.y) / 2, 0);
                    }
                }
            }
        }

        // fan partitions
        color("green") {
            width = 15;
            translate([fatness + inset / 2 + cover_side_fat + base_inner.x - fan_size.x - partition_fatness, (box.y - width) / 2,
                    fatness - .1]) {
                cube([partition_fatness, width, 2]);
            }

            translate([fatness + inset / 2 + cover_side_fat + base_inner.x - fan_size.x - partition_fatness, fatness + inset / 2 +
                cover_side_fat, fatness - .1]) {
                difference() {
                    s = .8;
                    cube([partition_fatness, base_inner.y, 10]);
                    translate([- .01, s, 0]) cube([5, base_inner.y - s * 2, 20]);
                }
            }
        }

        // screw holders
        ScrewHolder(- screwholder_size.x + fatness + inset / 2, box.y / 2);
        ScrewHolder(box.x - fatness, box.y / 2);
    }
}
