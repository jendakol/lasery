dia = 12;
main_length = 35.8;
total_length = 53.9;
length_space = 8;
ray_hole_dia = 5;
screw_hole_dia = 3.5;
cable_hole_width = 5;

fatness = 1.2;
cover_side_fatness = 0.8;
inset = 0.4;
round_prec = 20;

// ---

cover_inner = [total_length + length_space + inset, dia + inset, 20];
echo("Cover inner box size ", cover_inner.x, " x ", cover_inner.y); 

cover_outer = [cover_inner.x + cover_side_fatness*2, cover_inner.y + cover_side_fatness*2, 10];
echo("Cover outer box size ", cover_outer.x, " x ", cover_outer.y); 

inner_box = [cover_outer.x + inset, cover_outer.y + inset, 20];
echo("Inner box size ", inner_box.x, " x ", inner_box.y); 

box = [inner_box.x + fatness*2, inner_box.y + fatness*2, fatness + dia];
echo("Box size ", box.x, " x ", box.y, " x ", box.z); 


// ---------------------------------------------

module ScrewHolder(x, y) {
    size = [8, 8, 3];

    translate([x, y - size.x/2]) difference() {
        cube([size.x, size.y, size.z]);
        translate([size.x/2, size.y/2]) rotate([0, 0, 90]) {
            cylinder(h=size.z, d = screw_hole_dia, $fn = round_prec);
            translate([0, 0, size.z - 1]) color("yellow") cylinder(h=1, d = screw_hole_dia + 2, $fn = round_prec);
        }
    }
}

// cover
union() {
    difference() {
        cube(box);

        translate([fatness, fatness, fatness])
            cube(inner_box);
        
        // ray hole
        translate([-0.2, box.y/2, box.z - dia/2]) rotate([0,90]) {
            cylinder(h=fatness+0.4, d = ray_hole_dia, $fn = round_prec);
        }

        // cable hole
        translate([box.x - fatness, box.y/2 - cable_hole_width/2, box.z - 4]) {
            cube([5, cable_hole_width, 4]);
        }
    }
}


// base
translate([0, -30]) union() {
    union() {
        cube([box.x, box.y, fatness]);

        difference() {        
            translate([fatness + inset/2, fatness + inset/2, fatness -.1]) difference() {
                color("blue") cube(cover_outer);

                translate([cover_side_fatness, cover_side_fatness]) {                
                    color("red") cube(cover_inner);
                }
            }

            // cable hole
            translate([fatness, fatness + inset/2]) translate([cover_outer.x - cover_side_fatness, cover_outer.y/2 - cable_hole_width/2, fatness]) {
                color("green") cube([5, cable_hole_width, 20]);
            }

            // ray hole
            translate([fatness + inset/2, fatness + inset/2]) translate([0, cover_outer.y/2 - ray_hole_dia/2, fatness]) {
                color("green") cube([5, ray_hole_dia, 20]);
            }
        }

        // laser cylinder partition
        color("yellow") translate([fatness + inset/2 + cover_side_fatness + main_length, fatness + inset/2 + cover_side_fatness, fatness -.1]) {
            cube([.8, cover_inner.y, 1]);
        }

        // screw holders
        ScrewHolder(-8 + .05, box.y/2);
        ScrewHolder(box.x - .05, box.y/2);
    }
}
