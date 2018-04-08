#include "benchmark.h"
#include "for_each_file.h"

#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <sstream>
#ifdef _MSC_VER
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

template <typename T, bool add_front = false, bool add_back = false>
struct histogram { // follows the semantics of numpy.histogram
public:
    std::vector<T> edges;
    std::vector<size_t> bins; // invariant: edges.size() + 1 == bin.size()
    int output_precision;
    std::string bin_units;
    T inlier_sum = 0;
    size_t inlier_count = 0;
    T inlier_threshold;

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

        if(add_back)
            inlier_threshold = bin_edges[bin_edges.size()-2];
        else if(add_front)
            inlier_threshold = bin_edges[1];

        for (auto &d : data) { // FIXME: makes this O(log(n)) instead of O(n) ?
            if((add_back && d < inlier_threshold) ||
               (add_front && d > inlier_threshold)) {
                inlier_sum += d;
                inlier_count++;
            }
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
        for (ssize_t i=h.bins.size(); i>0; i--)
            stream << (i<h.bins.size() ? "\t" : "") << h.edges[i] << h.bin_units;
        stream <<  "-" << "\n";
        for (ssize_t i=h.bins.size()-1; i>=0; i--)
            stream << (i<h.bins.size()-1 ? "\t" : "") << h.bins[i];
    }
    size_t score = 0;
    for (size_t i=0; i < h.bins.size(); i++)
        score += i * h.bins[i];
    stream << "\nHistogram score (lower is better): " << score << "\n";
    stream << std::fixed << std::setprecision(h.output_precision+1);
    char inlier_char = add_back ? '<' : '>';
    stream << "Mean of " << h.inlier_count << " inliers (" << inlier_char << h.inlier_threshold << h.bin_units << ") is " << h.inlier_sum/h.inlier_count << h.bin_units;
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
        if (0 == strstr(file, ".json") && 0 == strstr(file, ".pose") && 0 == strstr(file, ".vicon") && 0 == strstr(file, ".tum") && 0 == strstr(file,".loop") && 0 == strstr(file, ".txt") && 0 == strstr(file, ".csv") && 0 == strstr(file, ".jpg"))
            files.push_back(file);
    });
    std::sort(files.begin(), files.end());

    struct benchmark_data { std::string basename, file; struct benchmark_result result; std::future<bool> ok; };

    threads = std::max<signed>(1, threads ? threads : std::thread::hardware_concurrency());
    std::cerr << "Using " << threads << " threads.\n";
    std::list<benchmark_data> thread_pool;
    std::vector<benchmark_data> results; results.reserve(files.size()); // avoid reallocing below

    for (auto &file : files) {
        std::cerr << "Launching " << file << "\n";
        thread_pool.emplace_back(benchmark_data { file.substr(strlen(directory) + 1), file });
        thread_pool.back().ok = std::async(std::launch::async, measure_file, thread_pool.back().file.c_str(), std::ref(thread_pool.back().result));
        while(thread_pool.size() >= threads) {
            for(auto i = thread_pool.begin(); i != thread_pool.end();) {
                if(i->ok.wait_for(std::chrono::milliseconds(100)) == std::future_status::ready) {
                    std::cerr << "Finished " << i->file << "\n";
                    measure_done(i->file.c_str(), i->result);
                    results.emplace_back(std::move(*i));
                    i = thread_pool.erase(i);
                } else ++i;
            }
        }
    }
    for(auto i = thread_pool.begin(); i != thread_pool.end();) {
        i->ok.wait();
        measure_done(i->file.c_str(), i->result);
        results.emplace_back(std::move(*i));
        i = thread_pool.erase(i);
    }

    std::vector<double> L_errors_percent, PL_errors_percent, primary_errors_percent, ate_errors_m, ate_60s_errors_m, ate_600ms_errors_m, rpe_T_errors_m, rpe_R_errors_deg,
            reloc_rpe_T_errors_m, reloc_rpe_R_errors_deg, precision_reloc, recall_reloc, reloc_times_sec;
    uint32_t precision_anomalies = 0, recall_anomalies = 0;
    bool has_reloc = false;

    std::sort(results.begin(), results.end(), [](const benchmark_data &lhs, const benchmark_data &rhs) {
        return lhs.basename < rhs.basename;
    });

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
        }
        if(r.errors.calculate_rpe_600ms()) {
            auto prec = stream.precision(); stream.precision(prec+1); // 600ms is so small, we need an extra digit
            stream << "\tTranslational RPE (600ms)\t" << r.errors.rpe_T.rmse << "m\n";
            rpe_T_errors_m.push_back(r.errors.rpe_T.rmse);
            stream << "\tRotational RPE (600ms)\t" << r.errors.rpe_R.rmse*(180.f/M_PI) << "deg\n";
            rpe_R_errors_deg.push_back(r.errors.rpe_R.rmse*(180.f/M_PI));
            stream.precision(prec);
        }
        if(r.errors.calculate_ate_60s()) {
            stream << "\tATE (60s)\t" << r.errors.ate_60s.rmse << "m\n";
            ate_60s_errors_m.push_back(r.errors.ate_60s.rmse);
        }
        if(r.errors.calculate_ate_600ms()) {
            auto prec = stream.precision(); stream.precision(prec+1); // 600ms is so small, we need an extra digit
            stream << "\tATE (600ms)\t" << r.errors.ate_600ms.rmse << "m\n";
            ate_600ms_errors_m.push_back(r.errors.ate_600ms.rmse);
            stream.precision(prec);
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
            stream << "\t               Correct/Detected\t" << r.errors.relocalization.true_positives << "/" << r.errors.relocalization.true_positives + r.errors.relocalization.false_positives << "\n";
            stream << "\t               Missed\t" << r.errors.relocalization.false_negatives << "\n";
            if (!std::isnan(r.errors.reloc_rpe_T.rmse)) {
                stream << "\t               Translational RPE\t" << r.errors.reloc_rpe_T.rmse << "m\n";
                reloc_rpe_T_errors_m.insert(reloc_rpe_T_errors_m.end(), r.errors.distances_reloc.begin(), r.errors.distances_reloc.end());
            }
            if (!std::isnan(r.errors.reloc_rpe_R.rmse)) {
                stream << "\t               Rotational RPE\t" << r.errors.reloc_rpe_R.rmse*(180.f/M_PI) << "deg\n";
                for(auto& angle_error : r.errors.angles_reloc)
                    reloc_rpe_R_errors_deg.push_back(angle_error*(180.f/M_PI));
            }
            if (!std::isnan(r.errors.reloc_time_sec.rmse)) {
                stream << "\t               Time between relocalizations\t" << r.errors.reloc_time_sec.rmse << "sec\n";
            }
            reloc_times_sec.insert(reloc_times_sec.end(), r.errors.relocalization_time.elapsed_times_sec.begin(), r.errors.relocalization_time.elapsed_times_sec.end());
        }
    }

    std::vector<double> std_edges = {0, 3, 10, 25, 50, 100};
    std::vector<double> alt_edges = {0, 4, 12, 30, 65, 100};
    std::vector<double> ate_edges = {0, 0.02, 0.05, 0.1, 0.2, 0.5};
    std::vector<double> ate_600ms_edges = {0, 0.002, 0.005, 0.01, 0.02, 0.05};
    std::vector<double> rpe_T_edges = {0, 0.02, 0.04, 0.06, 0.1, 0.15};
    std::vector<double> rpe_R_edges = {0, 0.1, 0.25, 0.5, 1, 2};
    std::vector<double> precision_edges = {0, 60, 80, 95, 99, 100};
    std::vector<double> recall_edges = {0, 1, 5, 10, 50, 100};
    std::vector<double> reloc_rpe_T_edges = {0, 0.02, 0.04, 0.06, 0.1, 0.15};
    std::vector<double> reloc_rpe_R_edges = {0, 0.25, 0.5, 1, 2, 5};
    std::vector<double> reloc_times_edges = {0, 1, 2, 3, 5, 10};
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

    stream << "ATE (60s) histogram (" << ate_60s_errors_m.size() << " sequences)\n";
    error_histogram ate_60s_hist(ate_60s_errors_m, ate_edges, 2, "m");
    stream << ate_60s_hist << "\n";

    stream << "ATE (600ms) histogram (" << ate_600ms_errors_m.size() << " sequences)\n";
    error_histogram ate_600ms_hist(ate_600ms_errors_m, ate_600ms_edges, 3, "m");
    stream << ate_600ms_hist << "\n";

    if (has_reloc) {
        stream << "Precision histogram (" << precision_reloc.size() << " sequences)\n";
        error_histogram_pr precision_hist(precision_reloc, precision_edges);
        stream << precision_hist;
        stream << "Undefined precision: " << precision_anomalies << " sequences\n\n";

        stream << "Recall histogram (" << recall_reloc.size() << " sequences)\n";
        error_histogram_pr recall_hist(recall_reloc, recall_edges);
        stream << recall_hist;
        stream << "Undefined recall: " << recall_anomalies << " sequences\n\n";

        stream << "Relocalization RPE (Translation) histogram (" << reloc_rpe_T_errors_m.size() << " events)\n";
        error_histogram reloc_rpe_T_hist(reloc_rpe_T_errors_m, reloc_rpe_T_edges, 2, "m");
        stream << reloc_rpe_T_hist << "\n";

        stream << "Relocalization RPE (Rotation) histogram (" << reloc_rpe_R_errors_deg.size() << " events)\n";
        error_histogram reloc_rpe_R_hist(reloc_rpe_R_errors_deg, reloc_rpe_R_edges, 2, "deg");
        stream << reloc_rpe_R_hist << "\n";

        stream << "Time between relocalizations (" << reloc_times_sec.size() << " events)\n";
        error_histogram reloc_time_hist(reloc_times_sec, reloc_times_edges, 2, "sec");
        stream << reloc_time_hist << "\n";
    }
}