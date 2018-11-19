# vcube

vcube is a fast optimal half-turn metric Rubik's cube solver that takes
advantage of vector instructions added in Intel's Haswell microarchitecture.
It is a rewrite of the optimal solver behind
[Voltara's Optimal Scrambler](https://voltara.org/cube/) and combines the
best ideas from the original solver, and from Tomas Rokicki's
[nxopt](https://github.com/rokicki/cube20src).

Using 22 GiB of memory on an Intel Kaby Lake i7-7700K test machine, vcube
achieved an average solution rate of 6.0 cubes/second on a set of 10,000
randomly generated cubes.  As a comparison, the original solver found 3.0
solutions/second, and nxopt found 3.8 solutions/second on the same hardware.

## Getting started

### Prerequisites

* Intel Haswell or equivalent processor (requires AVX2, BMI, BMI2, and LZCNT)
* 64-bit Linux (developed on Ubuntu 18.04.1 LTS)
* Clang (developed on 6.0.0)
* CMake (developed on 3.10.2)
* Cpputest (developed on 3.8)

### Building

```
cmake .
make -j
```

## Running the tests

Tests are run automatically during the build, but can also be invoked manually:

```
./tests/check
```

## Using the software

### Selecting and generating the pruning table

The `vc-optimal` solver supports pruning tables (pattern databases) of
varying size.  Larger tables require more memory, but solve cubes faster
than smaller tables.  Run `./vc-optimal --help` to see the list of supported
table coordinates and their memory requirements, and select an appropriate
one for your system.  The software was developed on a 32 GiB machine using
coordinate "308" (22 GiB), so that is the default.  Additional, larger
tables can be enabled by uncommenting them in `src/vc-optimal.cpp`
(however, they need to be tuned with an appropriate base depth which can
be determined experimentally.)

Tables are generated automatically the first time they are used.
For example, to use the 7.3 GiB "208" table:
```
./vc-optimal --coord=208 --no-input
```

### Solving cubes

To solve cubes, run `vc-optimal` with the desired command-line options,
then feed it cubes on standard input, one cube per line.  Several input
formats are supported:

#### Move sequence

Cubes can be specified as a move sequence with the `--format=moves`
option (default).
```
D' F' D' L B' L' D2 F2 L F U' R U F2 L'
D3F3D3L1B3L3D2F2L1F1U3R1U1F2L3
DDDFFFDDDLBBBLLLDDFFLFUUURUFFLLL
```

#### Singmaster positional notation

The `--format=singmaster` option allows specifying the cube sticker by
sticker.

Solved cube:
```
UF UR UB UL DF DR DB DL FR FL BR BL UFR URB UBL ULF DRF DFL DLB DBR
```

Cube within a cube pattern:
```
UF UR FL FD BR BU DB DL FR RD LU BL UFR FUL FLD FDR BUR BRD DLB BLU
```

#### Speffz

The [Speffz](https://www.speedsolving.com/wiki/index.php/Speffz) lettering
scheme is helpful for those familiar with blindfolded solving, and can be
used to input manually cubes quickly.  Speffz input can be chosen either
by `--format=speffz` (which uses the default corner/edge buffers of A/U),
or `--speffz=CE` where C and E are the desired corner and edge buffers.

Example input for the cube-within-cube-within-cube pattern (default buffers):
```
lopbipJS.teloal
```

In the example, `lopbip` is the permutation of corner stickers, `JS` are
corners twisted in place (and is shorthand for `jcsx`), and `teloal` is
the edge permutation.  If there were uppercase edges, they would denote
edges flipped in place.  In-place reorientations affect the buffer in the
opposite direction.  The notation is written as a blindfolded solver might
memorize a cube, so it reads as a list of steps to *solve* the cube.

## Example

This example uses the 22 GiB "308" table and uses the `--ordered` option
to output solutions in the same order as the input.  The solver automatically
spawns one thread per CPU core, in this case 8 threads.

Each output line includes the input line number (0 through 15), real time
spent, number of moves, and solution.  The final output line, which is
printed to standard error, gives the overall amount of real and CPU time
spent solving; this excludes time reading or generating the pruning table.

```
$ head -16 test64.txt | ./vc-optimal -c 308 --ordered
0 4.787566821 18 F' L2 D2 B  U  D2 F2 D' L2 U  F  D  R  U  B' D  R  L
1 0.856710460 18 D  R  U2 F  B2 U  F2 L' F' D  L' U  D2 F' R2 D2 B' U2
2 0.647874028 18 B2 R' U' D' R2 F  D' R2 B2 R' U  F2 B' R' U  R' D2 F'
3 1.939559155 18 U' D  L2 B  D  F2 L' F2 D2 F' R2 U2 D  B  R  L2 D  R'
4 0.759255970 18 R2 U' L' U2 R' L  U' B' U' L  F2 D' L' U  R2 D' F' L'
5 0.086163387 17 R2 F  D2 R' B' U  B  U' B' U2 F  B  R' B2 U2 F2 U2
6 0.988980881 18 R2 U  F2 U  D2 L  U' L2 U' F  R  B  U' R' F  R' F2 L2
7 1.661477776 18 U' L2 F  R' L' F  D  L2 D  L2 U' B2 L2 D' R  F' D2 F
8 0.300311157 17 L' B  R2 B2 U  F2 R  B' R' U  D  L  F2 B  R  F  B
9 1.012389118 18 L' U' D2 R' U  B  R2 F' R  D  L2 D' R2 F2 R2 F' R  B2
10 0.019232801 16 U  F  U  R' D2 F' R2 B  D2 R  F2 R' D2 F  U  R2
11 1.059923421 18 U  L' F  D2 F' U' B2 U  L2 B' U' F' R  F  B2 D  L  D'
12 0.109544175 17 B  D2 L  B' D2 L2 U2 R' D  R' F' L' U  F2 R' D  R
13 4.437426858 19 U' R' D  F' U  B' D' R2 U2 B2 U' R  B2 R2 L' D2 L2 B2 L2
14 0.097405511 17 B  R  B' D  B' L2 U  R  F2 U2 D2 B  D  B  L  B  L'
15 0.064808627 17 R' U2 D2 R  F  L  U' D  L' F' L2 U' F' L' F' D' B2
Total time: 5.294330243 real, 18.806913 cpu, 2.350864 cpu/worker
```

## Tuning your system for speed

### Huge pages

vcube supports 1GB and 2MB huge pages, and will automatically use the
largest page size supported.  Huge pages can be enabled by adding
parameters to the kernel command line.  For example, to use 1GB pages for
the 21.2 GiB "308" table, add the following kernel parameters (on Ubuntu,
the file is `/etc/default/grub`.  The `update-grub` command must be run
afterwards.)
```
hugepagesz=1G hugepages=22
```

*Note: Huge pages allocated at boot time are unavailable for ordinary
processes.  Be sure to leave enough free memory for the rest of your system
to function.*

### Meltdown and Spectre

**WARNING: Disabling security features is dangerous -- do so at your own risk!**

The [Meltdown and Spectre](https://meltdownattack.com/) CPU vulnerabilities
required operating system mitigations that negatively impacted performance.
Some workloads are impacted more than others; on the author's i7-7700K
machine, the vc-optimal solver takes 35% longer to run with mitigations
enabled.

If you are willing to accept the risk, the command line kernel parameter
`nopti` will restore performance.

**WARNING: Disabling security features is dangerous -- do so at your own risk!**

### Overclocking and XMP

Because vcube relies on random access to a large in-memory table, the main
performance bottleneck is memory access latency.  Increasing memory clock
speed and tightening memory timings can improve the speed of the solver.
Memory modules that support XMP (Extreme Memory Profile) makes it easy to
overclock with manufacturer supplied timings.

## Authors

* Andrew Skalski ([Voltara](https://github.com/Voltara) on GitHub)

See also the list of
[contributors](https://github.com/Voltara/vcube/contributors) who
participated in this project.

## License

This project is licensed under the GPLv3 License - see the [LICENSE](LICENSE)
file for details.

## Acknowledgments

Most of what I have learned about Rubik's cube solvers came from the work
of these individuals:
* Herbert Kociemba, [The Mathematics behind Cube Explorer](http://kociemba.org/cube.htm)
* Tomas Rokicki, [nxopt](https://github.com/rokicki/cube20src) and several papers
* Chen Shuang, [min2phase](https://github.com/cs0x7f/min2phase)
