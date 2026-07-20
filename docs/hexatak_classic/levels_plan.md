# Levels

## Concepts
1. Connection between two sides of the hex
   - Can be any two sides
2. Step a stone
3. Step a tower
4. Spread
5. Stacking stones / towers
6. Merging same value stones
7. Required-value gates
8. Blocked cells
9. Stack order -> spread order
10. Multiple spread distances
11. Multiple axis
12. Combination

## Notation
```
[124] = stack, bottom -> top
G2    = gate requiring top value 2
H2    = gate requiring heigth 2
#     = blocked
.     = empty
```

## Planning
### The Gap
Teaches: connection + step movement.
One click-select, one click-move. The player learns the win condition.
```
.   [1]  .
[1]  .  [1]
```

### The Gap 2
Teaches: step movement over a slightly larger shape.
This confirms that connection is about adjacency, and not in necessarily in a row.
```
 .   .  [1]  .  [1]
 .  [1]  .   .   .
[1]  .   .   .  [1]
 .   .   .   .   .
```

### First Merge Gate
Teaches: merge + gate.
```
.      [1]  .
[1] G2:[1] [1]
```

### The large gap
Teaches: spread.
One spread fills the road. This should be the first click-and-drag level.
```
[1] [111] . . [1]
```

### Teaches: move a stack, then spread it.
Teaches: move a stack, then spread it.
The stack is not aligned correctly at first. The player must step it once, then spread.
This should be instead using a diagonal, forcing to step on the correct diagonal to bridge the gap, but same ideas
as in this sketch.
```
[1] . [111] . [1]
```

### Bridge Building
Teaches: stack towers to gain spread length.
Important: avoid equal interface values here unless the level is intentionally teaching merge. This level is about height, not value merge.

```
name: Bridge Building
desc: Stack towers to gain height, then spread them.
tip: Move the towers onto each other to stack them up before spreading. Taller towers can be spread over a longer distance, which helps bridge larger gaps.
radius: 2
side_a: Q_NEG
side_b: Q_POS
move_limit: 2
stack: -2,0:2:2,4
stack: -1,0:2:1,2
stack: 2,0:1:1
```

Or something like, in two moves:
```
[1] [1] . [1] [1]

       [1]
       [24]
```
Maybe this could be The Bridge 2

### Short spread
Teaches: spreading less than the full stack.
The correct solution uses only part of the stack.
Spreading too much should leave a gap.
`[1111] [1] . . [1]`

### Short spread + gate
Teaches: spread distance can determine gate value.
The player must drop the correct stone on the gate. Spreading too far or in the wrong order fails.
`[1] [112] . G2 [1]`

### Order of the Split
Teaches: spread order matters because stones are placed sequentially.
Similar to 
```
```
But add gates to force order of split.

### Resonance Cascade
Teaches: merge chains.
```
 .   .  [21]
[1] [1]  G4  [1]
 .   .  [12]  .
```

### Toll bridge
Teaches: produce a specific gate value.
```
[1] [12] G4 [12] [1]
```

### Exact Toll Bridge
Teaches: bigger is not always better.
The player must keep top value 2, not accidentally create 4.
Would like for it to require a move + spread to put a one down

```
[1] [12] G1 G2[12] [1]
```

### Move the stack
Teaches: whole-stack movement preserves order and top values.

`[1] G4 [124] G1 [1]`

### Diagonal
```
      .   [1]
    .   [111]
[1] .   .   . [1]
    .   .
      .
```

### Around a block
might want it earlier?
```
[1] [111] # . [1]
        .
```

### Long way home
Requires two partial spread to make a zig-zag

### Synchro

Two stacks that needs to spread correct order to gated in the middle

### Stepping in Sync

Multiple stacks + one that needs to be step to meet gate

### The great barrier
Uses spread, merge, exact gates, blockers, interface contact, and stack order.
```
        .   .   .   .
      .   [1] .   .
    .   [12] # [21]
[1] .   G4  . [1]
    .   [1241] . .
      .   .   .   .
        .   .   .   .
```

## Level ideas
### The split
```
name: The Split
desc: Spread the central tower to bridge both sides.
tip: Click and drag from the center stack to spread the stones across the board. When you spread a stack, it places stones of matching heights in sequence. You want to connect the WEST (Left) side to the EAST (Right) side. Pay attention to the move limit!
radius: 2
side_a: Q_NEG
side_b: Q_POS
move_limit: 2
stack: -2,0:1:1
stack: 0,0:4:1,1,2,4
```

### Bridge building
```
name: Bridge Building
desc: Stack towers to gain height, then spread them.
tip: Move the towers onto each other to stack them up before spreading. Taller towers can be spread over a longer distance, which helps bridge larger gaps.
radius: 2
side_a: Q_NEG
side_b: Q_POS
move_limit: 2
stack: -2,0:2:2,4
stack: -1,0:2:1,2
stack: 2,0:1:1
```

### Merge + gates
```
name: Power of Two
desc: Merge matching stones to activate the central bridge.
tip: When you move a stone of value V onto another stone of value V, they merge to double their value (2V). The central bridge tile requires a stone of value 2. Move a 1 onto another 1 at the center to merge them into a 2!
radius: 1
side_a: Q_NEG
side_b: Q_POS
move_limit: 1
stack: -1,0:1:1
stack: 0,0:1:1
stack: 1,0:1:1
stack: 0,1:1:1
required: 0,0:2
```

### Blocking the shortest path
```
name: Around the Block
desc: Navigate around the central blockage.
radius: 2
side_a: Q_NEG
side_b: Q_POS
move_limit: 2
stack: -2,0:1:1
stack: -1,0:3:1,2,4
stack: 1,0:3:1,2,4
stack: 2,0:1:1
blocked: 0,0
```

### Long run
Could require to first build the tower?
```
name: The Long Run
desc: Spread a very tall tower across a radius 3 board.
radius: 3
side_a: Q_NEG
side_b: Q_POS
move_limit: 1
stack: -3,0:1:1
stack: -2,0:5:1,2,4,8,16
stack: 3,0:1:1
```

### The wall
Level where goal is two adjacent sides of the hex, but blocked cell, need to go around

### Merge + Gate (take two)
```
name: Toll Bridge
desc: The central bridge tile requires a specific value to activate.
tip: Look at the required value on the bridge. Only a stone of that exact value on top of the stack will act as a bridge. Move the stacks towards the center to merge their matching top stones and create the required 4!
radius: 2
side_a: Q_NEG
side_b: Q_POS
move_limit: 2
stack: -2,0:1:1
stack: -1,0:2:1,2
stack: 1,0:2:1,2
stack: 2,0:1:1
required: 0,0:4
```
