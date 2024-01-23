# Mμ Programming Language
[Acknowledgements](https://github.com/CpalmerD20/Mu-Lang-Compiler/blob/main/thank_you.md)
## Variable Declarations
There are two ways to declare a variable: constants (which are immutable) and mutables. Both are declared with the ‘let’ keyword. Constants are initialized with ‘:’ and mutables are assigned with ‘:=‘ . Identifiers for mutables have to begin with "#" such as "#value" or "#name". 

```
let phrase : “Hello World”;    // constant

let #number := 0;             // mutable
#number  := 1;
 
```
## Core Operators
For single line comments, Mu uses ‘//‘, for arithmetic operators Mu uses ('+', '-', '/', '*'), for concatenation '..' is used.
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
```
## Mutable Operators and Comments
Mutables have short-hand operators to perform mutations. Both are similar to other programming languages.
```
let #value := 34;
let #textString := “Meoow, ”;

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
## Logical Operators
Mu uses the keywords ('and' and 'or') and the characters ('=', '<', '>', '!~', '<=', '>=') for logical operators. '!' is the character for 'logical not'.
```
1 and 2;   3 or 5;

3 = 3;     1 !~ 2;
3 > 5;    -7 >= 2;
5 < 9;     3 <= 7;	    

!(3 > 2)   // not (3 is greater than 2)

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
let #count := 1;

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
## Function Closures and Global Scope
Constants are stored in the global scope, while mutables are scoped locally to the file. This is a design choice to reduce side effects, as any function can access the global scope. If a mutable is to be used with a function, it has to be passed in as a parameter or declared within the function's scope.

Further as functions create closures, they ignore mutables. Mutables are never captured within a function's closure.
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

let magicNumber : 12;

let triplePlusValue : use y as
  let #z := y;
  #z *= 3;
  return #z + magicNumber; 
,,
print triplePlusValue(9);

```
## Mutable Function Parameters
```
let #test := use x, #y as
    #y *= #y;
    return x + #y;
,,
print #test(1, 2);
```
```
// 5
```
