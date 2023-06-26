# gpugraph

GPU-based graphing calculator

Supports complex things like implicit equations, intensity charts, complex-plane
graphs, root locus plots, and the like, all with animation, all on the GPU.

### Implicit Equation
![squiggle](https://github.com/electrodude/gpugraph/raw/master/img/squiggle.png)
### Root locus
![root locus](https://github.com/electrodude/gpugraph/raw/master/img/rootlocus2.png)
### Mandelbrot Set
![Mandelbrot set](https://github.com/electrodude/gpugraph/raw/master/img/mandelbrot.png)

Currently requires you to write some GLSL yourself.

See [`graphs/*.txt`](graphs/) for examples.

## TODO

* Saving and loading multiple sessions
  * Group windows per session
* Lock/Unlock x-y scrolling
* Iterative functions
  * Won't happen until I get a GPU that can do geometry shaders or OpenCL
* More options for parameters
  * Configurable slider range
  * Better alternative to stupid wrap-around behavior
  * Multidimensional (vec{2,3,4}) parameters
    * Draggable points
  * Replace x, x', x'', etc. with numerical diffeq solver
    * Would allow e.g. sine wave: dx/dt = y, dy/dt = -x
* Don't require user to write GLSL
* Computer Algebra System
  * Automatically generate Laplace transform plots from time domain functions,
    and vice versa

## BUGS
* Delete graphs/nuklear window drawing order
* Don't hardcode shader uniform IDs.
* Show compile errors in GUI
* Less clunky GUI
