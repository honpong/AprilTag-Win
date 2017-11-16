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
    int output_precision;
    std::string bin_units;

    histogram(const std::vector<T> &data, const std::vector<T> &bin_edges, int precision = 1, std::string units = "%") :
        output_precision(precision), bin_units(units)
    {
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
            size_t i; for (i=0; i < bins.size()-1; i++)
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
    stream << std::fixed << std::setprecision(h.output_precision);
    if (add_back) {
        for (size_t i=0; i<h.bins.size(); i++)
            stream << (i ? "\t" : "") << h.edges[i] << h.bin_units;
        stream << (add_back ? "+" : "") << "\n";
        for (size_t i=0; i<h.bins.size(); i++)
            stream << (i ? "\t" : "") << h.bins[i];
    }
    if (add_front) {
        for (int i=h.bins.size(); i>0; i--)
            stream << (i<h.bins.size() ? "\t" : "") << h.edges[i] << h.bin_units;
        stream <<  "-" << "\n";
        for (int i=h.bins.size()-1; i>=0; i--)
            stream << (i<h.bins.size()-1 ? "\t" : "") << h.bins[i];
    }
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
#include <string.h>

void benchmark_run(std::ostream &stream, const char *directory, int threads,
        std::function<bool (const char *file, struct benchmark_result &result)> measure_file,
        std::function<void (const char *file, struct benchmark_result &result)> measure_done)
{
    std::vector<std::string> files;
    for_each_file(directory, [&files](const char *file) {
        if (0 == strstr(file, ".json") && 0 == strstr(file, ".pose") && 0 == strstr(file, ".vicon") && 0 == strstr(file, ".tum") && 0 == strstr(file,".loop"))
            files.push_back(file);
    });
    std::sort(files.begin(), files.end());

    struct benchmark_data { std::string basename, file; struct benchmark_result result; std::future<bool> ok; };
    std::vector<benchmark_data> results; results.reserve(files.size()); // avoid reallocing below

    int first=0, last=0; // a poor man's thread pool (works best with fixed size units of work)
    for (auto &file : files) {
        results.push_back(benchmark_data { file.substr(strlen(directory) + 1), file });
        results.back().ok = std::async(std::launch::async, measure_file, results.back().file.c_str(), std::ref(results.back().result));
        if ((++last - first) >= std::max<signed>(1, threads ? threads : std::thread::hardware_concurrency())) {
          results[first].ok.wait();
          measure_done(results[first].file.c_str(), results[first].result);
          first++;
        }
    }
    while (first < last) {
        results[first].ok.wait();
        measure_done(results[first].file.c_str(), results[first].result);
        first++;
    }

    std::vector<double> L_errors_percent, PL_errors_percent, primary_errors_percent, ate_errors_m, rpe_T_errors_m, rpe_R_errors_deg, reloc_rpe_T_errors_m, reloc_rpe_R_errors_deg, precision_reloc, recall_reloc;
    uint32_t precision_anomalies = 0, recall_anomalies = 0;
    bool has_reloc = false;

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

        bool has_L = !std::isnan(r.length_cm.reference), has_PL = !std::isnan(r.path_length_cm.reference);
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
            stream << "\tPL\t" << mPL.to_string() << "\n";
            PL_errors_percent.push_back(mPL.error_percent);
        }

        if (r.errors.calculate_ate()) {
            stream << "\tATE\t" << r.errors.ate.rmse << "m\n";
            ate_errors_m.push_back(r.errors.ate.rmse);
            stream << "\tTranslational RPE\t" << r.errors.rpe_T.rmse << "m\n";
            rpe_T_errors_m.push_back(r.errors.rpe_T.rmse);
            stream << "\tRotational RPE\t" << r.errors.rpe_R.rmse*(180.f/M_PI) << "deg\n";
            rpe_R_errors_deg.push_back(r.errors.rpe_R.rmse*(180.f/M_PI));
        }
        if (r.errors.calculate_precision_recall()) {
            has_reloc = true;
            stream << "\tRelocalization Precision\t" << r.errors.relocalization.precision*100 << "%\n";
            if (!std::isnan(r.errors.relocalization.precision))
                precision_reloc.push_back(r.errors.relocalization.precision*100);
            else
                precision_anomalies++;
            stream << "\t               Recall\t" << r.errors.relocalization.recall*100 << "%\n";
            if (!std::isnan(r.errors.relocalization.recall))
                recall_reloc.push_back(r.errors.relocalization.recall*100);
            else
                recall_anomalies++;
            if (!std::isnan(r.errors.reloc_rpe_T.rmse)) {
                stream << "\t               Translational RPE\t" << r.errors.reloc_rpe_T.rmse << "m\n";
                reloc_rpe_T_errors_m.push_back(r.errors.reloc_rpe_T.rmse);
            }
            if (!std::isnan(r.errors.reloc_rpe_R.rmse)) {
                stream << "\t               Rotational RPE\t" << r.errors.reloc_rpe_R.rmse*(180.f/M_PI) << "deg\n";
                reloc_rpe_R_errors_deg.push_back(r.errors.reloc_rpe_R.rmse*(180.f/M_PI));
            }
        }
    }

    std::vector<double> std_edges = {0, 3, 10, 25, 50, 100};
    std::vector<double> alt_edges = {0, 4, 12, 30, 65, 100};
    std::vector<double> ate_edges = {0, 0.01, 0.05, 0.1, 0.5, 1};
    std::vector<double> rpe_T_edges = {0, 0.01, 0.05, 0.1, 0.5, 1};
    std::vector<double> rpe_R_edges = {0, 0.05, 0.1, 0.5, 1, 5};
    std::vector<double> precision_edges = {0, 60, 80, 95, 99, 100};
    std::vector<double> recall_edges = {0, 2, 5, 10, 20, 50};
    std::vector<double> reloc_rpe_T_edges = {0, 0.01, 0.05, 0.1, 0.5, 1};
    std::vector<double> reloc_rpe_R_edges = {0, 0.05, 0.1, 0.5, 1, 5};
    typedef histogram<double, false, true> error_histogram;
    typedef histogram<double, true, false> error_histogram_pr;

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

    stream << "ATE histogram (" << ate_errors_m.size() << " sequences)\n";
    error_histogram ate_hist(ate_errors_m, ate_edges, 2, "m");
    stream << ate_hist << "\n";

    stream << "RPE (Translation) histogram (" << rpe_T_errors_m.size() << " sequences)\n";
    error_histogram rpe_T_hist(rpe_T_errors_m, rpe_T_edges, 2, "m");
    stream << rpe_T_hist << "\n";

    stream << "RPE (Rotation) histogram (" << rpe_R_errors_deg.size() << " sequences)\n";
    error_histogram rpe_R_hist(rpe_R_errors_deg, rpe_R_edges, 2, "deg");
    stream << rpe_R_hist << "\n";

    if (has_reloc) {
        stream << "Precision histogram (" << precision_reloc.size() << " sequences)\n";
        error_histogram_pr precision_hist(precision_reloc, precision_edges, 2);
        stream << precision_hist;
        stream << "Undefined precision: " << precision_anomalies << " sequences\n\n";

        stream << "Recall histogram (" << recall_reloc.size() << " sequences)\n";
        error_histogram_pr recall_hist(recall_reloc, recall_edges, 2);
        stream << recall_hist;
        stream << "Undefined recall: " << recall_anomalies << " sequences\n\n";

        stream << "Relocalization RPE (Translation) histogram (" << reloc_rpe_T_errors_m.size() << " sequences)\n";
        error_histogram reloc_rpe_T_hist(reloc_rpe_T_errors_m, reloc_rpe_T_edges, 2, "m");
        stream << reloc_rpe_T_hist << "\n";

        stream << "Relocalization RPE (Rotation) histogram (" << reloc_rpe_R_errors_deg.size() << " sequences)\n";
        error_histogram reloc_rpe_R_hist(reloc_rpe_R_errors_deg, reloc_rpe_R_edges, 2, "deg");
        stream << reloc_rpe_R_hist << "\n";
    }

    struct stat { size_t n; double sum, mean, median; } pe_le50 = {0, 0, 0, 0};
    std::sort(primary_errors_percent.begin(), primary_errors_percent.end());
    for(auto &pe : primary_errors_percent) if (pe < 50) { pe_le50.n++; pe_le50.sum += pe; }
    pe_le50.mean = pe_le50.sum / pe_le50.n;
    pe_le50.median = primary_errors_percent[primary_errors_percent.size()/2];

    stream << std::fixed << std::setprecision(2);
    stream << "Mean of " << pe_le50.n << " primary errors that are less than 50% is " << pe_le50.mean << "% and the median of all errors is " << pe_le50.median << "%\n";

    int score = 0;
    for (size_t i=0; i < pri_hist.bins.size(); i++)
        score += i * pri_hist.bins[i];

    int altscore = 0;
    for (size_t i=0; i < alt_hist.bins.size(); i++)
        altscore += i * alt_hist.bins[i];

    stream << "Histogram score (lower is better) is " << score << "\n";
    stream << "Alternate histogram score (lower is better) is " << altscore << "\n";
}
