# Learn D++ Programming Language

D++ (Dee Plus Plus) is a high-performance, safe, and productive language designed for modern systems programming. It aims to combine the power and flexibility of the D language with memory safety guarantees like ownership, borrowing, and lifetimes, inspired by the Rust language.

This guide provides the basic information to get you started with writing code in D++.

## 1. Setup

The D++ compiler is distributed under the name `dppc`. We assume you have already done the following:

1.  Compiled the D++ compiler from source code (using the `Makefile` we created in previous steps).
2.  Added the resulting `dppc` executable to your system's PATH or are ready to run it from its location.

To check if the compiler is working correctly, try this in your terminal:

```bash
dppc --version
This command should print the compiler version (or a simple message).
```
2. Hello, World!
As with any programming language, let's start with the "Hello, World!" program.
```
// main.d++

fn main() -> void {
    // The println! macro is used to print a line to the console.
    println!("Hello, World!");
}
```
Explanation:

Lines starting with // are comments and are ignored by the compiler.
The fn keyword defines a function.
The main function is the entry point where the program execution begins. Every executable D++ program must have a main function.
The () parentheses specify the function's parameters. The main function currently takes no parameters.
-> void specifies the function's return type. void means the function does not return any value.
The {} curly braces define a block and contain the function's body. Blocks also create a new scope.
println!("Hello, World!"); is a macro call (note the ! at the end). It's used to print text to the console. String literals are enclosed in double quotes "".
Most statements end with a semicolon ;.
Running:

Save the code above in a file named main.d++.
Navigate to the directory containing the file in your terminal.
To compile, run the command dppc main.d++ -o hello. This will create an executable file named hello (or hello.exe on Windows).
To run:
Linux/macOS: ./hello
Windows: hello.exe
You should see Hello, World! printed to the output.

3. Basic Syntax and Concepts
Comments
D++ supports two types of comments:
```
// Single-line comment

/*
A block comment
spanning multiple
lines
*/

/// Documentation comment (Can be used by tools to generate documentation)
fn my_function() -> void {}
Blocks and Scope
Curly braces {} // create a block. Blocks define the scopes where variables are defined and valid.

fn main() -> void {
    let x = 10; // x is defined in this scope

    { // A new, nested scope begins
        let y = 20; // y is only defined in this inner scope
        println!("Inside inner scope: x = {}, y = {}", x, y); // x is accessible from the outer scope
    } // Inner scope ends here, y is no longer defined

    // println!("Inside outer scope: x = {}, y = {}", x, y); // ERROR: y is not defined in this scope!
    println!("Inside outer scope: x = {}", x); // x is still defined
}
```
In D++, many control flow structures (if, while, for, match) use blocks.

Variables
In D++, variables are declared with the let keyword. Variables are immutable (cannot be changed) by default. If you want to change the value of a variable later, you must use the mut keyword.
```
fn main() -> void {
    let x = 5; // Immutable variable x
    // x = 6; // ERROR: Immutable variable cannot be changed

    let mut y = 10; // Mutable variable y
    println!("Initial y: {}", y);
    y = 11; // Valid: Mutable variable changed
    println!("New y: {}", y);

    // Explicit type annotation
    let z: int = 20; // Type can be explicitly specified
    let mut w: float = 3.14; // mutable float
}
```
Basic Data Types
D++ supports several basic data types:

Integers: int, uint, byte, ubyte, short, ushort, long, ulong. Their sizes may vary depending on the platform or specification (e.g., int could be 32-bit or 64-bit). Different sizes can be used with type suffixes (e.g., 123u, 45L) or explicit type annotation.
Floating-Point: float, double, real. Floating-point numbers with different precisions.
Boolean: bool. Takes only the values true or false.
Character: char. Represents a single character (usually 8-bit).
String: string. Represents sequences of text (similar to D's string which can be mutable, or Rust's immutable UTF-8 slice; depends on the D++ design. In this example, an immutable string literal is used).
Void: void. Represents the absence of a value (especially in function return types).
<!-- end list -->
```
fn main() -> void {
    let a: int = 42;
    let b: float = 1.23;
    let c: bool = true;
    let d: char = 'A';
    let e: string = "Hello";

    println!("int: {}", a);
    println!("float: {}", b);
    println!("bool: {}", c);
    println!("char: {}", d);
    println!("string: {}", e);
}
```
4. Control Flow
Control flow structures determine which code is executed.

