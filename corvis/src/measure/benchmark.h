#pragma once

#include <functional>
#include <algorithm>
#include <vec4.h>
#include <tpose.h>
#include <rc_tracker.h>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <limits>

struct benchmark_result {
    struct { double reference, measured; } length_cm, path_length_cm; void * user_data;
    struct error_metrics {
        // It calculates the error metrics ATE and RPE
        struct statistics {
            f_t rmse = 0, mean = 0, median = 0, std = 0, min = 0, max = 0;

            f_t find_median(aligned_vector<f_t> &v) {
                size_t n = v.size();
                if (n != 0) {
                    // Sort using nth_element (more efficient than sort)
                    std::nth_element(&v[0], &v[0] + n/2, &v[0] + n);
                    // Use first and second points in sorted vector to calculate exact median
                    return (v[(n-1)/2] + v[n/2])/2;
                } else {
                    return 0;
                }
            }

            void compute(aligned_vector<f_t> &errors) {
                if (errors.empty()) return;
                rmse = sqrt(map(errors).array().square().mean());
                mean = map(errors).mean();
                min = map(errors).minCoeff();
                max = map(errors).maxCoeff();
                std = sqrt(((map(errors).array() - mean).square()).mean());
                median = find_median(errors);
            }

            template <typename Stream>
            friend Stream& operator<<(Stream &stream, const statistics &error) {
                return stream << " rmse "   << error.rmse  << " mean "  << error.mean
                              << " median " << error.median << " std "  << error.std
                              << " min "    << error.min    << " max "  << error.max;
            }

            friend statistics operator*(const statistics &error, f_t scale) {
                statistics e;
                e.rmse = error.rmse * scale;
                e.mean = error.mean * scale;
                e.std = error.std * scale;
                e.median = error.median * scale;
                e.min = error.min * scale;
                e.max = error.max * scale;
                return e;
            }
        } ate, rpe_T, rpe_R;

        struct matching_statistics {
            f_t precision = 0, recall = 0;

            void compute_pr(f_t tp, f_t fp, f_t fn) {
                precision =  !(tp + fp) ? std::numeric_limits<float>::quiet_NaN() : tp / (tp + fp);
                recall = !(tp + fn) ? std::numeric_limits<float>::quiet_NaN() : tp / (tp + fn);
            }

            template <typename Stream>
            friend Stream& operator<<(Stream &stream, const matching_statistics &reloc_error) {
                return stream << " precision "   << reloc_error.precision*100 << " recall "  << reloc_error.recall*100;
            }
        } relocalization;

        // ATE variables
        m3 W = m3::Zero();
        m3 R = m3::Identity();
        v3 T = v3::Zero();
        v3 T_current_mean = v3::Zero();
        v3 T_ref_mean = v3::Zero();
        v3 T_error_mean = v3::Zero();
        int nposes = 0;
        int true_positives = 0, true_negatives = 0, false_positives = 0, false_negatives = 0;

        // RPE variables
        aligned_vector<v3> T_current_all, T_ref_all;
        aligned_vector<f_t> distances, angles;
        std::unique_ptr<tpose> current_tpose_ptr, ref_tpose_ptr;

