print "===Mu sample start===";

print " --Not Operator";
print 2 = 1;
print !(2 < 5);
print !(3 = 2 + 1);
print !(3 > 3 * 2);
print !(2 = 1);

print " --While Loop";
let x := 1;

while x < 100 ? 
    print x; 
    x := x * 2; 
,,

print " --Until Loop";
x := 1;

until x > 100 ?
    print x;
    x := x * 3;
,,

print " --If Clause";
if 0 > -5 ? 
    print "yes, zero is greater than negative five"; 
,,

if 0 > 10 ? 
    print "yes zero is greater than ten"; 
else 
    print "no, zero is not greater than ten";
// ,,

if false ? 
    print "one"; 
else if false ? 
    print "two"; 
else 
    print "three";


print " --Unless Clause";
x := 5;
unless x = 4 ? 
    print "good"; 
,,

unless x = 5 ? 
    print "not five"; 
else print "five"; // todo always prints...

print " --String Concatenation";
let name := "steve";
let last := " poe";
let full_name := name + last;
print full_name;

print " --functions";
define addTogether(x, y) {
    return x + y;
}
print addTogether(22, 55);

define multiplyBoth(x, y) {
    return x * y;
}
print multiplyBoth(10, 99);

define divideByTen(x) {
    return x / 10;
}
print divideByTen(39);

define isEven(x) {
    return 0 = x % 2;
}
print isEven(3);
print isEven(58);

print "===Mu sample end ===";