If / Else If / Else
Provides conditional execution. The condition must be a bool expression.

```dplusplus
fn main() -> void {
let number = 15;

if number > 10 {
    println!("Number is greater than 10.");
} else if number < 10 {
    println!("Number is less than 10.");
} else {
    println!("Number is equal to 10.");
}

// An if expression can return a value (like Rust)
let status = if number > 10 { "Greater" } else { "Less or Equal" };
println!("Status: {}", status);
}
```

### Loops: While, For

Used to repeat blocks of code.

#### While Loop

The block executes as long as the condition is `true`.

```dplusplus
fn main() -> void {
    let mut counter = 0;
    while counter < 5 {
        println!("Counter: {}", counter);
        counter = counter + 1; // or counter += 1;
    }
}
```
For Loop (Iteration)
Iterates over a range, array, or collection.
```
fn main() -> void {
    // Iterating over a range
    for i in 0..5 { // from 0 up to 5 (5 is exclusive)
        println!("For i: {}", i);
    }

    // Perhaps a D-style foreach syntax might also be available:
    /*
    let arr = [10, 20, 30];
    foreach elem; index, value in arr {
        println!("Element[{}]: {}", index, value);
    }
    */
}
```
Break and Continue
Used to alter the flow of loops.
```
fn main() -> void {
    let mut i = 0;
    while i < 10 {
        if i == 3 {
            i = i + 1;
            continue; // Jump to the start of the loop
        }
        println!("Loop i: {}", i);
        if i == 5 {
            break; // Exit the loop
        }
        i = i + 1;
    }
}
```
Match Expression
A powerful construct for matching values against patterns and executing code based on the matched pattern. It's very useful for destructuring enums and data structures.
```
enum Color {
    Red,
    Green,
    Blue,
    RGB(int, int, int), // Associated data
    CMYK { cyan: float, magenta: float, yellow: float, key: float }, // Named associated data
}

fn main() -> void {
    let favorite_color = Color::RGB(255, 0, 100);

    match favorite_color {
        Color::Red => println!("Red."),
        Color::Green => println!("Green."),
        Color::Blue => println!("Blue."),
        Color::RGB(r, g, b) => { // Destructure the RGB variant and bind values to r, g, b variables
            println!("RGB color: ({}, {}, {})", r, g, b);
        }
        Color::CMYK { cyan, magenta, .. } => { // Destructure CMYK variant, take only cyan and magenta (ignore others with ..)
            println!("CMYK color: Cyan={}, Magenta={}", cyan, magenta);
        }
        // _ => println!("Other color"), // Wildcard pattern to cover all other cases
    }

    // A match expression can return a value
    let is_red = match favorite_color {
        Color::Red => true,
        _ => false, // All other cases
    };
    println!("Is favorite color red? {}", is_red);

    // Match expressions must be exhaustive (cover all possible cases), otherwise they cause a compile-time error.
    // The wildcard pattern `_` can be used to cover all remaining cases.
}
```
5. Functions
Functions are used to name code blocks and make them reusable.
```
// Function that adds two integers
fn add(a: int, b: int) -> int {
    a + b // The last expression without a semicolon becomes the return value
    // return a + b; // The 'return' keyword can also be used
}

// Function that prints a message and a count
fn print_message(message: string, count: int) -> void {
    for i in 0..count {
        println!("{}", message);
    }
}

fn main() -> void {
    let result = add(5, 3);
    println!("Sum: {}", result);

    print_message("Hello from function!", 3);
}
```
6. Compound Types
D++ supports various compound data types.

