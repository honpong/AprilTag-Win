#pragma once

#include <functional>
#include <algorithm>
#include <vec4.h>
#include <tpose.h>
#include <rc_tracker.h>
#include <rc_compat.h>
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
        } ate, ate_chunked, rpe_T, rpe_R, reloc_rpe_T, reloc_rpe_R, reloc_time_sec;

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
                angle = std::acos(std::min(std::max((G_current_ref_kk.Q.toRotationMatrix().trace()-1)/2, -1.0f),1.0f));
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
                return stream << "\t precision "  << error.precision*100 << " recall "  << error.recall*100;
            }
        } relocalization;

        struct relocalization_time_statistics {
            rc_Timestamp last_reloc_us = 0;
            aligned_vector<f_t> elapsed_times_sec;
        } relocalization_time;

        chunk_state ate_s;
        chunk_group ate_minute_s { std::chrono::seconds(60), 1 };
        chunk_group rpe_second_s { std::chrono::seconds(1), 1 };
        int nposes = 0;
        // RPE variables
        aligned_vector<f_t> distances_reloc, angles_reloc;

        //append the current and ref translations to matrices
        void add_pose(const tpose &current_tpose,const tpose &ref_tpose) {
            nposes++;
            ate_s.add_pose(current_tpose, ref_tpose);
            ate_minute_s.add_pose(current_tpose, ref_tpose);
            rpe_second_s.add_pose(current_tpose, ref_tpose);
        }

        bool calculate_ate() {
            return ate_s.calculate_ate(ate);
        }

        bool calculate_ate_chunked() {
            if(!ate_minute_s.ate_chunk_results.size()) return false;
            ate_chunked.compute(ate_minute_s.ate_chunk_results);
            return true;
        }

        bool calculate_rpe() {
            if(!rpe_second_s.rpe_T_chunk_results.size()) return false;
            rpe_T.compute(rpe_second_s.rpe_T_chunk_results);
            rpe_R.compute(rpe_second_s.rpe_R_chunk_results);
            return true;
        }

        bool calculate_precision_recall(){
            if (relocalization.compute_pr()) {
                if (!std::isnan(relocalization.precision) && !std::isnan(relocalization.recall)) {
                    reloc_rpe_T.compute(distances_reloc);
                    reloc_rpe_R.compute(angles_reloc);
                } else {
                    reloc_rpe_T = reloc_rpe_T*std::numeric_limits<float>::quiet_NaN();
                    reloc_rpe_R = reloc_rpe_R*std::numeric_limits<float>::quiet_NaN();
                }
                reloc_time_sec.compute(relocalization_time.elapsed_times_sec);
                return true;
            } else {
                return false;
            }
        }

        inline bool is_valid() { return nposes > 0; }

        void add_edges(const rc_Timestamp current_frame_timestamp,
                       const rc_RelocEdge* reloc_edges,
                       const int num_reloc_edges,
                       const std::unordered_set<rc_Timestamp>& ref_edges,
                       const tpose_sequence& ref_poses) {
            int tp = 0, fp = 0;
            transformation relative_error;
            // maybe all reloc edges are false positive
            if (!ref_edges.size()) {
                fp = num_reloc_edges;
            } else {
                // traverse all relocalization mapnode timestamps and check if they are in canditate timestamps
                auto current_frame_tp = sensor_clock::micros_to_tp(current_frame_timestamp);
                tpose ref_tpose_current(current_frame_tp);
                bool success = ref_poses.get_pose(current_frame_tp, ref_tpose_current);
                for (int i = 0; i < num_reloc_edges; ++i) {
                    tp += ref_edges.count(reloc_edges[i].time_destination);
                    auto mapnode_tp = sensor_clock::micros_to_tp(reloc_edges[i].time_destination);
                    tpose ref_tpose_mapnode(mapnode_tp);
                    transformation reloc_pose =  to_transformation(reloc_edges[i].pose_m);
                    if (success && ref_poses.get_pose(mapnode_tp, ref_tpose_mapnode)) {
                        transformation ref_relative_pose = invert(ref_tpose_mapnode.G) * ref_tpose_current.G;
                        relative_error = ref_relative_pose * invert(reloc_pose);
                        distances_reloc.emplace_back(relative_error.T.norm());
                        angles_reloc.emplace_back(std::acos(std::min(std::max((relative_error.Q.toRotationMatrix().trace()-1)/2, -1.0f),1.0f)));
                    }
                }
                fp = num_reloc_edges - tp;
            }

            relocalization.true_positives += tp;
            relocalization.false_positives += fp;
            relocalization.false_negatives += ref_edges.size() - tp;

            if (num_reloc_edges > 0) {
                if (relocalization_time.last_reloc_us)
                    relocalization_time.elapsed_times_sec.emplace_back((current_frame_timestamp - relocalization_time.last_reloc_us) * 1e-6);
                relocalization_time.last_reloc_us = current_frame_timestamp;
            }
        }
    } errors;
};

void benchmark_run(std::ostream &stream, const char *directory, int threads,
        std::function<bool (const char *file, struct benchmark_result &result)> measure_file,
        std::function<void (const char *file, struct benchmark_result &result)> measure_done);
