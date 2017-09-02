### Directory Structures

This directory includes the following subdirectoris and files:
1. ./fiz   includes source code for the FIZ interpreter and test files for FIZ
   ./fiz/fiz.l      LEX file
   ./fiz/fiz.y      YACC/Bison file
   ./fiz/Makefile   
   ./fiz/test1.y    Test file.  Within fiz interpreter, type import test1.y
   ./fiz/test2.y    Test file.
2. rfliz  is a binary for FLIZ interpreter, built on data.cs
3. tf1.f to tf4.f are test files for FLIZ

### FLIZ Language

#### Builtin Functions
| Function | Description |
| -------- | ----------- |
| `head`    | Returns the 1st element of a list. |
| `tail`    | Returns the last element of a list. |
| `list`    | Concatenates 2 lists. |
| `ifn`   | Checks if the 1st list is NULL, if yes return 2nd list. If not return the 3 list. |
| `ifa`   | Checks if the 1st list is just atomic, if yes return 2nd list. If not return the 3 list. |

### FIZ Language

#### Builtin Functions
| Function | Description |
| -------- | ----------- |
| `inc`    | Increases the argument by 1. |
| `dec`    | Decreases the argument by 1. Prints an error on attempt to decrease 0. |
| `ifz`    | If first argument is not 0, evaluates and returns the second argument; otherwise evaluates and returns the third argument. |
| `halt`   | Accepts no arguments. Terminates the interpreter. |

#### Example Scripts

```
(dec
  (ifz 0 4 (inc 1))
)                            ; interpreter prints 3
```

```
(define (add x y)
  (ifz y x
    (add
      (inc x)
      (dec y)
    )
  )
)
(add 1 2)                    ; interpreter prints 3