Structs
Allow you to define custom data types with named fields.
```
struct Person {
    name: string,
    age: int,
    active: bool,
}

fn main() -> void {
    // Create a new instance of the Person struct
    let person1 = Person {
        name: "Alice",
        age: 30,
        active: true,
    };

    // Access fields
    println!("Name: {}", person1.name);
    println!("Age: {}", person1.age);

    // Create a mutable struct instance and change its fields
    let mut person2 = Person {
        name: "Bob",
        age: 25,
        active: false,
    };
    person2.age = 26;
    person2.active = true;
    println!("{}'s new age: {}", person2.name, person2.age);
}
```
Enums
Allow you to define a type that represents one of a set of possible values. They are often used with the match expression. An example was shown in the match section.

Tuples
Are fixed-size collections of elements of potentially different types, without names for the elements.
```
fn main() -> void {
    // Create a tuple
    let IT_specialist = ("Ali", 42, true);

    // Access tuple elements by index
    println!("Name: {}", IT_specialist.0); // First element (index 0)
    println!("Age: {}", IT_specialist.1); // Second element (index 1)
    println!("Active: {}", IT_specialist.2); // Third element (index 2)

    // Destructure a tuple into variables
    let (name, age, active) = IT_specialist;
    println!("Destructured: Name={}, Age={}, Active={}", name, age, active);
}
```
Arrays and Slices
Used to store multiple elements of the same type. Arrays have a fixed size, while slices are dynamic-sized views into an array or another slice.
```
fn main() -> void {
    // A fixed-size integer array
    let numbers: [int; 5] = [1, 2, 3, 4, 5];

    // Access elements by index
    println!("First element of array: {}", numbers[0]);
    println!("Third element of array: {}", numbers[2]);

    // Iterate over an array
    for number in numbers {
        println!("Array element: {}", number);
    }

    // Create a slice from an array (using the & operator)
    let slice = &numbers[1..4]; // from index 1 up to (but not including) index 4

    println!("Slice:");
    for element in slice {
        println!("Slice element: {}", element);
    }

    // Slices are dynamic-sized references, their size is determined at runtime
    fn print_slice(slice: &[int]) -> void { // Takes a slice of ints as parameter
        println!("Printing (length {}):", slice.len()); // len() method
        for &x in slice { // Dereference the references to get copies of elements
            println!("{}", x);
        }
    }
    print_slice(&numbers); // Pass a slice of the entire array to the function expecting &[int]
}
```
7. Pointers and References (&, &amp;mut, *)
Used for working directly with memory addresses or creating references to existing data. In D++, references (borrows) are fundamental to the language's ownership and memory safety system.

*T: A pointer to a value of type T.
&T: An immutable reference (shared borrow) to a value of type T.
&mut T: A mutable reference (mutable borrow) to a value of type T.
<!-- end list -->
```
fn main() -> void {
    let value = 100;
    let ref_imm = &value; // Create an immutable reference
    let pointer = &value as *const int; // Create a pointer (casting may be needed)

    println!("Value: {}", value);
    println!("Value via immutable reference: {}", *ref_imm); // Access value via dereference
    println!("Value via pointer: {}", *pointer); // Access value via dereference

    let mut mutable_value = 200;
    let ref_mut = &mut mutable_value; // Create a mutable reference

    println!("Mutable value (before reference): {}", mutable_value);
    *ref_mut = 201; // Change value via mutable reference
    println!("Mutable value (after reference): {}", mutable_value);

    // Multiple immutable references can exist
    let r1 = &value;
    let r2 = &value;
    println!("r1: {}, r2: {}", *r1, *r2);

    // BUT! While a mutable reference is active, NO other references (mutable or immutable) or direct uses of the value are allowed.
    let mut x = 50;
    let m = &mut x; // mutable borrow begins
    // println!("{}", x); // ERROR! x is used directly
    // let r = &x; // ERROR! Immutable borrow is not allowed
    // let m2 = &mut x; // ERROR! Another mutable borrow is not allowed
    *m = 51; // The mutable reference can be used while it is valid

    // When m goes out of scope or is no longer used, the mutable borrow ends.
    // Then x can be used directly or with other references again.
}
```
8. Ownership, Borrowing, and Lifetimes
These are the core concepts in D++ that provide memory safety without garbage collection.

