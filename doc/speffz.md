# Overview

This document describes the Speffz cycle notation supported by vcube,
intended as an input format for quickly entering a 3x3x3 cube state
manually.

[Speffz](https://www.speedsolving.com/wiki/index.php/Speffz) is a lettering
scheme commonly used in blindfolded solving.  The 24 edge and 24 corner
stickers on the cube are each labeled with the letters `a` through `x`.

The cube state is decomposed into cycles of corner and edge stickers,
and the sticker letters are written in cycle order.

## Sticker lettering

The sticker lettering scheme is as follows:

           a a b
           d[U]b
           d c c
    
    e e f  i i j  m m n  q q r
    h[L]f  l[F]j  p[R]n  t[B]r
    h g g  l k k  p o o  t s s
    
           u u v
           x[D]v
           x w w

In the above diagram, the bracketed letters `U, R, F, D, L, B` are included
for reference only, and are not part of the lettering scheme.

## Buffers

The notation is written with respect to corner and edge *buffers*.  All
maneuvers (exchanges and reorientations) are performed with respect to
a buffer.  Unless configured otherwise, the following buffers are assumed:

    Corner: A
    Edge:   U

The default buffer choices originate from the blindfolded solving methods
[Classic Pochmann](https://www.speedsolving.com/wiki/index.php/Classic_Pochmann)
for corners and [M2](https://www.speedsolving.com/wiki/index.php/M2/R2) for
edges, but are otherwise arbitrary.

## Buffer cubies

The term *buffer cubie* refers to a cubie which contains a buffer sticker.
The stickers `A`, `E` and `R` comprise the default buffer corner, and the
stickers `K` and `U` comprise the default buffer edge.

Buffer cubie letters never appear in well-formed notation because they add
no information.

## Buffer location

The term *buffer location* refers to the home position of the buffer.

# Syntax

    cube-state    = sticker-list [ "." sticker-list ]
                     ; corner stickers first, then edge stickers
    
    sticker-list  = *sticker
    
    sticker       = %x61-78 / %x41-58
                    ; a-x: sticker to be exchanged with the buffer
                    ; A-X: sticker to be reoriented in-place

# Reading the notation

The notation can be interpreted as a sequence of *exchanges* and in-place
*reorientations* of stickers with respect to the buffer, which when applied
to the cube state described, will restore the cube to the solved state.
When reading the notation, each letter refers to a *location* on the cube.

## Exchanges

A lowercase letter denotes a location to be exchanged with the buffer.
The two cubies are swapped and oriented such that the named location
is effectively exchanged with the buffer location.

### Example corner exchange

The exchange `f` will swap two cubies such that the stickers at locations
`f` and `a` are exchanged.

Before:

    Location    Cubie
    --------    -----
    (a)e r      (f)d i   <- corner buffer
     d i(f)      m j(c)

After:

    Location    Cubie
    --------    -----
    (a)e r      (c)m j   <- corner buffer
     d i(f)      d i(f)


## In-place reorientations

A cubie may be reoriented in-place by making two exchanges.  For example,
the pair of corner exchanges `jc` twists the `URF` corner clockwise, with
the side-effect of twisting the buffer (`ULB`) in the opposite direction.
Similarly, the pair of edge exchanges `ci` flips the `UF` edge, with the
side-effect of flipping the buffer (`DF`) as well.

Uppercase letters are an optional shorthand for these in-place reorientations.

### Corners

Uppercase corner letters twist the cubie at the named location such that
the named sticker ends up on the *up* or *down* face.  As a side-effect,
the buffer is twisted in the opposite direction.

For example, the stickers `cmj` denote the `URF` corner.  The reorientation
`M` will twist the cubie at that location counterclockwise (and the buffer
clockwise); `J` will twist the cubie clockwise (and the buffer
counterclockwise.)  The uppercase corner `C` has no effect because it
already refers to a location on the *up* face, so it never appears in
well-formed notation.

### Edges

Uppercase edge letters flip the cubie at the named location, and flip the
buffer as a side-effect.  Both of an edge cubie's letters are equivalent;
`C` and `I` have the same effect.

# Writing the notation

The notation can be written by examining all corners and edges using the
following algorithm:

    procedure TRACE:
        Examine the sticker at the current location
        Unless the sticker belongs to the buffer cubie:
            Write down the letter for that sticker
        TRACE the sticker's home location

    TRACE the buffer location
    While there are still unexamined cubies:
        Choose an unexamined cubie
        If the cubie is in its solved position:
             Skip
        Else
            Choose a sticker on that cubie
            Write down the letter for that sticker
            TRACE the sticker's current location
        End
    End

## Example

### Scramble sequence:

    L' B' R F2 D' U' B R D2 U2 B2 F' D B' L U2 L U'

          f i w
          t . k
          e l m
    d n a r f c j u t o c i
    m . d e . s w . q a . b
    x g l u r b q v p v p h
          g h n
          x . o
          s j k

### Corners

Start at the buffer location.

         (f). w
          . . .
          e . m
    d . a r . c j . t o . i
    . . . . . . . . . . . .
    x . l u . b q . p v . h
          g . n
          . . .
          s . k

Write down `f`, move to location `f`.

          * . w
          . . .
          e . m
    * .(a)r . c j . t o . *
    . . . . . . . . . . . .
    x . l u . b q . p v . h
          g . n
          . . .
          s . k

Sticker at location `f` is `a`, which is on the buffer cubie, so do not
write it down.  The cubie at location `a` has been visited already, so
that ends the first cycle (`f`).

The cubie at location `bqn` is unvisited, so write down `b` and visit
location `b` next.

          * .(w)
          . . .
          * . m
    * . * * . c j . t o . *
    . . . . . . . . . . . .
    x . l u . b q . p v . h
          g . n
          . . .
          s . k

Write down `w`, move to location `w`.

          * . *
          . . .
          * . m
    * . * * . c j . * * . *
    . . . . . . . . . . . .
    x . l u . b q . p v . h
          g . n
          . . .
          s .(k)

Write down `k`, move to location `k`.

          * . *
          . . .
          * . m
    * . * * . c j . * * . *
    . . . . . . . . . . . .
    x . l u .(b)q . * * . h
          g . n
          . . .
          s . *

Write down `b`.  The cubie at location `b` has been visited already, so
that ends the second cycle (`bwkb`).

The cubie at location `ugl` is unvisited, so write down `u` and visit
location `u` next.

          * . *
          . . .
          * . m
    * . * * . c j . * * . *
    . . . . . . . . . . . .
    x . l u . * * . * * . h
         (g). *
          . . .
          s . *

Write down `g`.  The cubie at location `g` has been visited already,
so that ends the third cycle (`ug`).  Note: This is an in-place twist
and can be shortened to `L`.

The cubie at location `cmj` is unvisited, so write down `c` and visit
location `c` next.

          * . *
          . . .
          * .(m)
    * . * * . c j . * * . *
    . . . . . . . . . . . .
    x . * * . * * . * * . h
          * . *
          . . .
          s . *

Write down `m`.  The cubie at location `m` has been visited already, so
that ends the fourth cycle (`cm`).  Note: This is an in-place twist
and can be shortened to `J`.

The cubie at location `xsh` is unvisited, so write down `x` and visit
location `x` next.

          * . *
          . . .
          * . *
    * . * * . * * . * * . *
    . . . . . . . . . . . .
    x . * * . * * . * * . h
          * . *
          . . .
         (s). *

Write down `s`.  The cubie at location `s` has been visited already,
so that ends the fifth and final cycle (`xs`).  Note: This is an in-place
twist and can be shortened to `H`.

The corner notation is thus:

    fbwkbLJH

### Edges

Start at the buffer location.

          . i .
          t . k
          . l .
    . n . . f . . u . . c .
    m . d e . s w . q a . b
    . g . . r . . v . . p .
          .(h).
          x . o
          . j .

Write down `h`, move to location `h`.

          . i .
          t . k
          . l .
    . n . . f . . u . . c .
   (m). d e . s w . q a . b
    . g . . * . . v . . p .
          . * .
          x . o
          . j .

Write down `m`, move to location `m`.

          . i .
          t . k
          . l .
    . n . . f . .(u). . c .
    * . d e . s w . q a . *
    . g . . * . . v . . p .
          . * .
          x . o
          . j .

Sticker at location `m` is `u`, which is on the buffer cubie, so do not
write it down.  The cubie at location `u` has been visited already, so
that ends the first cycle (`hm`).

The cubie at location `aq` is unvisited, so write down `a` and visit
location `a` next.

          .(i).
          t . *
          . l .
    . n . . f . . * . . c .
    * . d e . s w . q a . *
    . g . . * . . v . . p .
          . * .
          x . o
          . j .

Write down `i`, move to location `i`.

          . * .
          t . *
          . l .
    . n . .(f). . * . . * .
    * . d e . s w . q a . *
    . g . . * . . v . . p .
          . * .
          x . o
          . j .

Write down `f`, move to location `f`.

          . * .
          t . *
          . * .
    . n . . * . . * . . * .
    * .(d)e . s w . q a . *
    . g . . * . . v . . p .
          . * .
          x . o
          . j .

Write down `d`, move to location `d`.

          . * .
         (t). *
          . * .
    . n . . * . . * . . * .
    * . * * . s w . q a . *
    . g . . * . . v . . p .
          . * .
          x . o
          . j .

Write down `t`, move to location `t`.

          . * .
          * . *
          . * .
    . * . . * . . * . . * .
    * . * * . s w . q(a). *
    . g . . * . . v . . p .
          . * .
          x . o
          . j .

Write down `a`.  The cubie at location `a` has been visited already, so
that ends the second cycle (`aifdta`).

The cubie at location `jp` is unvisited, so write down `j` and visit
location `j` next.

          . * .
          * . *
          . * .
    . * . . * . . * . . * .
    * . * * .(s)w . * * . *
    . g . . * . . v . . p .
          . * .
          x . o
          . j .

Write down `s`, move to location `s`.

          . * .
          * . *
          . * .
    . * . . * . . * . . * .
    * . * * . * * . * * . *
    . g . . * . . v . .(p).
          . * .
          x . o
          . j .

Write down `p`.  The cubie at location `p` has been visited already,
so that ends the third cycle (`jsp`).

The cubie at location `vo` is unvisited, so write down `v` and visit
location `v` next.

          . * .
          * . *
          . * .
    . * . . * . . * . . * .
    * . * * . * * . * * . *
    . g . . * . . v . . * .
          . * .
          x .(o)
          . * .

Write down `o`.  The cubie at location `o` has been visited already,
so that ends the fourth cycle (`vo`).  Note: This is an in-place flip
and can be shortened to `O` or `V`.

The only unvisited cubie is `xg` which is already in the solved position,
so we skip it.

The edge notation is thus:

    hmaifdtajspO

### Combined notation

Combining the corner and edge notation yields a complete description
of the cube state:

    fbwkbLJH.hmaifdtajspO

With practice, cubes can be input quickly in this manner.
