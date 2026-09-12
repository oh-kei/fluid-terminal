# Fluid Motion

### how it works

this is a tiny fluid playground made from a grid of cells. each cell remembers
how much red, green, and blue dye it contains, plus which way the surrounding
fluid is moving. when you stir, the program pushes nearby cells. each frame,
dye is carried along by that movement, spreads a little into its neighbours,
and pressure stops the fluid from bunching up or leaving holes. unused motion
fades away, so the tank comes to rest after about ten seconds.

## Build and run

```powershell
cmake -S . -B build
cmake --build build
.\build\Debug\fluid_demo.exe
```

Run it in Windows Terminal or PowerShell 7. The program enables ANSI terminal
support itself so it can redraw one frame in place and show RGB dye colours.

## What is simulated?

Every cell stores RGB dye and a two-dimensional velocity. Each step:

1. A keyboard-controlled circular stirrer adds velocity and dye.
2. Advection moves every cell's value to its velocity-predicted destination.
   The value is distributed between the four closest destination cells, which
   preserves mass more smoothly than choosing only one neighbour.
3. Diffusion repeatedly averages each cell with its four neighbours.
4. A pressure projection removes divergence from the velocity field, giving an
   approximate incompressible Navier--Stokes flow.

The output is intentionally an interactive coloured terminal animation. A
bright character is dense dye; a space is empty fluid; `O` is the stirrer.

## Controls

- `W`, `A`, `S`, `D`: move the stirrer. Each move injects the selected dye and pushes the
  surrounding velocity field in that direction.
- `Space`: add dye without moving the stirrer.
- `1`: blue dye; `2`: red dye; `3`: green dye; `4`: gold dye; `5`: purple dye.
- `R`: clear the tank.
- `Q`: quit.

No Enter key is needed. Hold a movement key to continue stirring.

## Interaction ideas

- make a fast figure-eight with two different colours, then pause and watch the
  boundary between them fold into ribbons.
- use `4` for gold, hold `D` to make a jet, then steer it upward with `W`.
- paint a pool of one colour with `Space`, stir the edge with a contrasting
  colour, and observe diffusion mix them.
- press `R`, make a single fast sweep, then leave it alone to see dissipation
  settle the tank.
