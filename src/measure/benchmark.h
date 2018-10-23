#pragma once

#include <functional>
#include <algorithm>
#include <set>
#include <vec4.h>
#include <tpose.h>
#include <rc_tracker.h>
#include <rc_compat.h>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <limits>
#include "rc_tracker.h"
#include "rc_internal.h"

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
                using rc::map;
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
                return stream << "\t rmse "   << error.rmse  << " mean "  << error.mean
                              << "\t median " << error.median << " std "  << error.std
                              << "\t min "    << error.min    << " max "  << error.max;
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
        } ate, ate_60s, ate_600ms, rpe_T, rpe_R, reloc_rpe_T, reloc_rpe_R, reloc_time_sec;

        struct chunk_state {
            // ATE variables
            m3 W = m3::Zero();
            m3 R = m3::Identity();
            v3 T = v3::Zero();
            v3 T_current_mean = v3::Zero();
            v3 T_ref_mean = v3::Zero();
            v3 T_error_mean = v3::Zero();
            int nposes = 0;
            aligned_vector<v3> T_current_all, T_ref_all;

            //RPE
            transformation G_ref_1, G_our_1, G_ref_2, G_our_2;

            void add_pose(const tpose &current_tpose,const tpose &ref_tpose) {
                if(!nposes) {
                    G_ref_1 = ref_tpose.G;
                    G_our_1 = current_tpose.G;
                }
                G_ref_2 = ref_tpose.G;
                G_our_2 = current_tpose.G;

                // calculate the incremental mean of translations
                T_current_mean = (current_tpose.G.T + T_current_mean*nposes) / (nposes + 1);
                T_ref_mean = (ref_tpose.G.T +  T_ref_mean*nposes) / (nposes + 1);
                T_ref_all.emplace_back(ref_tpose.G.T);
                T_current_all.emplace_back(current_tpose.G.T);
                W = W + (ref_tpose.G.T - T_ref_mean) * (current_tpose.G.T - T_current_mean).transpose();
                nposes++;
            }

            //solve for Horn's Rotation and translation. It uses a closed form solution
            //(no approximation is applied, but it is subject to the svd implementation).
            bool calculate_ate(statistics &statistics) {
                using rc::map;
                R = project_rotation(W.transpose());
                T = T_current_mean - R * T_ref_mean;
                m<3,Eigen::Dynamic> T_ref_aligned = R*map(T_ref_all).transpose() + T.replicate(1,nposes);
                m<3,Eigen::Dynamic> T_errors = T_ref_aligned - map(T_current_all).transpose();
                v<Eigen::Dynamic> rse = T_errors.colwise().norm(); // ||T_error||_l2
                aligned_vector<f_t> rse_v(rse.data(),rse.data() + rse.cols()*rse.rows());
                statistics.compute(rse_v);
                return nposes;
            }

            bool calculate_rpe(float &distance, float &angle) {
                // Calculate the Relative Pose Error (RPE) between two relative poses
                transformation G_ref_kp1_k = invert(G_ref_2)*G_ref_1;
                transformation G_current_kp1_k = invert(G_our_2)*G_our_1;
                transformation G_current_ref_kk = invert(G_current_kp1_k)*G_ref_kp1_k;
                distance = G_current_ref_kk.T.norm();
                angle = to_rotation_vector(G_current_ref_kk.Q).raw_vector().norm(); // [0,pi]
                return nposes > 1;
            }
        };

        struct chunk_group {
            chunk_group(sensor_clock::duration i, size_t o): start_interval(i), overlap_count(o) {}

            sensor_clock::duration start_interval;
            size_t overlap_count;
            sensor_clock::duration length() { return start_interval * overlap_count; }
            std::list<chunk_state> chunks;
            sensor_clock::time_point last_chunk_start;

            aligned_vector<f_t> ate_chunk_results, rpe_T_chunk_results, rpe_R_chunk_results;

            void add_pose(const tpose &current_tpose,const tpose &ref_tpose) {
                if(chunks.size() == 0 || current_tpose.t - last_chunk_start > start_interval) {
                    last_chunk_start = current_tpose.t;
                    chunks.emplace_back();
                    if(chunks.size() > overlap_count) {
                        statistics s;
                        float distance, angle;
                        chunks.front().calculate_ate(s);
                        chunks.front().calculate_rpe(distance, angle);
                        chunks.pop_front();
                        ate_chunk_results.emplace_back(s.rmse);
                        rpe_T_chunk_results.emplace_back(distance);
                        rpe_R_chunk_results.emplace_back(angle);
                    }
                }
                for(auto &i : chunks) i.add_pose(current_tpose, ref_tpose);
            }
        };

        struct matching_statistics {
            int detections = 0;
            int true_positives = 0, false_positives = 0, false_negatives = 0;
            f_t precision = 0, recall = 0;

            bool compute_pr() {
                precision =  !(true_positives + false_positives) ? std::numeric_limits<f_t>::quiet_NaN() :
                                                                   static_cast<float>(true_positives) / (true_positives + false_positives);
                recall = !(true_positives + false_negatives) ? std::numeric_limits<f_t>::quiet_NaN() :
                                                               static_cast<float>(true_positives) / (true_positives + false_negatives);
                return true_positives + false_positives + false_negatives != 0;
            }

            template <typename Stream>
            friend Stream& operator<<(Stream &stream, const matching_statistics &error) {
                stream << "\t detections " << error.detections;
                if (error.true_positives + error.false_positives + error.false_negatives != 0) {
                    stream << "\n"
                           << "\t precision "  << error.precision*100 << " recall "  << error.recall*100 << "\n"
                           << "\t correct " << error.true_positives << "\n"
                           << "\t missed " << error.false_negatives;
                }
                return stream;
            }
        } relocalization;

        struct relocalization_time_statistics {
            rc_Timestamp last_reloc_us = 0;
            aligned_vector<f_t> elapsed_times_sec;
        } relocalization_time;

        chunk_state ate_s;
        chunk_group chunks_60s { std::chrono::seconds(60), 1 };
        chunk_group chunks_600ms { std::chrono::milliseconds(600), 1 };
        int nposes = 0;
        // RPE variables
        aligned_vector<f_t> distances_reloc, angles_reloc;

        //append the current and ref translations to matrices
        void add_pose(const tpose &current_tpose,const tpose &ref_tpose) {
            nposes++;
            ate_s.add_pose(current_tpose, ref_tpose);
            chunks_60s.add_pose(current_tpose, ref_tpose);
            chunks_600ms.add_pose(current_tpose, ref_tpose);
        }

        bool calculate_ate() {
            return ate_s.calculate_ate(ate);
        }

        bool calculate_ate_60s() {
            if(!chunks_60s.ate_chunk_results.size()) return false;
            ate_60s.compute(chunks_60s.ate_chunk_results);
            return true;
        }

        bool calculate_ate_600ms() {
            if(!chunks_600ms.ate_chunk_results.size()) return false;
            ate_600ms.compute(chunks_600ms.ate_chunk_results);
            return true;
        }

        bool calculate_rpe_600ms() {
            if(!chunks_600ms.rpe_T_chunk_results.size()) return false;
            rpe_T.compute(chunks_600ms.rpe_T_chunk_results);
            rpe_R.compute(chunks_600ms.rpe_R_chunk_results);
            return true;
        }

        bool calculate_precision_recall(){
            reloc_time_sec.compute(relocalization_time.elapsed_times_sec);
            if (relocalization.compute_pr()) {
                if (!std::isnan(relocalization.precision) && !std::isnan(relocalization.recall)) {
                    reloc_rpe_T.compute(distances_reloc);
                    reloc_rpe_R.compute(angles_reloc);
                } else {
                    reloc_rpe_T = reloc_rpe_T*std::numeric_limits<float>::quiet_NaN();
                    reloc_rpe_R = reloc_rpe_R*std::numeric_limits<float>::quiet_NaN();
                }
                return true;
            } else {
                return false;
            }
        }

        inline bool is_valid() { return nposes > 0; }

        void add_relocalization(const rc_Relocalization& reloc) {
            ++relocalization.detections;
            if (relocalization_time.last_reloc_us != reloc.time_us) {
                relocalization_time.elapsed_times_sec.emplace_back((reloc.time_us - relocalization_time.last_reloc_us) * 1e-6);
                relocalization_time.last_reloc_us = reloc.time_us;
            }
        }

        void add_edges(const rc_Timestamp current_frame_timestamp,
                       const rc_RelocEdge* reloc_edges,
                       const int num_reloc_edges,
                       const std::set<rc_SessionTimestamp>& ref_edges,
                       const std::vector<tpose_sequence>& ref_poses) {
            int tp = 0, fp = 0;
            transformation relative_error;
            // maybe all reloc edges are false positive
            if (!ref_edges.size()) {
                fp = num_reloc_edges;
            } else {
                // traverse all relocalization mapnode timestamps and check if they are in canditate timestamps
                auto current_frame_tp = sensor_clock::micros_to_tp(current_frame_timestamp);
                tpose ref_tpose_current(current_frame_tp);
                bool success = ref_poses[rc_SESSION_CURRENT_SESSION].get_pose(current_frame_tp, ref_tpose_current);
                for (int i = 0; i < num_reloc_edges; ++i) {
                    tp += ref_edges.count(reloc_edges[i].time_destination);
                    auto mapnode_tp = sensor_clock::micros_to_tp(reloc_edges[i].time_destination.time_us);
                    tpose ref_tpose_mapnode(mapnode_tp);
                    transformation reloc_pose =  to_transformation(reloc_edges[i].pose_m);
                    if (success && ref_poses[reloc_edges[i].time_destination.session_id].get_pose(mapnode_tp, ref_tpose_mapnode)) {
                        transformation ref_relative_pose = invert(ref_tpose_mapnode.G) * ref_tpose_current.G;
                        relative_error = ref_relative_pose * invert(reloc_pose);
                        distances_reloc.emplace_back(relative_error.T.norm());
                        angles_reloc.emplace_back(to_rotation_vector(relative_error.Q).raw_vector().norm());
                    }
                }
                fp = num_reloc_edges - tp;
            }

            relocalization.true_positives += tp;
            relocalization.false_positives += fp;
            relocalization.false_negatives += ref_edges.size() - tp;
        }
    } errors;

    rc_StorageStats storage;
};

template <typename Stream>
Stream& operator<<(Stream &stream, const rc_StorageStats &s) {
    return stream << "\t nodes "   << s.nodes  << " edges " << s.edges
                  << " features " << s.features << " keypoints " << s.unique_features
                  << " words " << s.relocalization_bins;
}

void benchmark_run(std::ostream &stream, const std::vector<const char *> &filenames, int threads,
        std::function<bool (const char *file, struct benchmark_result &result)> measure_file,
        std::function<void (const char *file, struct benchmark_result &result)> measure_done);