Ownership: Each value has a variable that owns it. When the owner goes out of scope, the value is dropped and its memory is reclaimed.
Moving: Ownership is transferred (moved) by default. When a value is assigned from one variable to another or passed as a function parameter, if its type does not implement the Copy trait, ownership is moved and the original variable becomes invalid.
```
struct Resource { id: int, data: string } // string is not Copy

fn main() -> void {
    let r1 = Resource { id: 1, data: "original".to_string() }; // r1 owns the resource

    let r2 = r1; // Ownership of r1 is moved to r2

    // println!("Data from r1: {}", r1.data); // ERROR! r1 is moved
    println!("Data from r2: {}", r2.data); // r2 is the new owner of the resource
} // r2 goes out of scope, the resource is dropped
```
Copy: Simple, fixed-size types like int, bool, float implement the Copy trait. For these types, assignment or passing as a function parameter results in a copy instead of a move, and the original value remains valid.
```
fn main() -> void {
    let a = 10; // int is Copy
    let b = a; // a is copied, ownership is not moved

    println!("a: {}", a); // Valid
    println!("b: {}", b); // Valid (copy)
}
```
Borrowing: References (&, &mut) provide temporary access to data without transferring ownership. The Borrow Checker (a system within the compiler) enforces the following rules at compile time to ensure memory safety:
At any given time, you can either have one mutable reference (&mut) OR one or more immutable references (&).
While a mutable reference is active, the original value cannot be accessed through any other means (directly, via an immutable reference, or via another mutable reference). <!-- end list -->
```
fn main() -> void {
    let mut data = [10, 20, 30, 40];

    let slice1 = &data[0..2]; // immutable borrow 1
    let slice2 = &data[1..3]; // immutable borrow 2 (valid)

    println!("Slice1: {}", slice1[0]);
    println!("Slice2: {}", slice2[0]);

    // Try to create a mutable borrow while immutable borrows are active
    // let slice_mut = &mut data[2..4]; // ERROR! immutable borrows (slice1, slice2) are active

    // We can create a mutable borrow after the immutable borrows end
} // immutable borrows slice1 and slice2 end here

fn main2() -> void {
    let mut data = [10, 20, 30, 40];
    let slice_mut = &mut data[0..2]; // mutable borrow begins

    // let slice1 = &data[0..2]; // ERROR! mutable borrow (slice_mut) is active
    // println!("{}", data[0]); // ERROR! data is used directly
    // let slice_mut2 = &mut data[1..3]; // ERROR! another mutable borrow is not allowed

    slice_mut[0] = 100; // mutable borrow can be used as long as it's valid

} // mutable borrow slice_mut ends here
```
Lifetimes: Ensure that references are valid for as long as the data they point to lives. The Borrow Checker uses lifetimes to prevent dangling pointers (pointers pointing to deallocated memory). D++ often infers lifetimes automatically, but in some cases (especially in function signatures), explicit lifetime annotations (like 'a, 'b) might be required.
These rules might seem restrictive at first, but they catch memory errors at compile time, significantly reducing runtime bugs like segmentation faults.

