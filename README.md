# D-Plus-Plus
D++ is a more feature-added version of the Classic D programming language developed by Sahne Dünya. Some software developers may have heard of the D programming language for the first time, especially C/C++ users may be curious. The D programming language is actually an alternative language to C/C++, just like Rust. The D programming language is a language that has been greatly influenced by C++, and can be considered a continuation. When we look at the history of the Classic C language, we see that it is based on the B programming language, C++ is based on the Classic C language, and the D programming language is based on C++. Sahne Dünya has created a new programming language, D++, from these coincidences. The other feature is that D developers expect a more advanced programming language here, and it is. C developers expected more features from C++. The D++ programming language compilation follows the principle of precompilation (AOT) and initially supports x86, but the standard D++ compiler targets a cross-platform compiler and we need your support for this.

# How is the D++ programming language different from the classical D language?
1. Memory management, In Classic D language there are two memory management supported which are Manual and Garbage collection whereas D++ supports three memory management which are Manual, Garbage collection and Rust inspired Ownership and Borrowing.
2. Control structures: In classical D programming language, two control structures are supported, namely if-else and switch, but it does not directly support match statement, whereas in D++ programming language, three control structures are supported, namely if-else, switch and match statement, all three are directly supported.
3. Other features: Some features in the D++ programming language are not available in Classic D or are handled in a more advanced way. Some features are inspired by the Rust programming language. D++ programming language has Traits, Result and Option Types, Lifetimes etc. Features that are not available in Classic D language and are not effective in D++.

# Basic features
* File extension: .d++ (For D++)
* Memory Management: Ownership and borrowing
* Compilation type: Ahead-of-Time
* The executable file developed for it: .dplus
* Underlying programming language: Classic D Programing Languge
* Modern language features: yes
* Standard Libray: yes

# Target Hello World code
```
import std.stdio;

void main()
{
    writeln("Hello World!");
}