        //append the current and ref translations to matrices
        void add_pose(const tpose &current_tpose,const tpose &ref_tpose) {
            if (!current_tpose_ptr) current_tpose_ptr = std::make_unique<tpose>(current_tpose);
            if (!ref_tpose_ptr) ref_tpose_ptr = std::make_unique<tpose>(ref_tpose);

            // calculate the incremental mean of translations
            T_current_mean = (current_tpose.G.T + T_current_mean*nposes) / (nposes + 1);
            T_ref_mean = (ref_tpose.G.T +  T_ref_mean*nposes) / (nposes + 1);
            T_ref_all.emplace_back(ref_tpose.G.T);
            T_current_all.emplace_back(current_tpose.G.T);
            W = W + (ref_tpose.G.T - T_ref_mean) * (current_tpose.G.T - T_current_mean).transpose();
            nposes++;
            // Calculate the Relative Pose Error (RPE) between two relative poses
            std::chrono::duration<f_t> delta_ref = ref_tpose.t - ref_tpose_ptr->t;
            if ( delta_ref > std::chrono::seconds(1) ) {
                transformation G_ref_kp1_k = invert(ref_tpose.G)*ref_tpose_ptr->G;
                transformation G_current_kp1_k = invert(current_tpose.G)*current_tpose_ptr->G;
                transformation G_current_ref_kk = invert(G_current_kp1_k)*G_ref_kp1_k;
                distances.emplace_back(G_current_ref_kk.T.norm());
                angles.emplace_back(std::acos(std::min(std::max((G_current_ref_kk.Q.toRotationMatrix().trace()-1)/2, -1.0f),1.0f)));
                // update pose
                *current_tpose_ptr = current_tpose;
                *ref_tpose_ptr = ref_tpose;
                rpe_T.compute(distances);
                rpe_R.compute(angles);
            }
        }

        //solve for Horn's Rotation and translation. It uses a closed form solution
        //(no approximation is applied, but it is subject to the svd implementation).
        bool calculate_ate() {
            R = project_rotation(W.transpose());
            T = T_current_mean - R * T_ref_mean;
            m<3,Eigen::Dynamic> T_ref_aligned = R*map(T_ref_all).transpose() + T.replicate(1,nposes);
            m<3,Eigen::Dynamic> T_errors = T_ref_aligned - map(T_current_all).transpose();
            v<Eigen::Dynamic> rse = T_errors.colwise().norm(); // ||T_error||_l2
            aligned_vector<f_t> rse_v(rse.data(),rse.data() + rse.cols()*rse.rows());
            ate.compute(rse_v);
            return nposes;
        }

        bool calculate_precision_recall(){
            relocalization.compute_pr(true_positives,false_positives,false_negatives);
            return true;
        }

        inline bool is_valid() { return nposes > 0; }

        bool add_edges(const int num_reloc_edges,
                       const int num_mapnodes,
                       const rc_RelocEdge* reloc_edges,
                       const rc_Timestamp* mapnodes_timestamps,
                       const std::unordered_multimap<rc_Timestamp, std::unordered_set<rc_Timestamp>>& ref_edges) {

            if (!num_reloc_edges) return false;
            // get range in reference multimap for currentframe timestamp and creates a multimap from it
            const rc_Timestamp& current_frame_timestamp = reloc_edges[0].time_source;

            // search for map node timestamps in a subset of reference edges
            std::unordered_set<rc_Timestamp> ref_mapnode_edges;
            {
                auto it = ref_edges.find(current_frame_timestamp);
                if (it != ref_edges.end()) {
                    for (rc_Timestamp destination : it->second) {
                        if (std::binary_search(mapnodes_timestamps, mapnodes_timestamps + num_mapnodes,
                                               destination)) {
                            ref_mapnode_edges.emplace(destination);
                        }
                    }
                }
            }

            int tp = 0, fp = 0;
            // maybe all reloc edges are false positive
            if (!ref_mapnode_edges.size()) {
                fp = num_reloc_edges;
            } else {
                // traverse all relocalization mapnode timestamps and check if they are in canditate timestamps
                for (int i = 0; i < num_reloc_edges; ++i) {
                    tp += ref_mapnode_edges.count(reloc_edges[i].time_destination);
                }
                fp = num_reloc_edges - tp;
            }

            true_positives += tp;
            false_positives += fp;
            false_negatives += ref_mapnode_edges.size() - tp;

            return true;
        }
    } errors;
};

void benchmark_run(std::ostream &stream, const char *directory, int threads,
        std::function<bool (const char *file, struct benchmark_result &result)> measure_file,
        std::function<void (const char *file, struct benchmark_result &result)> measure_done);
