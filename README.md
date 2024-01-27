# Mμ Programming Language
[Acknowledgements](https://github.com/CpalmerD20/Mu-Lang-Compiler/blob/main/thank_you.md)
## Colons, Comments, and Variable Declarations
* Mu is designed to look closer to human writing, as such the colon is not an operator. Colons are used with keywords to clarify boundaries, in contexts where the meaning is clearly defined.
* For example the 'let' keyword is used to declare a variable. There are two types of variables to declare: constants (which are immutable) and mutables. Both are initialized with the value or expression after the a ‘:’ character.
* Mutables can be reassigned with the ‘:=‘ operator and Identifiers for mutables must begin with "#", such as "#value" or "#name".
* For single line comments, Mu uses ‘//‘. 

```
let phrase : “Hello World”;   // constant

let #number : 0;             // mutable
#number  := 1;               // reassignment operator
 
```
## Arithmetic and Concatenation
For arithmetic operators Mu uses ('+', '-', '/', '*'), for concatenation '..' is used. Note concatenation works with strings and strings or numbers and numbers. And, if one of the operands for number concatenation is negative, the runtime errors.
```
// addition
let sum : 3 + 8;

// subtraction
let net : proft - cost;

// multiplication
let product : x * y;

// division
let quotient : x / y;

// concatenation
let phrase : "Hello" .. " " .. "World";
print phrase;

let number : 2 .. 1;
print number;
```
```
// "Hello World"
// 21
```
## Logical Operators
Mu uses the keywords ('and' and 'or') and the characters ('=', '<', '>', '!~', '<=', '>=') for logical operators. '!' is the character for 'logical not'.
```
1 and 2;   3 or 5;

3 = 3;     1 !~ 2;
3 > 5;    -7 >= 2;
5 < 9;     3 <= 7;	    

!(3 > 2)   // not (3 is greater than 2)

```
## Bitwise Operators
Mu uses standard operators for bitwise and, or, xor, and not. ( &, |, ^, ~ ). In the future Mu will support bit shifting left or right, but the implemtation is still being thought out.
```
print 60 & 13;

print 60 ^ 13;

print 60 | 13;

print ~60;
```
```
//  12
//  41
//  61
// -61
```

## Mutable Assignment Operators
Mutables have short-hand operators to perform mutations. They are similar to other programming languages: sum assignment (+=), product assignment (*=), quotient assignment (/=), concatenation assignment (.=), and modulus assignment (%=).
```
let #value : 34;
let #textString : “Meoow, ”;

// assign the sum of one plus the value
#value += 1; 	 
print #value;

// assign the sum of negative five and the value
#value += -5;
print #value;

// multiply the value by ten
#value *= 10;  
print #value;	 

// divide the value by three
#value /= 3;	  
print #value;	 

// assigns the remainder of the value from modulus five
#value %= 5;	  

// concatenate #textString
#textString .= “meow.”; 
print #textString;      
```
```
// 35
// 30
// 300
// 100
// “Meoow, meow.”
```
## The ,, operator and ?
In programming languages it's beneficial to express the end of a block of code. In Mu ',,' is used to express the ending of a code block. 
The ? operator is used to denote a guard-clause protected by a boolean condition. In other programming languages this would be represented with a 'then' keyword.

## Logical Control Flow
Mu has ternary statements “if-else”, “unless-else”. Based on a condition they act as a guard clause for one or two outcomes. Note that else-if chaining is not possible in Mu. If a context is needed where one of several statements is true, Mu has a “when” statement. Shown further below. 

```
if 0 > 10 ?
    print "yes zero is greater than ten";
else 
    print "no, zero is not greater than ten";
,,


let x: 5;
unless x = 4 ?
    print "good"; 
else    
    print "not good";
,,


unless x = 5 ?
    print "it's not five"; 
else
    print "it is five";
,,


if (0 > -5) ?     
    print "yes, zero is greater than negative five"; 
,,
```
```
// "no, zero is not greater than ten”
// “good”
// "it is five"
// "yes, zero is greater than negative five"
```
The "when" statement is similar to switch in C-like languages: it allows for multiple branching outcomes. A key difference is that the when statement can check for any conditional operator (= > < !~) etc. Lastly, the "when" statement is break-by-default.
```
let value : 1;

when value:
    is = 0 ? print "it's zero";
    ,,
    is = 1 ? print "it's one";
    ,, 
    is >= 1 ? print "it's greater than or equal to one.";
    ,,
,,

```
```
// “it’s one”
```
Loops are controlled by 'while' and 'until'. While is the standard loop that will break when the condition is false. Until is the same operations except the loop will break if true. This is to allow programmers to think with the booleans they have available and not worry about inverting. A programmer is free to write "until queue.isEmpty()" or "until stack.size() > 100". 
```
let #count : 1;

while #count < 100 ?
    print #count;
    #count *= 2; 
,,


#count := 1;

until #count > 100 ?
    print #count;
    #count *= 3;
,,

```
```
// 1 2 4 8 16 32 64
// 1 3 9 27 81
```
## Functions
Mu represents functions with "use" expressions. The are anonymous and first-class. Following the grammars 'use' parameters* 'as' expression. The 'as' keyword can be omitted if the function simply returns an expression. 
  Functions can access constants from the global scope or within their closure, but they cannot access mutables outside of their scope.
Function parameters can be constant or immutable, a function can be passed in a constant and have a mutable copy of the value.
```
let fibonacci: use n as
    if n < 2 ? return n;
    ,,
    return fibonacci(n - 2) + fibonacci(n - 1);
,,

print fibonacci(20);

let addTogether:
    use x, y return x + y;
,,
print addTogether(22, 55);

let multiplyBoth:
    use x, y return x * y;
,,
print multiplyBoth(10, 99);

let divideByTen:
    use x return x / 10;
,,
print divideByTen(39);

let isEven: 
    use x return 0 = x % 2;
,,

print isEven(3);
print isEven(58);
```
An example of function currying.
```
let defineAdd: 
    use x return use y as
        print x + y;
        return x + y;
    ,,
,,

let add5plus : defineAdd(5); 

add5plus(3);   // 8
add5plus(1);   // 6
add5plus(2);   // 7
```
An example with mutable Function Parameters
```
let test : use x, #y as
    #y *= #y;
    return x + #y;
,,
print test(1, 2);
```
```
// 5
```
## Function Closures and Global Scope
Functions only being able to get constants from the global scope or their closure is a design choice to reduce side effects. If a mutable is to be used with a function, it has to be passed in as a parameter or declared within the function's scope.
```
//  ** Functions are not aware of mutables outside of their scope **
//      let #number : 6;
//      
//      define increaseNumber() as
//          #number := #number + 1; // function doesn't know what #number is
//          print #number;
//      ,,
//      increaseNumber(); // won't work
//  ** They are aware of constants, and can capture constants within a closure **

let number : 6;
define increaseNumberBy(x) as
    return number + x;
,,

print increaseNumberBy(3);  // 9
print increaseNumberBy(2);  // 8
print increaseNumberBy(1);  // 7
print increaseNumberBy(0);  // 6
print increaseNumberBy(-1); // 5

let magicNumber : 12;

let triplePlusValue : use y as
  let #z : y;
  #z *= 3;
  return #z + magicNumber; 
,,
print triplePlusValue(9);

```
