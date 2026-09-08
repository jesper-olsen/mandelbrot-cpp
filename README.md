# Mandelbrot in C++

This repository contains a modern C++ (C++23) implementation for generating visualizations of the Mandelbrot set. It is a direct port of the [mandelbrot-c](https://github.com/jesper-olsen/mandelbrot-c) reference implementation, part of a [cross-language comparison project](https://github.com/jesper-olsen/mandelbrot-c#other-language-implementations).

The program compiles to a single native executable. It can render the Mandelbrot set directly to the terminal as ASCII art or produce a data file for `gnuplot` to generate a high-resolution PNG image.

## Differences from the C reference

The escape-time algorithm and ASCII/gnuplot output formats are unchanged and verified byte-identical to `mandelbrot.c` across ASCII, gnuplot-text, and 5000x5000 renders. The differences are in the surrounding scaffolding:

- **Argument parsing** (`parse_arg`) uses `std::string_view` and `std::from_chars` instead of `strchr`/`strcmp`/`atoi`/`atof`. `argv` is never mutated (the C version temporarily writes a `'\0'` into the argument string and restores it afterwards).
- **Row serialization** (`gptext_output`) uses `std::to_chars` instead of a hand-rolled 3-digit itoa. This incidentally fixes a real bug: the C version's itoa assumes `iter` never exceeds 3 digits, so `max_iter >= 1000` silently renders garbage characters (e.g. `1491` comes out as `>91`). Confirmed via direct comparison; `std::to_chars` has no such limit.
- **Invalid numeric input behaves differently**: `atoi`/`atof` return `0` on unparseable input, silently zeroing the field (e.g. `width=abc` collapses `width` to `0`, producing degenerate output). `std::from_chars` leaves the target variable untouched on failure, so a bad value falls back to whatever the field held before (its default, if unset elsewhere) rather than zero. Worth knowing if any script relies on the C behavior.
- `Config` uses default member initializers instead of a C99 designated-initializer literal in `main`.
- Pointer parameters (`const Config *`) became reference parameters (`const Config&`).

Hot-loop I/O (`putchar`, `fwrite`) is unchanged — `std::cout`/`std::print` don't fit a per-pixel loop and would only add overhead here.

## Prerequisites

You will need the following installed:

1. A **C++ compiler** with C++23 support (Apple Clang 17+, Clang 16+, or GCC 13+).
2. **Make** (optional, but recommended for easy building).
3. **Gnuplot** (required *only* for generating PNG images).

## Build

You can compile the program directly or use the provided Makefile.

**Option 1: Manual Compilation**

```
clang++ -std=c++23 -O3 -o mandelbrot mandelbrot.cpp
```

**Option 2: Using Make**

```
make
```

## Usage

The compiled executable can be configured via command-line arguments using a `key=value` format.

### 1. ASCII Art Output

To render the Mandelbrot set directly in your terminal, run the executable.

```
./mandelbrot
```

You can change the view and resolution by passing parameters:

```
# Zoom in on a different area with a wider view
./mandelbrot width=120 ll_x=-0.75 ll_y=0.1 ur_x=-0.74 ur_y=0.11
```

### 2. PNG Image Generation

To create a high-resolution PNG, you first generate a data file and then process it with `gnuplot`.

**Step 1: Generate the data file**

```
./mandelbrot png=1 width=1000 height=750 > image.dat
```

**Step 2: Run gnuplot** (reuse `topng.gp` from [mandelbrot-c](https://github.com/jesper-olsen/mandelbrot-c) unchanged — the data format is identical)

```
gnuplot topng.gp
```

## Performance

**Generating a 1000x750 data file:**
```sh
time ./mandelbrot png=1 width=1000 height=750 > image.dat
0.14s user 0.01s system 97% cpu 0.148 total
```

**Generating a 5000x5000 data file:**
```sh
time ./mandelbrot png=1 width=5000 height=5000 > image.dat
3.59s user 0.04s system 99% cpu 3.652 total
```

#**Generating a 5000x5000 data file multiple worker threads**
#```sh
#time ./mandelbrot_pthread  png=1 width=5000 height=5000 > image.dat
#5.11s user 0.06s system 884% cpu 0.584 total
#```
#
#**Generating a 5000x5000 data file with SIMD and multiple worker threads:**
#```sh
#time ./mandelbrot_simd_pthread_v8 png=1 width=5000 height=5000 > image.dat
#0.95s user 0.05s system 631% cpu 0.160 total
#```
#**Generating a 20000x20000 data file with SIMD and multiple worker threads:**
#```sh
#time ./mandelbrot_simd_pthread_v8 png=1 width=20000 height=20000 > image.dat
#13.51s user 0.47s system 679% cpu 2.057 total
#```



