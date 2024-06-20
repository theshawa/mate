## Op+erators

### Arithmetic Operators:
+ - * / % 
** exponentiation
// floor division

### Bitwise Operators:
& bitwise and
| bitwise or
^ bitwise xor
~ bitwise not
">>" bitwise right shift
"<<" bitwise left shift

### Comparison Operators:
== != > >= < <=

### Assignment Operators:
= += -= *= /= %= **= //= 
&= |= ^= >>= <<=

### Range Operator:
..

### Access Operator:
.

### Ternary Operators:
? : 

### Logical Operators:
&& || !

## Punctutations:
() [] , ;

## Keywords

### Booleans:
true
false

### Conditionals:
if
then
elseif
else
endif

#### Examples:
```
if a>18 then;
println("you are an adult");
elseif a>12;
println("you are a teenager");
else;
println("you are a child");
endif;
```

### Loops:

while
do
endwhile
in
break
continue

#### Examples:
```
while a>20 do;
print(a);
endwhile;

while i in 0..5 do;
print(i**i);
endwhile;
```

### Define variables:
def

### Primitive types:
int
float
double
string
bool

### None primitive types:
array
map

#### map:
key is either int or string
value is any type

#### Examples
```
def a:int = 24;
def b:double = 24.2;
def c:float = 23.1;
def d:string = "Theshawa Dasun";
def e:bool = false;

def f:array = array("asdads", 24, 23.2, true);
def g:map = map("asdasd"=>24,24=>"asdasd","asd":55);
```

## Comments

### Example
```
# This is a comment
```
