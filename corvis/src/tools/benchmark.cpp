#include "benchmark.h"
#include "for_each_file.h"

#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <sstream>

template <typename T, bool add_front = false, bool add_back = false>
struct histogram { // follows the semantics of numpy.histogram
public:
    std::vector<T> edges;
    std::vector<size_t> bins; // invariant: edges.size() + 1 == bin.size()

    histogram(const std::vector<T> &data, const std::vector<T> &bin_edges) {
        if (bin_edges.size() < 2)
            return;

        edges = bin_edges;

        if ((add_front || add_back) && data.size()) {
            auto minmax = std::minmax_element(data.begin(), data.end());
            if (add_front && *minmax.first < bin_edges.front())
                edges.insert(edges.begin(), *minmax.first);
            if (add_back && *minmax.second > bin_edges.back())
                edges.push_back(*minmax.second);
        }

        bins = std::vector<size_t>(edges.size()-1, 0);

        for (auto &d : data) { // FIXME: makes this O(log(n)) instead of O(n) ?
            int i; for (i=0; i < bins.size()-1; i++)
                if (edges[i] <= d && d < edges[i+1])
                    bins[i]++;
            if (edges[i] <= d && d <= edges[i+1]) // last bin is closed
                bins[i]++;
        }
    }
};

template<typename T, bool add_front, bool add_back>
static inline std::ostream& operator<<(std::ostream &stream, const histogram<T, add_front, add_back> &h)
{
    stream << std::fixed << std::setprecision(1);
    for (int i=0; i<h.bins.size(); i++)
        stream << (i ? "\t" : "") << h.edges[i] << "%";
    stream << (add_back ? "+" : "") << "\n";
    for (int i=0; i<h.bins.size(); i++)
        stream << (i ? "\t" : "") << h.bins[i];
    return stream << "\n";
}

#include <iostream>
#include <iomanip>
struct measurement {
    double L, L_measured;
    double error, error_percent;
    measurement(double L_, double L_measured_) : L(L_), L_measured(L_measured_) {
        error = std::abs(L_measured - L);
        error_percent = L < 1 ? error : 100 * error / L;
    }
    std::string to_string() {
        std::stringstream ss; ss << std::fixed << std::setprecision(2);
        ss << L << "cm actual, " << L_measured << "cm measured, " << error << "cm error (" << error_percent << "%)";
        return ss.str();
    }
};

#include <future>
#include <thread>
#include <algorithm>

void benchmark_run(std::ostream& stream, const char *directory, std::function<bool (const char *capture_file, struct benchmark_result &result)> measure_file) {
    std::vector<std::string> files;
    for_each_file(directory, [&files](const char *file) {
        if (0 == strstr(file, ".json") && 0 == strstr(file, ".pose"))
            files.push_back(file);
    });
    std::sort(files.begin(), files.end());

    struct benchmark_data { std::string basename, file; struct benchmark_result result; std::future<bool> ok; };
    std::vector<benchmark_data> results; results.reserve(files.size()); // avoid reallocing below

    int first=0, last=0; // a poor man's thread pool (works best with fixed size units of work)
    for (auto &file : files) {
        results.push_back(benchmark_data { file.substr(strlen(directory) + 1), file });
        results.back().ok = std::async(std::launch::async, measure_file, results.back().file.c_str(), std::ref(results.back().result));
        if ((++last - first) >= std::max<signed>(1, std::thread::hardware_concurrency()))
          results[first++].ok.wait();
    }

    std::vector<double> L_errors_percent, PL_errors_percent, primary_errors_percent;

    for (auto &bm : results) {
        if (!bm.ok.get()) {
            stream << "FAILED " << bm.basename << "\n";
            continue;
        } else
            stream << "Result " << bm.basename << "\n";

        auto &r = bm.result;

        double L  = r.length_cm.measured,      base_L  = r.length_cm.reference;
        double PL = r.path_length_cm.measured, base_PL = r.path_length_cm.reference;

        // FIXME: Backward comapt since we used to read the output of %.2f, but could be removed once we delete benchmark.py
        /**/ L = round(     L * 100) / 100;      PL = round(     PL * 100) / 100;
        base_L = round(base_L * 100) / 100; base_PL = round(base_PL * 100) / 100;

        bool has_PL = !isnan(r.path_length_cm.reference), has_L = !isnan(r.length_cm.reference);
        if (!has_L)
            base_L = 0.;
        if (!has_PL)
            base_PL = PL;
        if (base_PL == 0.)
            base_PL = 1.;

        measurement mL(base_L, L), mPL(base_PL, PL);
        if (has_L) {
            stream << "\t" << "L" << "\t" << mL.to_string() << "\n";
            L_errors_percent.push_back(mL.error_percent);
            if (has_PL || (PL > 5 && base_L <= 5)) {
                // either explicitly closed the loop, or an implicit loop closure using measured PL as loop length
                double loop_close_error_percent = 100. * std::abs(L - base_L) / base_PL;
                stream << "\t" << "Loop closure error: " << std::fixed << std::setprecision(2) << loop_close_error_percent << "%\n";
                primary_errors_percent.push_back(loop_close_error_percent);
            } else if (base_L > 5)
                primary_errors_percent.push_back(mL.error_percent);
            else
                primary_errors_percent.push_back(mL.error);
        } else
            primary_errors_percent.push_back(mPL.error_percent);

        if (has_PL) {
            stream << "\tPL\t" << mPL.to_string();
            PL_errors_percent.push_back(mPL.error_percent);
        }
    }

    std::vector<double> std_edges = {0, 3, 10, 25, 50, 100};
    std::vector<double> alt_edges = {0, 4, 12, 30, 65, 100};
    typedef histogram<double, false, true> error_histogram;

    stream << "Length error histogram (" << L_errors_percent.size() << " sequences)\n";
    stream << error_histogram(L_errors_percent, std_edges) << "\n";

    stream << "Path length error histogram (" << PL_errors_percent.size() << " sequences)\n";
    stream << error_histogram(PL_errors_percent, std_edges) << "\n";

    stream << "Primary error histogram (" << primary_errors_percent.size() << " sequences)\n";
    error_histogram pri_hist(primary_errors_percent, std_edges);
    stream << pri_hist << "\n";

    stream << "Alternate error histogram (" << primary_errors_percent.size() << " sequences)\n";
    error_histogram alt_hist(primary_errors_percent, alt_edges);
    stream << alt_hist << "\n";

    struct stat { size_t n; double sum, mean; } pe_le50 = {0, 0, 0};
    for(auto &pe : primary_errors_percent) if (pe < 50) { pe_le50.n++; pe_le50.sum += pe; }
    pe_le50.mean = pe_le50.sum / pe_le50.n;

    stream << std::fixed << std::setprecision(2);
    stream << "Mean of " << pe_le50.n << " primary errors that are less than 50% is " << pe_le50.mean << "%\n";

    int score = 0;
    for (size_t i=0; i < pri_hist.bins.size(); i++)
        score += i * pri_hist.bins[i];

    int altscore = 0;
    for (size_t i=0; i < alt_hist.bins.size(); i++)
        altscore += i * alt_hist.bins[i];

    stream << "Histogram score (lower is better) is " << score << "\n";
    stream << "Alternate histogram score (lower is better) is " << altscore << "\n";
}
