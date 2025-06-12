outter_dia = 36.9;
height = 1.5;
hole_dia = 3;

difference() {
    color("blue") cylinder(h = height, d = outter_dia, $fn = 40);
    translate([0, 0, -0.1]) color("red") cylinder(h = height + 0.2, d = hole_dia + 0.2, $fn = 40);
}