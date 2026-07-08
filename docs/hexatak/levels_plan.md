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
