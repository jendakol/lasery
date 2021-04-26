PRDEL


magnifier_d = [50,40];
magnifier_f = 30;
magnifier_ring = 0.6;

photodiode_d = 5;

connector_width = 10;

fatness = 1.2;
inset = .2;

round_prec = 40;

// holder = [magnifier_size.x + 10, magnifier_size.y + 2*fatness, 20];

box = [50, magnifier_d[0] + inset + 2*fatness, magnifier_d[0] + 2*fatness];

echo("Box size ", box.x, " x ", box.y, " x ", box.z); 

// ---------------------------------------------

// magnifier holder
union() {
    difference() {
        cube([box.x, box.y, box.z/2]);
        translate([fatness + inset/2, fatness + inset/2, fatness]) {
            cube([box.x - 2*fatness - inset, box.y - 2*fatness - inset, box.z]);
        }

        union() {
            h = (fatness-magnifier_ring)/2;
            center_z = magnifier_d[0]/2 + fatness;

            translate([box.x - h, box.y/2, center_z]) rotate([0, 90]) {
                color("red") cylinder(h = 1, d = magnifier_d[1], $fn = round_prec);
            }
            translate([box.x - fatness - inset/2 - 1, box.y/2, center_z]) rotate([0, 90]) {
                color("red") cylinder(h = 1 + h, d = magnifier_d[1], $fn = round_prec);
            }

            translate([box.x - fatness, box.y/2, center_z]) rotate([0, 90]) {
                color("blue") cylinder(h = fatness, d = magnifier_d[1] - 2.5, $fn = round_prec);
            }
        }
    
        color("green") translate([fatness/2 - inset/2, fatness/2 - inset/2, box.z/2 - 10]) {
            cube([box.x - fatness + inset, box.y - fatness + inset, 10]);
        }
    }

    led_column_width = 8;

    translate([box.x - fatness - magnifier_f - 2, box.y/2 - led_column_width/2, fatness]) {
        color("yellow") 
        difference() {
            cube([2, led_column_width, magnifier_d[0]/2 + 4]);
            translate([-.1, led_column_width /2, magnifier_d[0]/2]) rotate([0, 90]) {
                color("red") cylinder(h = 2 + 0.2, d = 5, $fn = round_prec);
            }
        }
    }
}


// cover
// translate([0, -100, 0])
// union() {
//     difference() {
//         cube([box.x, box.y, box.z/2]);
//         translate([fatness + inset/2, fatness + inset/2, fatness]) {
//             cube([box.x - 2*fatness - inset, box.y - 2*fatness - inset, box.z]);
//         }

//         union() {
//             h = (fatness-magnifier_ring)/2;
//             center_z = magnifier_d[0]/2 + fatness;

//             translate([box.x - h, box.y/2, center_z]) rotate([0, 90]) {
//                 color("red") cylinder(h = 1, d = magnifier_d[1], $fn = round_prec);
//             }
//             translate([box.x - fatness - inset/2 - 1, box.y/2, center_z]) rotate([0, 90]) {
//                 color("red") cylinder(h = 1 + h, d = magnifier_d[1], $fn = round_prec);
//             }

//             translate([box.x - fatness, box.y/2, center_z]) rotate([0, 90]) {
//                 color("blue") cylinder(h = fatness, d = magnifier_d[1] - 2.5, $fn = round_prec);
//             }
//         }
//     }

//     // connection to main box
    
// }
