# 2d Fluid Motion within Terminal 

### how it works

this is a tiny fluid playground made from a grid of cells. each cell remembers
how much red, green, and blue dye it contains, plus which way the surrounding
fluid is moving. when you stir, the program pushes nearby cells. each frame,
dye is carried along by that movement, spreads a little into its neighbours,
and pressure stops the fluid from bunching up or leaving holes. unused motion
fades away, so the tank comes to rest after about ten seconds.

## run

```powershell
cmake -S . -B build
cmake --build build
.\build\Debug\fluid_demo.exe
```

Run it in Windows Terminal or PowerShell 7. The program enables ANSI terminal
support itself so it can redraw one frame in place and show RGB dye colours.

## how it works

Every cell stores RGB dye and a two-dimensional velocity. Each step:

1. A keyboard-controlled circular stirrer adds velocity and dye.
2. Advection moves every cell's value to its velocity-predicted destination.
   The value is distributed between the four closest destination cells, which
   preserves mass more smoothly than choosing only one neighbour.
3. Diffusion repeatedly averages each cell with its four neighbours.
4. A pressure projection removes divergence from the velocity field, giving an
   approximate incompressible Navier--Stokes flow.

I watched a 2 minute paper videos where he explained this basic concept and thought i might try it!
https://youtu.be/mOvtumfyjCs?t=287

## controls

- wasd to move, this injects dye and moves other dye correspondingly
- space adds dye without moving
- 1/2/3/4/5 change colours
- r to clear tank, q/ctrl+c to quit



