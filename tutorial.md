## Comments, Colons, Question Marks, ';' and '}'
* For single line comments, Mu uses **( // )**.
* Mu is designed to look closer to human writing, as such the colon '**:**' is not an operator. Colons are used with keywords to clarify boundaries, in contexts where the meaning is clearly defined.
* For example the 'as' keyword is used to declare a variable, and is written **( as )** identifier **( : )** value **( ; )**.
* Curly Brackets **{}** are used to for language code blocks. Functions, data structures, and any keyword derived scope. And are compiler enforced for keywords with associated scope.
* For anonymous scoping \*( and )\* are used to create arbitrary local scoping. 
* Semicolons **( ; )** are used to notate the end of a statement or expression, similar to most C inspired languages.

## Variable Declarations and Comments
* There are two types of variables to declare: constants (which are immutable) and mutables. Both are initialized with the value or expression after the a **( : )** character.
* Mutables can be reassigned with the **( := )** operator and Identifiers for mutables must begin with **( # )**, such as **#value** or **#name**.

```
as phrase: “Hello World”;  // constant assignment

as #number: 0;             // mutable assignment
#number := 1;              // reassignment operator
 
```
## Arithmetic and Concatenation
* For arithmetic operators Mu uses **( +, -, /, * )** and for concatenation **( .. )** is used.
* Note: concatenation works with strings and strings or numbers and numbers.
* Lastly, if one of the operands for number concatenation is negative, the runtime errors.
```
// addition
as sum: 3 + 8;

// subtraction
as net: proft - cost;

// multiplication
as product: x * y;

// division
as quotient: x / y;

// concatenation
as phrase: "Hello" .. " " .. "World";
print(phrase);

as number: 2 .. 1;
print(number);
```
```
// "Hello World"
// 21
```
## Logical Operators
Mu uses the keywords **( and, or )** and the characters **( =, <, >, !~, <=, >=, ! )** for logical operators. 
```
1 and 2;   3 or 5;

3 = 3;     1 !~ 2; // not equivalent
3 > 5;    -7 >= 2;
5 < 9;     3 <= 7;	    

!(3 > 2)   // not (3 is greater than 2)

```
## Bitwise Operators
Mu uses standard operators for bitwise and, or, xor, and not. **( &, |, ^, ~ )**. In the future Mu will support bit shifting left or right, but the implementation is still being thought out.
```
print(60 & 13);

print(60 ^ 13);

print(60 | 13);

print(~60);
```
```
//  12
//  41
//  61
// -61
```

## Mutable Assignment Operators
Mutables have short-hand operators to perform mutations. They are similar to other programming languages: sum assignment **(+=)**, product assignment **(*=)**, quotient assignment **(/=)**, concatenation assignment **(.=)**, and modulus assignment **(%=)**.
```
as #value: 34;
as #textString: “Meoow, ”;

// assign the sum of one plus the value
#value += 1; 	 
print(#value);

// assign the sum of negative five and the value
#value += -5;
print(#value);

// multiply the value by ten
#value *= 10;  
print(#value);	 

// divide the value by three
#value /= 3;	  
print(#value);	 

// assigns the remainder of the value from modulus five
#value %= 5;	  

// concatenate #textString
#textString .= “meow.”; 
print(#textString);      
```
```
// 35
// 30
// 300
// 100
// “Meoow, meow.”
```
## Logical Control Flow
Mu has ternary statements **if-else**, **unless-else**. Based on a condition they act as a guard clause for one or two outcomes. 
Both **if** and **unless** can guard a single block of conditional code. 
Note that else-if chaining is not possible in Mu. If a context is needed where one of several statements is true, Mu has a “when” statement. Shown further below. 
Within the scope of an **if** block there can be an else clause. It's important to note that **unless** does not have an alternative clause.

```
if 0 > 10 {
    print("yes zero is greater than ten");
else 
    print("no, zero is not greater than ten");
}

as x: 5;
unless x = 4 {
    print("good"); 
}

unless x = 5 {
    print("it's five"); 
}


if (0 > -5) {     
    print("yes, zero is greater than negative five"); 
}
```
```
// "no, zero is not greater than ten”
// “good”
// "yes, zero is greater than negative five"
```
The **when** statement is similar to switch in C-like languages: it allows for multiple branching outcomes. A key difference is that the when statement can check for any relational pattern ( =, >, <, !~, etc. ). Lastly, the **when** statement is break-by-default.
```
as value : 1;

when value {
    is = 0 { 
        print("it's zero");
    }
    is = 1 { 
        print("it's one");
    } 
    is >= 1 { 
        print("it's greater than or equal to one.");
    }
}

```
```
// “it’s one”
```
Loops are controlled by **while** or **until** statements. **While** is the standard loop that will break when the condition is false. **Until** is the same operations except the loop will break if true. This is to allow programmers to think with the booleans they have available and not worry about inverting. A programmer is free to write "until queue.isEmpty()" or "until stack.size() > 100". 
```
as #count : 1;

while #count < 100 {
    print(#count);
    #count *= 2; 
}


#count := 1;

until #count > 100 {
    print(#count);
    #count *= 3;
}

```
```
// 1 2 4 8 16 32 64
// 1 3 9 27 81
```
(Not in alpha version 0.0.1) you can scope variables to loops in Mu. By adding a comma after **while** or **until** you can add a single declaration. The variable can be referred to as 'is' in the loop condition, or by its name.
```
while, #iteration : 0; is < 5 {
    print(#iteration);
    #iteration += 1;
}

until, #u : 1; is > 35 {
    print(#u);
    #u *= 2;
}

print(#iteration);
```
```
// 0 1 2 3 4
// 1 2 4 8 16 32
// Error: Undefined variable '#iteration'.
```
(Not in alpha version 0.0.1) you can scope two variables to a loop. However, they cannot be aliased with 'is'. This is to make writing two-pointer situations more consistent. 
```
until, #left : 4, #right : 20; #left >= #right {
    print(#right + #left);
    #right += -1;
    #left += 1;
}

while, #index : 0, arraySize : 10; #index < arraySize {
    print(#index);
    #index += 1;
}
```
## Functions
Mu represents functions with anonymous, first-class expressions. 
Functions can access constants from the global scope or within their closure, but they cannot access mutables outside of their scope.
Function parameters can be constant or immutable, a function can be passed in a constant and have a mutable copy of the value.
```
as fibonacci: Number (n) {
    if n < 2 { 
        return n;
    }
    return fibonacci(n - 2) + fibonacci(n - 1);
}

print(fibonacci(20));

as addTogether: Number (x, y) { return x + y; }

print(addTogether(22, 55));

as multiplyBoth: Number (x, y) {
    return x * y;
}
print(multiplyBoth(10, 99));

as divideByTen: Number (x) {
    return x / 10;
}
print(divideByTen(39));

// booleans are refered to as a 'Truth' type in Mu. 
// a Truth will let you know if the value is true or false.

as isEven: Truth (x) { return 0 = x % 2; } 

print(isEven(3));
print(isEven(58));
```
An example of function currying.
```
as defineAdd: Number (x) {
    return Number (y) {
        print(x + y);
        return x + y;
    }
}

as add5plus: defineAdd(5); 

add5plus(3);   // 8
add5plus(1);   // 6
add5plus(2);   // 7
```
An example with mutable Function Parameters
```
as test: Number (x, #y) {
    #y *= #y;
    return x + #y;
}
print(test(1, 2));
```
```
// 5
```
## Function Closures and Global Scope
Functions only being able to get constants from the global scope or their closure is a design choice to reduce side effects. If a mutable is to be used with a function, it has to be passed in as a parameter or declared within the function's scope.
```
//  ** Functions are not aware of mutables outside of their scope **
//      as #number: 6;
//      
//      as increaseNumber: Number (input) {
//          #number := #number + input; // function doesn't know what #number is
//          print(#number);
//      }
//      increaseNumber(5); // won't work
//  ** They are aware of constants, and can capture constants within a closure **

as number: 6;
as increaseNumberBy: Number (x) { 
    return number + x;
}

print(increaseNumberBy(3));  // 9
print(increaseNumberBy(2));  // 8
print(increaseNumberBy(1));  // 7
print(increaseNumberBy(0));  // 6
print(increaseNumberBy(-1)); // 5

as magicNumber: 12;


as triplePlusValue: Number (#z) {
  #z *= 3;
  return #z + magicNumber; 
}
print(triplePlusValue(9));

```
// 45