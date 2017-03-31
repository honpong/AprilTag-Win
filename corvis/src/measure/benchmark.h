#pragma once

#include <functional>
#include <iostream>
#include <fstream>
#include <vector>
#include <vec4.h>

struct benchmark_result {
    struct { double reference, measured; } length_cm, path_length_cm; void * user_data;
    struct absolute_translation_error {
        // This class calculates ATE (absolute translation error) using the Horn's method
        m3 R = m3::Identity();
        m3 W = m3::Zero();
        v3 T = v3::Zero();
        v3 T_current_mean = v3::Zero();
        v3 T_ref_mean = v3::Zero();
        int nposes = 0;
        float rmse = 0, mean = 0, median = 0, std = 0, min = 0, max = 0;
        bool bincremental_ate = 0;        
        aligned_vector<v3> T_current_all, T_ref_all; // we need to keep the whole trajectory

        absolute_translation_error(){}
        ~absolute_translation_error(){}
        inline void print_statistics() {
            std::cout << "Error Statistics (ATE): "<< "ate.rmse "  << rmse
                      << " ate.mean " << mean
                      << " ate.std " << std << " ate.min " << min
                      << " ate.max " << max << "\n"; }
        //append the current and ref translations to matrices
        inline void add_translation(const v3 &T_current, const v3 &T_ref) {
            // calculate the incremental mean of translations
            T_current_mean = (T_current + T_current_mean*nposes) / (nposes + 1);
            T_ref_mean = (T_ref +  T_ref_mean*nposes) / (nposes + 1);
            nposes++;
            T_ref_all.emplace_back(T_ref);
            T_current_all.emplace_back(T_current);
            accumulate_W();
        }
        //add constraint
        inline void accumulate_W() {
            //substract mean
            v3 T_current = T_current_all.back();
            v3 T_ref = T_ref_all.back();
            v3 T_current_centered =  T_current - T_current_mean;
            v3 T_ref_centered =  T_ref - T_ref_mean;
            //calculate LS matrix
            W = W + T_ref_centered * T_current_centered.transpose();
        }
       //solve for Horn's Rotation and  translation. It uses a closed form solution
        //(no approximation is applied, but it is subject to the svd implementation)
        inline void solve_horn() {
            Eigen::JacobiSVD<m3> svd(W.transpose(), Eigen::ComputeFullU | Eigen::ComputeFullV);
            m3 U = svd.matrixU();
            m3 V = svd.matrixV();
            m3 S = m3::Identity();
            if (U.determinant()*V.determinant() < 0) {
                S(2,2) = -1;
            }
            R = U*S*V.transpose();
            T = T_current_mean - R*T_ref_mean;
        }
        // Calculate statistics. This can be called at each timestep but it might be computationally
        // expensive since we align the whole gt trajectory to the estimated one using the rotation
        // and translation in the results variable;
        inline void calculate_statistics() {
            solve_horn();
            m<3,Eigen::Dynamic> T_ref_aligned = R*::map(T_ref_all).transpose() + T.replicate(1,nposes);
            m<3,Eigen::Dynamic> T_errors = T_ref_aligned - ::map(T_current_all).transpose();
            v<Eigen::Dynamic> rse = T_errors.colwise().norm(); // ||T_error||_l2
            rmse = sqrt(rse.array().square().mean()); // sum(||T_error||_l2^2/n)^(1/2)
            mean = rse.mean();
            min = rse.minCoeff();
            max = rse.maxCoeff();
            std = sqrt(((rse.array() - mean).square()).mean());
        }
        // In case we want to calculate ate at each timestep
        inline float calculate_ate(const v3 &T_current, const v3 &T_ref) {
            add_translation(T_current, T_ref);
            accumulate_W();
            calculate_statistics();
            return rmse;
        }
        inline void set_execution_type(const bool incremental_ate) { bincremental_ate = incremental_ate; }
        inline bool is_incremental(){ return bincremental_ate; }
        inline bool is_valid(){ return nposes > 0; }
    } ate;
};

void benchmark_run(std::ostream &stream, const char *directory,
        std::function<bool (const char *file, struct benchmark_result &result)> measure_file,
        std::function<void (const char *file, struct benchmark_result &result)> measure_done);
