# Mu Programming Language

## Variable Declarations
There are two ways to declare a variable: constants (which are immutable) and mutables. Both are declared with the ‘let’ keyword. Constants are initialized with ‘:’ and mutables are assigned with ‘:=‘ . As show below:

```
let phrase : “Hello Word”;

let #number := 0;  
#number  := 1;
 
```

## Mutable Operators and Comments
Mutables have short-hand operators to perform mutations. For single line comments, Mu uses ‘//‘. Both are similar to other programming languages.
```
let #value := 34; 

#value += 1; 	// assigns the sum of one plus the value
print #value;	// 35
  
#value -= 5;	// subtracts five from the value
print #value;	// 30

#value *= 10;   // multiplies the value by ten
print #value;	// 300

#value /= 3;	// divides the value by three
print #value;	// 100

#value %= 5;	// assigns the remainder of the value from modulus five

let #textString := “Meoow,”; 
#textString .= “ meow.”; // assigns the concatenation of textString and “ meow.”
print #textString; // “Meoow, meow.”
```

## Logical Operators
```
1 and 2 // logical and

3 or 5	// logical or

3 = 3	// equality
3 > 5	// greater
3 < 7	// less

5 <= 9 		 // less than or equal to
-7 >= 2		// greater than or equal to

```
## Logical Control Flow
Mu has ternary statements “if-else”, “unless-else”. As a design choice else-if is not supported. If a context is needed where one of several is true, Mu has a “when” statement. 

*note: when statements currently have fallthrough (“it’s greater than or equal to one” current prints as well), in the specification each ‘is’ clause will break by default.

```
if 0 > 10 ?
    print "yes zero is greater than ten";
else 
    print "no, zero is not greater than ten";
,,

// "no, zero is not greater than ten”

let x: 5;
unless x = 4 ?
    print "good"; 
else    
    print "not good";
,,

// “good”

unless x = 5 ?
    print "it's not five"; 
else
    print "it is five";
,,

// "it is five"

let value : 1;

when value:
    is = 0 ? print "it's zero";
    ,,
    is = 1 ? print "it's one";
    ,, 
    is >= 1 ? print "it's greater than or equal to one.";
    ,,
,,
// “it’s one”

```
Loops are controlled by 'while' and 'until'.
```
let #count := 1;

while #count < 100 ?
    print #count;
    #count *= 2; 
,,

// 1 2 4 8 16 32 64

#count := 1;

until #count > 100 ?
    print #count;
    #count *= 3;
,,

// 1 3 9 27 81

```
## Functions
Mu declares functions with the “define” keyword as show below.
```
define addTogether(x, y) as
    return x + y;
,,
print addTogether(22, 55);

define multiplyBoth(x, y) as
    return x * y;
,,
print multiplyBoth(10, 99);

define divideByTen(x) as
    return x / 10;
,,
print divideByTen(39);

define isEven(x) as
    return 0 = x % 2;
,,

print isEven(3);
print isEven(58);

define fibonacci(n) as
    if n < 2 ? return n;
    ,,
    return fibonacci(n - 2) + fibonacci(n - 1);
,,

print fibonacci(20);
```
## Function and Global Scope
Constants are stored in the global scope, while mutables are scoped locally to the file. This is a design choice to reduce side effects, as any function can access the global scope. If a mutable is to be used with a function, it has to be passed in as a parameter, declared within the function, or declared within a closure.
```
//  ** Mutables are not in global scope **
//      let #number := 6;
//      
//      define increaseNumber() as
//          #number := #number + 1; // function doesn't know what #number is
//          print #number;
//      ,,
//      increaseNumber(); // won't work
//
//  ** Constants are in global scope **

let number : 6;
define increaseNumberBy(x) as
    return number + x;
,,

print increaseNumberBy(3);  // 9
print increaseNumberBy(2);  // 8
print increaseNumberBy(1);  // 7
print increaseNumberBy(0);  // 6
print increaseNumberBy(-1); // 5

```
