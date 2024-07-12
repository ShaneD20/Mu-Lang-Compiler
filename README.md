# Mμ Programming Language
* [Acknowledgements](https://github.com/CpalmerD20/Mu-Lang-Compiler/blob/main/thank_you.md) 
* [Language Tutorial](https://github.com/CpalmerD20/Mu-Lang-Compiler/blob/main/tutorial.md)

## Goals
Mu is designed with the intention of revisiting what it would mean to have a "human readable" programming language that is also a "joy to type". Many languages lean heavily one right pinky usage, a big part of Mu is choosing grammar that will more evenly distribute the workload across both hands.

### (1) Be easy to read, with short time-to-code
* Keywords must have clear meaning
* Symbols and operators must have the same meaning given contextual keywords
* Keywords should be whole (non-noun) words that would not be expected to be names for user-defined variables, functions, or objects
* The language syntax and grammars should be relatively ergonomic

### (2) Be relatively easy to debug/refactor
* Immutable variables are the default
* Mutables have a writing convention enforced by the compiler to be easy to find and identify
* Mutables have limited scope and functions are unaware of mutables outside their scope
