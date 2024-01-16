print "===Mu sample start===";

print " --While Loop";
let x : 1;

while x < 100 ? 
    print x; 
    x : x * 2; 
,,

print " --Until Loop";
x : 1;

until x > 100 ?
    print x;
    x : x * 3;
,,

print " --If Clause";
if 0 > -5 ? 
    print "yes, zero is greater than negative five"; 
,,

if 0 > 10 ? 
    print "yes zero is greater than ten"; 
else print "no, zero is not greater than ten";

if false ? 
    print "one"; 
else if false ? 
    print "two"; 
else 
    print "three";

print " --String Concatenation";
let name : "steve";
let last : " poe";
let full_name : name + last;
print full_name;

print "===Mu sample end===";