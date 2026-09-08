/**
 * @file mandelbrot_thread.cpp
 * @brief Generates Mandelbrot set visualizations in ASCII or for gnuplot,
 *        using a pool of worker threads.
 *
 * A modern C++ (C++23) port of mandelbrot_pthread.c for a cross-language
 * comparison project. Renamed from "_pthread" to "_thread" since it no
 * longer uses pthreads directly - std::jthread (backed by the platform's
 * native threads) replaces pthread_create/pthread_join, and std::atomic<int>
 * replaces stdatomic.h's atomic_int for the same dynamic row-stealing scheme.
 *
 * Compilation:
 *   clang++ -std=c++23 -O3 -o mandelbrot_thread mandelbrot_thread.cpp
 *
 * Usage:
 *   ./mandelbrot_thread
 *   ./mandelbrot_thread width=120 ll_x=-0.75 ll_y=0.1 ur_x=-0.74 ur_y=0.11
 *   ./mandelbrot_thread png=1 width=800 height=600 > mandelbrot.dat
 */

#include <cstdio>
#include <cstdlib>
#include <string_view>
#include <charconv>
#include <vector>
#include <thread>
#include <atomic>
#include <algorithm>

//constexpr unsigned int NUM_THREADS = 9;  // Adjust based on CPU core count
static unsigned int NUM_THREADS = std::thread::hardware_concurrency();  // M5: 10
constexpr int CHUNK_SIZE = 1;   // Number of rows to process per task

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

/**
 * @brief Worker loop: pulls the next unclaimed row from next_y and fills
 *        it into buffer until the image is exhausted.
 *
 * Unlike mandelbrot_pthread.c's thread_mandelbrot, there's no ThreadArgs
 * struct to marshal arguments through a void* - the lambda just captures
 * what it needs by reference. next_y is passed in rather than a
 * translation-unit-global atomic, so there's no shared mutable state
 * outside of what's explicitly wired up in main.
 */
void worker(const Config& config, std::atomic<int>& next_y, std::vector<int>& buffer)
{
        double fwidth = config.ur_x - config.ll_x;
        double fheight = config.ur_y - config.ll_y;

        for (;;) {
                int y_start = next_y.fetch_add(CHUNK_SIZE);
                if (y_start >= config.height) {
                        break;
                }
                int y_end = std::min(y_start + CHUNK_SIZE, config.height);

                for (int y = y_start; y < y_end; ++y) {
                        for (int x = 0; x < config.width; ++x) {
                                double real = config.ll_x + x * fwidth / config.width;
                                double imag = config.ur_y - y * fheight / config.height;
                                int iter = escape_time(real, imag, config.max_iter);
                                buffer[y * config.width + x] = iter;
                        }
                }
        }
}

/**
 * @brief Writes the completed image to stdout, as ASCII art or gnuplot text.
 */
void final_output(const Config& config, const std::vector<int>& result)
{
        // Buffer one row of text data; std::to_chars writes directly into it -
        // no hardcoded digit-count assumption (the C version's manual itoa
        // silently mis-renders max_iter >= 1000).
        std::vector<char> buffer(static_cast<size_t>(config.width) * 6 + 64);

        if (config.png) {
                for (int y = config.height - 1; y >= 0; --y) {
                        char* ptr = buffer.data();
                        char* const end = buffer.data() + buffer.size();
                        const int* row = &result[static_cast<size_t>(y) * config.width];

                        for (int x = 0; x < config.width; ++x) {
                                if (x > 0) {
                                        *ptr++ = ',';
                                        *ptr++ = ' ';
                                }
                                auto[next, ec] = std::to_chars(ptr, end, row[x]);
                                ptr = next;
                        }
                        *ptr++ = '\n';
                        std::fwrite(buffer.data(), 1, ptr - buffer.data(), stdout);
                }
        } else {
                for (int y = 0; y < config.height; ++y) {
                        char* ptr = buffer.data();
                        const int* row = &result[static_cast<size_t>(y) * config.width];
                        for (int x = 0; x < config.width; ++x) {
                                *ptr++ = cnt2char(row[x], config.max_iter);
                        }
                        *ptr++ = '\n';
                        std::fwrite(buffer.data(), 1, ptr - buffer.data(), stdout);
                }
        }
}

int main(int argc, char* argv[])
{
        Config config;

        for (int i = 1; i < argc; ++i) {
                parse_arg(argv[i], config);
        }

        std::vector<int> result(static_cast<size_t>(config.width) * config.height);
        std::atomic<int> next_y{0};

        {
                // jthreads auto-join when this vector goes out of scope - no
                // separate pthread_join loop, and no way to accidentally forget one.
                std::vector<std::jthread> workers;
                workers.reserve(NUM_THREADS);
                for (unsigned int i = 0; i < NUM_THREADS; ++i) {
                        workers.emplace_back(worker, std::cref(config), std::ref(next_y), std::ref(result));
                }
        }

        final_output(config, result);
        return EXIT_SUCCESS;
}
