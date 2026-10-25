// Dev-only check: runs the C++ MDX pipeline and writes the result as raw planar float32.
#include "../Source/stems/MdxModel.h"
#include <chrono>
#include <cstdio>
#include <fstream>
#include <vector>
using namespace bony;
int main (int argc, char** argv)
{
    if (argc < 8) { std::puts ("usage: stem_test model.onnx in.f32 out.f32 nFft dimF dimT numSamples"); return 1; }
    MdxSpec s { "x", atoi (argv[4]), atoi (argv[5]), atoi (argv[6]), 1.0f };
    size_t n = (size_t) atoll (argv[7]);
    std::vector<float> in (n * 2);
    { std::ifstream f (argv[2], std::ios::binary); f.read ((char*) in.data(), (std::streamsize) (in.size() * 4)); }
    auto t0 = std::chrono::steady_clock::now();
    MdxModel m (argv[1], s, 2);
    std::vector<float> L, R;
    m.separate (in.data(), in.data() + n, n, L, R, [] (double p) { std::fprintf (stderr, "\r%.0f%%", p * 100); return true; });
    double sec = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
    std::fprintf (stderr, "\ntime %.2fs for %.1fs audio\n", sec, n / 44100.0);
    std::ofstream o (argv[3], std::ios::binary);
    o.write ((char*) L.data(), (std::streamsize) (n * 4)); o.write ((char*) R.data(), (std::streamsize) (n * 4));
}
