/**
 * @file mandelbrot.cpp
 * @brief Generates Mandelbrot set visualizations in ASCII or for gnuplot.
 *
 * A modern C++ implementation for a cross-language comparison project.
 * Direct port of mandelbrot.c, but using C++ facilities (std::string_view,
 * std::from_chars/std::to_chars) instead of the C string/parsing idioms.
 * Parses command-line arguments in the format key=value.
 *
 * Compilation:
 *   clang++ -std=c++23 -O3 -o mandelbrot mandelbrot.cpp
 *
 * Usage:
 *   ./mandelbrot
 *   ./mandelbrot width=120 ll_x=-0.75 ll_y=0.1 ur_x=-0.74 ur_y=0.11
 *   ./mandelbrot png=1 width=800 height=600 > mandelbrot.dat
 */

#include <cstdio>
#include <cstdlib>
#include <string_view>
#include <charconv>
#include <array>

struct Config {
        int width = 100;
        int height = 75;
        bool png = false;
        double ll_x = -1.2;
        double ll_y = 0.20;
        double ur_x = -1.0;
        double ur_y = 0.35;
        int max_iter = 255;
};

/**
 * @brief Maps an iteration count to an ASCII character.
 * @param value The iteration value (0 to max_iter).
 * @param max_iter The maximum number of iterations.
 * @return A character for visualization.
 */
char cnt2char(int value, int max_iter)
{
        constexpr std::string_view symbols = "MW2a_. ";
        int idx = static_cast<int>(static_cast<double>(value) / max_iter * (symbols.size() - 1));
        return symbols[idx];
}

/**
 * @brief Calculates the escape time for a point in the complex plane.
 * @param cr The real part of the complex number c.
 * @param ci The imaginary part of the complex number c.
 * @param max_iter The maximum number of iterations.
 * @return An integer representing how close the point is to the set.
 */
int escape_time(double cr, double ci, int max_iter)
{
        double zr = 0.0, zi = 0.0;
        int iter;
        for (iter = 0; iter < max_iter; ++iter) {
                double zr2 = zr * zr;
                double zi2 = zi * zi;
                if (zr2 + zi2 > 4.0) {
                        break;
                }
                double tmp = zr2 - zi2 + cr;
                zi = 2.0 * zr * zi + ci;
                zr = tmp;
        }
        return max_iter - iter;
}

/**
 * @brief Renders the Mandelbrot set as ASCII art to stdout.
 * @param config The configuration struct.
 */
void ascii_output(const Config& config)
{
        double fwidth = config.ur_x - config.ll_x;
        double fheight = config.ur_y - config.ll_y;
        for (int y = 0; y < config.height; ++y) {
                for (int x = 0; x < config.width; ++x) {
                        double real = config.ll_x + x * fwidth / config.width;
                        double imag = config.ur_y - y * fheight / config.height;
                        int iter = escape_time(real, imag, config.max_iter);
                        putchar(cnt2char(iter, config.max_iter));
                }
                putchar('\n');
        }
}

/**
 * @brief Generates text output suitable for gnuplot to stdout.
 * @param config The configuration struct.
 */
void gptext_output(const Config& config)
{
        double fwidth = config.ur_x - config.ll_x;
        double fheight = config.ur_y - config.ll_y;

        // Buffer one row of text data. std::to_chars writes directly into it -
        // no per-pixel sprintf/printf, and no hardcoded digit-count assumption
        // (the C version's hand-rolled itoa silently mis-renders max_iter >= 1000).
        std::array<char, 65536> buffer;

        for (int y = config.height - 1; y >= 0; --y) {
                char* ptr = buffer.data();
                char* const end = buffer.data() + buffer.size();
                for (int x = 0; x < config.width; ++x) {
                        double real = config.ll_x + x * fwidth / config.width;
                        double imag = config.ur_y - y * fheight / config.height;
                        int iter = escape_time(real, imag, config.max_iter);

                        if (x > 0) {
                                *ptr++ = ',';
                                *ptr++ = ' ';
                        }
                        auto[next, ec] = std::to_chars(ptr, end, iter);
                        ptr = next;

                        // Safety check: flush buffer if it's getting full
                        if (end - ptr < 32) {
                                std::fwrite(buffer.data(), 1, ptr - buffer.data(), stdout);
                                ptr = buffer.data();
                        }
                }
                *ptr++ = '\n';
                std::fwrite(buffer.data(), 1, ptr - buffer.data(), stdout);
        }
}

/**
 * @brief Parses a single "key=value" command-line argument.
 * @param arg The argument string, as a view (argv is never mutated).
 * @param config The configuration struct to update.
 */
void parse_arg(std::string_view arg, Config& config)
{
        auto eq = arg.find('=');
        if (eq == std::string_view::npos) {
                std::fprintf(stderr, "Warning: Ignoring invalid argument '%.*s'\n",
                             static_cast<int>(arg.size()), arg.data());
                return;
        }

        std::string_view key = arg.substr(0, eq);
        std::string_view val = arg.substr(eq + 1);
        const char* vbeg = val.data();
        const char* vend = val.data() + val.size();

        if (key == "width") std::from_chars(vbeg, vend, config.width);
        else if (key == "height") std::from_chars(vbeg, vend, config.height);
        else if (key == "png") {
                int v = 0;
                std::from_chars(vbeg, vend, v);
                config.png = static_cast<bool>(v);
        } else if (key == "ll_x") std::from_chars(vbeg, vend, config.ll_x);
        else if (key == "ll_y") std::from_chars(vbeg, vend, config.ll_y);
        else if (key == "ur_x") std::from_chars(vbeg, vend, config.ur_x);
        else if (key == "ur_y") std::from_chars(vbeg, vend, config.ur_y);
        else if (key == "max_iter") std::from_chars(vbeg, vend, config.max_iter);
        else std::fprintf(stderr, "Warning: Unknown parameter '%.*s'\n",
                                  static_cast<int>(key.size()), key.data());
}

int main(int argc, char* argv[])
{
        Config config;

        for (int i = 1; i < argc; ++i) {
                parse_arg(argv[i], config);
        }

        if (config.png) {
                gptext_output(config);
        } else {
                ascii_output(config);
        }
        return EXIT_SUCCESS;
}
