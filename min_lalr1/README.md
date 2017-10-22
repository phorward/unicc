# min_lalr1

This is the minimalist LALR(1) parser generator.

## About

min_lalr1 served as some kind of prototype parser generator for UniCC, and was
initially written in 2007 in the course of experimenting with character-based
parsing methods and their pitfalls.

In the end, min_lalr1 served as the first bootstrap stage to build UniCC.
It is not intended to be used in production or the be complete in any kind.
It just does it's job ;-).

## Building

To build min_lalr1 stand-alone, just do

```
cc -o min_lalr1 min_lalr1.c
```

and that's all.