9. Traits
Traits are like interfaces or typeclasses that define shared behavior (methods) that types can implement. By implementing a trait, a type guarantees that it provides the methods required by that trait.
```
// A trait defining printable behavior
trait Printable {
    // Any type implementing this trait must provide this method
    fn print(&self) -> void;
}

// MyStruct type implements the Printable trait
struct MyStruct {
    value: int,
}

// The impl block is used to implement a trait for a type
impl Printable for MyStruct {
    fn print(&self) -> void {
        // &self is an immutable reference to the implementing struct
        println!("MyStruct value: {}", self.value);
    }
}

// Another type
struct MyOtherStruct {
    name: string,
}

impl Printable for MyOtherStruct {
    fn print(&self) -> void {
        println!("MyOtherStruct name: {}", self.name);
    }
}


// A function that takes any type implementing the Printable trait as a parameter
fn generic_print(obj: &dyn Printable) -> void {
    // The function can call the print method guaranteed by the trait
    // without knowing the concrete type of the object.
    // This uses dynamic dispatch (the method called is determined at runtime).
    obj.print();
}


fn main() -> void {
    let s1 = MyStruct { value: 123 };
    let s2 = MyOtherStruct { name: "Example".to_string() };

    // Call the method directly
    s1.print();
    s2.print();

    // Call the generic function (using a trait object)
    generic_print(&s1); // automatic coercion from &MyStruct to &dyn Printable
    generic_print(&s2); // &MyOtherStruct to &dyn Printable
}
```
Traits are a powerful tool in D++ for polymorphism and generic programming.

10. Error Handling: Option and Result
In D++, errors and potential absence of values are typically expressed using the Option<T> and Result<T, E> enums. This forces handling potential null values and fallible operations at compile time, making programs safer.

Option<T>: Represents a value that is either present (Some(value)) or absent (None). It's used instead of nullable pointers.
Result<T, E>: Represents a value that is either a successful value of type T (Ok(value)) or an error value of type E (Err(error)). It's used in return types to indicate operations that can fail.
These enums are commonly used with the match expression.
```
// Function that tries to convert a string to an integer (can fail)
fn string_to_int(s: string) -> Result<int, string> { // Returns int on success, string on error
    // (Mock implementation)
    if s == "42" {
        Ok(42) // Success case
    } else {
        Err("Invalid integer format".to_string()) // Error case
    }
}

// A division operation (can fail on division by zero)
fn divide(a: int, b: int) -> Option<int> { // Returns Option<int>, None on division by zero
    if b == 0 {
        None // Division by zero, no value
    } else {
        Some(a / b) // Successful division, return value inside Some
    }
}


fn main() -> void {
    // Using the string_to_int function
    let result1 = string_to_int("42".to_string());
    match result1 {
        Ok(number) => println!("Conversion successful: {}", number),
        Err(error) => println!("Conversion error: {}", error),
    }

    let result2 = string_to_int("abc".to_string());
    match result2 {
        Ok(number) => println!("Conversion successful: {}", number),
        Err(error) => println!("Conversion error: {}", error),
    }

    // Using the divide function
    let division_result1 = divide(10, 2);
    match division_result1 {
        Some(value) => println!("Division successful: {}", value),
        None => println!("Division error: Division by zero!"),
    }

    let division_result2 = divide(10, 0);
    match division_result2 {
        Some(value) => println!("Division successful: {}", value),
        None => println!("Division error: Division by zero!"),
    }
}
```
11. Modules
Modules are used to organize your code and promote reusability.
```
// src/main.d++
mod math; // Loads src/math.d++ or src/math/mod.d++ in the same directory

fn main() -> void {
    let sum = math::add(5, 7); // Call the add function from the math module
    println!("Sum: {}", sum);
}

// src/math.d++ or src/math/mod.d++
// By default, module contents are private.
// Use the `pub` keyword to make them accessible from outside.

pub fn add(a: int, b: int) -> int {
    a + b
}

fn subtract(a: int, b: int) -> int { // Private function
    a - b
}
```
12. Conclusion
This guide introduced the basic syntax and distinguishing features of the D++ programming language (ownership, borrowing, traits, Option/Result). D++ aims to combine the performance of C++ and the productivity of D with the memory safety guarantees it inherits.

D++ is a hypothetical language still under development. The concepts in this guide are based on the design of the compiler components we created in previous steps.

You can deepen your understanding by practicing. Compile and run the example code using your dppc compiler. Try writing code that violates ownership and borrowing rules to observe how the Borrow Checker works.

You've made a great start to developing safe and performant solutions in systems programming with D++!
