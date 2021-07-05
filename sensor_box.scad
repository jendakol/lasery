DEBUG = false;

pcb_size = [48.25, 25.7, 2];
height = 24;

space_around = 1;

pcb_hole_dia = 2.9;
pcb_hole_pos = 2;

fatness = 1.2;
inset = 0.2; // very small!

cover_fatness = 0.8;

inner_box = [pcb_size.x + 2 * space_around + inset / 2, pcb_size.y + 2 * space_around + inset / 2, height - fatness];
box = [inner_box.x + 2 * fatness, inner_box.y + 2 * fatness, inner_box.z + fatness];

echo("Inner box size ", inner_box.x, " x ", inner_box.y, " x ", inner_box.z);
echo("Box size ", box.x, " x ", box.y, " x ", box.z);

round_prec = 30;

usb_hole_size = [11, 6];
cable_hole_dia = 3.4;

column_hole_dia = 2;
column_hole_depth = 10 - pcb_size.z;
column_size = [pcb_hole_dia + 2, pcb_hole_dia + 2, 6 + column_hole_depth];

echo("Column size ", column_size.x, " x ", column_size.y, " x ", column_size.z);

// ---------------------------------------------

module Board(x, y, z) {
    translate([x, y, z]) union() {
        difference() {
            color("black") cube(pcb_size);
            translate([pcb_hole_pos, pcb_hole_pos, - .01]) rotate([0, 0, 90])
                cylinder(h = 10, d = pcb_hole_dia, $fn = round_prec);
            translate([pcb_size.x - pcb_hole_pos, pcb_hole_pos, - .01]) rotate([0, 0, 90])
                cylinder(h = 10, d = pcb_hole_dia, $fn = round_prec);
            translate([pcb_size.x - pcb_hole_pos, pcb_size.y - pcb_hole_pos, - .01]) rotate([0, 0, 90])
                cylinder(h = 10, d = pcb_hole_dia, $fn = round_prec);
            translate([pcb_hole_pos, pcb_size.y - pcb_hole_pos, - .01]) rotate([0, 0, 90])
                cylinder(h = 10, d = pcb_hole_dia, $fn = round_prec);
        }
        color("gray") translate([4.5, 0, - 8]) difference() {
            cube([37.5, pcb_size.y, 8]);
            translate([- .01, 1.5, - .01]) cube([50, pcb_size.y - 2 * 1.5, 10]);
        }
        usb_width = 7.7;
        color("red") translate([- 1, (pcb_size.y - usb_width) / 2, pcb_size.z]) cube([5, usb_width, 1]);
    }
}

module Column(x, y, z) {
    translate([x, y, z - .1]) {
        difference() {
            color("blue") cube(column_size);
            color("red") translate([column_size.x / 2, column_size.y / 2, column_size.z - column_hole_depth]) {
                cylinder(h = column_hole_depth + .01, d = column_hole_dia, $fn = round_prec);
            }
        }
        if (DEBUG) Screw(column_size.x / 2, column_size.y / 2, column_size.z + pcb_size.z);
    }
}

module Screw(x, y, z) {
    dia = 5.8;

    translate([x, y, z]) {
        color("red") cylinder(h = 1.5, d = dia, $fn = round_prec);
    }
}

// main box
union() {
    if (DEBUG) Board(fatness + space_around, fatness + space_around, fatness + column_size.z);

    difference() {
        cube([box.x, box.y, box.z]);

        // for debugging:
        // translate([fatness, - .01, fatness]) cube([inner_box.x, fatness + .02, inner_box.z]); // front

        translate([fatness, fatness, fatness])
            cube([inner_box.x, inner_box.y, box.z]);

        // usb hole
        translate([- 1, (box.y - usb_hole_size[0]) / 2, 15])
            cube([10, usb_hole_size[0], usb_hole_size[1]]);

        // holes for wires
        translate([box.x - fatness - .01, box.y / 2, 5]) {
            translate([0, - 5]) rotate([0, 90]) cylinder(h = fatness + .1, d = cable_hole_dia, $fn = round_prec);
            translate([0, 5]) rotate([0, 90]) cylinder(h = fatness + .1, d = cable_hole_dia, $fn = round_prec);
        }
    }

    Column(box.x - space_around - fatness - column_size.x + .2, fatness + space_around - .2, fatness);
    Column(fatness + space_around - .2, box.y - space_around - fatness - column_size.y + .2, fatness);
}

// cover
//translate([0, - 9.8, fatness + box.z]) rotate([180, 0, 0])
translate([0, - 40, 0])
    union() {
        cube([box.x, box.y, fatness]);

        outer = [inner_box.x, inner_box.y, 4.5];
        echo("Cover outer size ", outer.x, " x ", outer.y, " x ", outer.z);
        translate([fatness, fatness, fatness - .01]) difference() {
            color("green") cube(outer);
            translate([cover_fatness, cover_fatness])
                color("yellow") cube([outer.x - cover_fatness * 2, outer.y - cover_fatness * 2, outer.z + .01]);

            color("red") translate([- .01, (outer.y - usb_hole_size[0]) / 2, 3]) cube([10, usb_hole_size[0], usb_hole_size[1]]);
        }
    }
