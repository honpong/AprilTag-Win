#include "observation.h"
#include "utils.h"
#include "Trace.h"

int observation_queue::size()
{
    int size = 0;
    for(auto &o : observations)
        size += o->size;
    return size;
}

void observation_queue::predict()
{
    for(auto &o : observations)
        o->predict();
}

void observation_queue::measure_and_prune()
{
    observations.erase(remove_if(observations.begin(), observations.end(), [this](std::unique_ptr<observation> &o) {
       bool ok = o->measure();
       if (!ok)
           this->cache_recent(std::move(o));
       return !ok;
    }), observations.end());
}

void observation_queue::compute_innovation(matrix &inn)
{
    int count = 0;
    for(auto &o : observations) {
        o->compute_innovation();
        for(int i = 0; i < o->size; ++i) {
            inn[count + i] = o->innovation(i);
        }
        count += o->size;
    }
}

void observation_queue::compute_measurement_covariance(matrix &m_cov)
{
    int count = 0;
    for(auto &o : observations) {
        o->compute_measurement_covariance();
        for(int i = 0; i < o->size; ++i) {
            m_cov[count + i] = o->measurement_covariance(i);
        }
        count += o->size;
    }
}

#ifdef ENABLE_SHAVE_PROJECT_OBSERVATION_COVARIANCE
#include "covariance_projector.h"

void observation_queue::compute_prediction_covariance_shave(const matrix &cov, int statesize, int meas_size)
{
    auto start = std::chrono::steady_clock::now();

    __attribute__((section(".cmx_direct.bss")))
    static project_observation_covariance_data queue_data;
    START_EVENT(SF_PROJECT_OBSERVATION_COVARIANCE, cov.rows());
    queue_data.src_rows     = cov.rows();
    queue_data.src_cols     = cov.cols();
    queue_data.src_stride   = cov.get_stride();
    queue_data.src          = cov.Data();
    queue_data.HP_stride    = HP.get_stride();
    queue_data.HP_src_cols  = HP.cols();
    queue_data.HP_dst_cols  = statesize;
    queue_data.HP_rows      = HP.rows();
    queue_data.HP           = HP.Data();
    queue_data.dst_rows     = res_cov.rows();
    queue_data.dst_cols     = meas_size;
    queue_data.dst_stride   = res_cov.get_stride();
    queue_data.dst          = res_cov.Data();
    queue_data.observations_size = observations.size();

    int obs_per_shave = (observations.size() + PROJECT_COVARIANE_SHAVES -1) / PROJECT_COVARIANE_SHAVES;

    for(int i=0; i < PROJECT_COVARIANE_SHAVES; i++) {
        start_index[i] = 0;
    }

    int i=0;
    int index = 0;
    for (auto &o : observations) {
        o->cache_jacobians();
        start_index[i++/obs_per_shave + 1] += o->size;
        observation_datas.push_back(o->getData(index++));
    }
    queue_data.observations = (observation_data**)observation_datas.data();

    for(int i=1; i < PROJECT_COVARIANE_SHAVES; i++) {
        start_index[i] += start_index[i-1];
    }

    static covariance_projector projector;
    projector.project_observation_covariance(queue_data, start_index);

    observation_datas.clear();
    END_EVENT(SF_PROJECT_OBSERVATION_COVARIANCE, meas_size);
    auto stop = std::chrono::steady_clock::now();
    project_stats.data(v<1> { static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
}
#endif

void observation_queue::compute_prediction_covariance(const matrix &cov, int statesize, int meas_size)
{
    auto start = std::chrono::steady_clock::now();
    START_EVENT(SF_PROJECT_OBSERVATION_COVARIANCE_LEON, cov.rows());
    int index = 0;
    for(auto &o : observations) {
        matrix dst(HP, index, 0, o->size, statesize);
        o->cache_jacobians();
        o->project_covariance(dst, cov); // HP = H * P'
        index += o->size;
    }

    index = 0;
    for(auto &o : observations) {
        matrix dst(res_cov, index, 0, o->size, meas_size);
        o->project_covariance(dst, HP); // res_cov = H * (H * P')' = H * P * H'
        index += o->size;
    }
    END_EVENT(SF_PROJECT_OBSERVATION_COVARIANCE_LEON, meas_size);
    auto stop = std::chrono::steady_clock::now();
    project_stats.data(v<1> { static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
}

void observation_queue::compute_innovation_covariance(const matrix &m_cov)
{
    int index = 0;
    for(auto &o : observations) {
        for(int i = 0; i < o->size; ++i) {
            res_cov(index + i, index + i) += m_cov[index + i];
        }
        o->innovation_covariance_hook(res_cov, index);
        index += o->size;
    }
}

bool observation_queue::update_state_and_covariance(matrix &x, matrix &P, const matrix &y, matrix &HP, matrix &S)
{
    int meas_size = HP.rows(), statesize = HP.cols();
    matrix Px(P, 0,0, statesize+1, statesize); // [ P ; x ]
    Px.map().bottomRows(1) = x.map();

    if (meas_size != 3)
        meas_size_stats.data(v<1> { (float)meas_size });
    state_size_stats.data(v<1> { (float)statesize });
    matrix HP_y (HP, 0,0, meas_size, statesize+1);
    HP_y.map().rightCols(1) = -y.map().transpose();
    bool ok;
    {
        auto start = std::chrono::steady_clock::now();
        ok = matrix_cholesky(S); // S = L L^T;
        auto stop = std::chrono::steady_clock::now();
        cholesky_stats.data(v<1> { static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
    }
    if (!ok)
        return false;

    {
        auto start = std::chrono::steady_clock::now();
        matrix_half_solve(S , HP_y); // HP_y = L^-1 [ HP -y ]
        auto stop = std::chrono::steady_clock::now();
        solve_stats.data(v<1> { static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
    }

    {
        auto start = std::chrono::steady_clock::now();
        matrix_product(Px, HP_y, HP, true, false, -1); // [P ; x ] -= (L^-1 [HP -y])' * (L^-1 HP)
        auto stop = std::chrono::steady_clock::now();
        multiply_stats.data(v<1> { static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
    }

    x.map() = Px.map().bottomRows(1); // write back the updated state

    return true;
}

void observation_queue::preprocess(state_root &s, sensor_clock::time_point time)
{
    s.time_update(time);

    predict();
}

bool observation_queue::process(state_root &s, bool run_on_shave)
{
    bool success = true;

    int orig_meas_size = size();

    measure_and_prune();

    int meas_size = size(), statesize = s.cov.size();
    if(meas_size) {
        inn.resize(meas_size);
        m_cov.resize(meas_size);
        HP.resize(meas_size, statesize);
        res_cov.resize(meas_size, meas_size);
        state.resize(statesize);

        compute_innovation(inn);
        compute_measurement_covariance(m_cov);
#ifdef ENABLE_SHAVE_PROJECT_OBSERVATION_COVARIANCE
        if (run_on_shave && observations.size() >= 4)
            compute_prediction_covariance_shave(s.cov.cov, statesize, meas_size);
        else
            compute_prediction_covariance(s.cov.cov, statesize, meas_size);
#else
            compute_prediction_covariance(s.cov.cov, statesize, meas_size);
#endif
        compute_innovation_covariance(m_cov);

        s.copy_state_to_array(state);

        success = update_state_and_covariance(state, s.cov.cov, inn, HP, res_cov);

        s.copy_state_from_array(state);
    } else if(orig_meas_size && orig_meas_size != 3) {
        //s.log->warn("In Kalman update, original measurement size was {}, ended up with 0 measurements!\n", orig_meas_size);
    }

    s.cov.cov.map().triangularView<Eigen::StrictlyUpper>() = s.cov.cov.map().triangularView<Eigen::StrictlyLower>().transpose();

    recent_f_map.clear();
    for (auto &o : observations)
        cache_recent(std::move(o));

    observations.clear();
#ifdef TEST_POSDEF
    if(!test_posdef(s.cov.cov)) {fprintf(stderr, "not pos def when finishing process observation queue\n"); assert(0);}
#endif
    return success;
}

void observation::innovation_covariance_hook(const matrix &cov, int index)
{
    if (show_tuning)
        std::cerr << " predicted stdev is " << cov.map().block(index, index, size, size).diagonal().array().sqrt() << "\n";
}

void observation_vision_feature::innovation_covariance_hook(const matrix &cov, int index)
{
    track.innovation_variance_x = cov(index, index);
    track.innovation_variance_y = cov(index + 1, index + 1);
    track.innovation_variance_xy = cov(index, index +1);
    observation::innovation_covariance_hook(cov, index);
}

void observation_vision_feature::predict()
{
    Rt = state.Q.v.toRotationMatrix().transpose();
    Rr = feature->group.Qr.v.toRotationMatrix();
    Rct = curr.camera.extrinsics.Q.v.toRotationMatrix().transpose();
    m3 Ro = orig.camera.extrinsics.Q.v.toRotationMatrix();
    Rw = Rr * Ro;
    m3 Rb = Rt * Rw;
    Tw = Rr * orig.camera.extrinsics.T.v + feature->group.Tr.v;
    v3 Tb = Rt * (Tw - state.T.v);
    Rtot = Rct * Rb;
    Ttot = Rct * (Tb - curr.camera.extrinsics.T.v);

    X0 = orig.camera.intrinsics.unproject_feature(feature->v->initial);
    feature->body = Rb * X0 * feature->v->depth() + Tb;

    X = Rtot * X0 + Ttot * feature->v->invdepth();
    pred = curr.camera.intrinsics.project_feature(X);
    track.track.pred_x = pred.x();
    track.track.pred_y = pred.y();
}

void observation_vision_feature::cache_jacobians()
{
    m<2,3> dx_dX = curr.camera.intrinsics.dproject_dX(X);
    v3 dX_dp = Ttot * (feature->is_initialized() ? feature->v->invdepth_jacobian() : 0);
    dx_dp = dx_dX * dX_dp;
    f_t invrho = feature->is_initialized() ? feature->v->invdepth() : 0;

    // X = Rtot * X0 + Ttot / depth
    // dX = dRtot * X0 + Rtot * dX0 + dTtot / depth - Tot / depth / depth ddepth
    // d(Rtot X0) = (Rc' Rr' Ro X0)^ Rc' (dRc Rc')v + (Rc' Rr' Ro X0)^ Rc' Rr' (dRr Rr')v  - (Rr' Rc' Ro X0)^ Rc' Rr' (dRo Ro')v + Rtot dX0
    // d Ttot = Ttot^ Rc' (dRc Rc')v  - Rc' Tc^ (dRc Rc')v + (Rc' Rr' Rc) Rc' (To - Tr)^ (dRr Rr')v + Rc' Rr' (dTo - dTr) - Rc dTc
    v3 RtotX0 = Rtot * X0;
    m3 dRtotX0_dQr = skew(-RtotX0) * (Rct * Rt);
    m3 dRtotX0_dQ = skew(RtotX0) * (Rct * Rt);

    m3 dTtot_dQr = (Rct * Rt) * skew(Rr * -orig.camera.extrinsics.T.v);
    m3 dTtot_dQ = (Rct * Rt) * skew(Rr * orig.camera.extrinsics.T.v + feature->group.Tr.v - state.T.v);
    m3 dTtot_dTr = (Rct * Rt);
    m3 dTtot_dT = -(Rct * Rt);

    dx_dQr = dx_dX * (dTtot_dQr * invrho + dRtotX0_dQr);
    dx_dQ  = dx_dX * (dTtot_dQ  * invrho + dRtotX0_dQ );
    dx_dTr = dx_dX *  dTtot_dTr * invrho;
    dx_dT  = dx_dX *  dTtot_dT  * invrho;

    if(orig.camera.intrinsics.estimate) {
        m<3,2> dX0_dc;
        m<3,1> dX0_dF;
        m<3,4> dX0_dk;
        orig.camera.intrinsics.dunproject_dintrinsics(feature->v->initial, dX0_dc, dX0_dF, dX0_dk);
        orig.dx_dF = dx_dX * Rtot * dX0_dF;
        orig.dx_dk = dx_dX * Rtot * dX0_dk;
        orig.dx_dc = dx_dX * Rtot * dX0_dc;
    }

    if (curr.camera.intrinsics.estimate) {
        curr.camera.intrinsics.dproject_dintrinsics(X, curr.dx_dc, curr.dx_dF, curr.dx_dk);
    }

    if (curr.camera.extrinsics.estimate) {
        m3 dRtotX0_dQc = skew(RtotX0) * Rct;
        m3 dTtot_dQc = skew(Ttot) * Rct - Rct * skew(curr.camera.extrinsics.T.v);
        m3 dTtot_dTc = -Rct;
        curr.dx_dQ = dx_dX * (dTtot_dQc * invrho + dRtotX0_dQc);
        curr.dx_dT = dx_dX *  dTtot_dTc * invrho;
    }

    if (orig.camera.extrinsics.estimate) {
        m3 dTtot_dQo = m3::Zero();
        m3 dTtot_dTo = Rct * Rt * Rr;
        m3 dRtotX0_dQo = skew(-RtotX0) * (Rct * Rt * Rr);
        orig.dx_dQ = dx_dX * (dTtot_dQo * invrho + dRtotX0_dQo);
        orig.dx_dT = dx_dX *  dTtot_dTo * invrho;
    }
}

void observation_vision_feature::project_covariance(matrix &dst, const matrix &src) const
{
    int i=0;
    i = project_covariance<4>(dst,src,i);
    i = project_covariance<1>(dst,src,i);
}

template<int N>
    int observation_vision_feature::project_covariance(matrix &dst, const matrix &src, int j) const
{
    for(; j < dst.cols()/N*N; j+=N) {
        const m<1,N> cov_feat = feature->from_row<N>(src, j);
        const m<3,N> scov_Qr  = feature->group.Qr.from_row<N>(src, j);
        const m<3,N> cov_Tr  = feature->group.Tr.from_row<N>(src, j);
        const m<3,N> scov_Q  = state.Q.from_row<N>(src, j);
        const m<3,N> cov_T  = state.T.from_row<N>(src, j);
        col<N>(dst, j) = dx_dp * cov_feat + dx_dQr * scov_Qr + dx_dTr * cov_Tr + dx_dQ * scov_Q + dx_dT * cov_T;

        if (curr.camera.extrinsics.estimate) {
            const m<3,N> scov_Qc = curr.camera.extrinsics.Q.from_row<N>(src, j);
            const m<3,N>  cov_Tc = curr.camera.extrinsics.T.from_row<N>(src, j);
            col<N>(dst, j) += curr.dx_dQ * scov_Qc + curr.dx_dT * cov_Tc;
        }

        if (orig.camera.extrinsics.estimate) {
            const m<3,N> scov_Qo = orig.camera.extrinsics.Q.from_row<N>(src, j);
            const m<3,N>  cov_To = orig.camera.extrinsics.T.from_row<N>(src, j);
            col<N>(dst, j) += orig.dx_dQ * scov_Qo + orig.dx_dT * cov_To;
        }

        if (curr.camera.intrinsics.estimate) {
            const m<1,N> cov_F = curr.camera.intrinsics.focal_length.from_row<N>(src, j);
            const m<2,N> cov_c = curr.camera.intrinsics.center.from_row<N>(src, j);
            const m<4,N> cov_k = curr.camera.intrinsics.k.from_row<N>(src, j);
            col<N>(dst, j) += curr.dx_dF * cov_F + curr.dx_dc * cov_c + curr.dx_dk * cov_k;
        }

        if (orig.camera.intrinsics.estimate) {
            const m<1,N> cov_F = orig.camera.intrinsics.focal_length.from_row<N>(src, j);
            const m<2,N> cov_c = orig.camera.intrinsics.center.from_row<N>(src, j);
            const m<4,N> cov_k = orig.camera.intrinsics.k.from_row<N>(src, j);
            col<N>(dst, j) += orig.dx_dF * cov_F + orig.dx_dc * cov_c + orig.dx_dk * cov_k;
        }
    }
    return j;
}

f_t observation_vision_feature::projection_residual(const v3 & X, const feature_t & found_undistorted)
{
    v2 Xu = X.head<2>() / X[2];
    return (Xu - found_undistorted).squaredNorm();
}

void observation_vision_feature::update_initializing()
{
    if(feature->is_initialized()) return;
    f_t min = 0.01f; //infinity-ish (100m)
    f_t max = 10; //1/.10 for 10cm
    f_t min_d2, max_d2;
    v3 X_inf = Rtot * X0;

    v3 X_inf_proj = X_inf / X_inf[2];
    v3 X_0 = X_inf + max * Ttot;

    v3 X_0_proj = X_0 / X_0[2];
    v3 delta = (X_inf_proj - X_0_proj);
    auto &intrinsics = curr.camera.intrinsics;
    f_t normalized_distance_to_px = intrinsics.focal_length.v * intrinsics.image_height;
    f_t pixelvar = delta.dot(delta) * normalized_distance_to_px * normalized_distance_to_px;
    if(pixelvar > 5.f * 5.f * source.measurement_variance) { //tells us if we have enough baseline
        feature->status = feature_normal;
    }

    feature_t bestkp = meas;
    feature_t bestkp_norm = intrinsics.undistort_feature(intrinsics.normalize_feature(bestkp));

    min_d2 = projection_residual(X_inf + min * Ttot, bestkp_norm);
    max_d2 = projection_residual(X_inf + max * Ttot, bestkp_norm);
    f_t best = min;
    f_t best_d2 = min_d2;
    for(int i = 0; i < 10; ++i) { //10 iterations = 1024 segments
        if(min_d2 < max_d2) {
            max = (min + max) / 2;
            max_d2 = projection_residual(X_inf + max * Ttot, bestkp_norm);
            if(min_d2 < best_d2) {
                best_d2 = min_d2;
                best = min;
            }
        } else {
            min = (min + max) / 2;
            min_d2 = projection_residual(X_inf + min * Ttot, bestkp_norm);
            if(max_d2 < best_d2) {
                best_d2 = max_d2;
                best = max;
            }
        }
    }
    if(best > .01f && best < 10.f) {
        feature->v->set_depth_meters(1/best);
    }
    //repredict using triangulated depth
    predict();
}

bool observation_vision_feature::measure()
{
    if(feature->status == feature_lost) return false;
    meas = {track.track.x, track.track.y};

    if(track.track.found()) {
        source.meas_stdev.data(meas);
        if(!feature->is_initialized()) {
            update_initializing();
        }
    }

    // The covariance will become ill-conditioned if either the original predict()ion
    // or the predict()ion in update_initializing() lead to huge or non-sensical pred[]s
    if (X[2] < .01f) { // throw away features that are predicted to be too close to or even behind the camera
        feature->make_outlier();
        return false;
    }

    return track.track.found();
}

void observation_vision_feature::compute_measurement_covariance()
{
    source.inn_stdev.data(inn);
    f_t ot = track.outlier_thresh * track.outlier_thresh * (curr.camera.intrinsics.image_height/240.f)*(curr.camera.intrinsics.image_height/240.f);
    f_t residual = inn.squaredNorm();
    if(residual / ot > source.measurement_variance) {
        m_cov = v2::Constant(residual / ot);
        track.outlier += residual / ot / source.measurement_variance;
    } else {
        m_cov = v2::Constant(source.measurement_variance);
        track.outlier = 0.;
    }
}

#ifdef ENABLE_SHAVE_PROJECT_OBSERVATION_COVARIANCE
#include "state_size.h"

observation_data* observation_vision_feature::getData(int index)
{
    __attribute__((section(".cmx_direct.bss")))
    static observation_vision_feature_data vision_datas[MAXOBSERVATIONSIZE];

    vision_datas[index].size = size;

    vision_datas[index].orig.e_estimate = orig.camera.extrinsics.estimate;
    vision_datas[index].orig.i_estimate = orig.camera.intrinsics.estimate;
    vision_datas[index].curr.e_estimate = curr.camera.extrinsics.estimate;
    vision_datas[index].curr.i_estimate = curr.camera.intrinsics.estimate;

    vision_datas[index].feature.index              = feature->index;
    vision_datas[index].Qr.index                   = feature->group.Qr.index;
    vision_datas[index].Tr.index                   = feature->group.Tr.index;
    vision_datas[index].Q.index                    = state.Q.index;
    vision_datas[index].T.index                    = state.T.index;
    vision_datas[index].orig.Q.index               = orig.camera.extrinsics.Q.index;
    vision_datas[index].orig.T.index               = orig.camera.extrinsics.T.index;
    vision_datas[index].orig.focal_length.index    = orig.camera.intrinsics.focal_length.index;
    vision_datas[index].orig.center.index          = orig.camera.intrinsics.center.index;
    vision_datas[index].orig.k.index               = orig.camera.intrinsics.k.index;
    vision_datas[index].curr.Q.index               = curr.camera.extrinsics.Q.index;
    vision_datas[index].curr.T.index               = curr.camera.extrinsics.T.index;
    vision_datas[index].curr.focal_length.index    = curr.camera.intrinsics.focal_length.index;
    vision_datas[index].curr.center.index          = curr.camera.intrinsics.center.index;
    vision_datas[index].curr.k.index               = curr.camera.intrinsics.k.index;

    vision_datas[index].feature.initial_covariance             = feature->get_initial_covariance();
    vision_datas[index].Qr.initial_covariance                  = feature->group.Qr.get_initial_covariance();
    vision_datas[index].Tr.initial_covariance                  = feature->group.Tr.get_initial_covariance();
    vision_datas[index].Q.initial_covariance                   = state.Q.get_initial_covariance();
    vision_datas[index].T.initial_covariance                   = state.T.get_initial_covariance();
    vision_datas[index].orig.Q.initial_covariance              = orig.camera.extrinsics.Q.get_initial_covariance();
    vision_datas[index].orig.T.initial_covariance              = orig.camera.extrinsics.T.get_initial_covariance();
    vision_datas[index].orig.focal_length.initial_covariance   = orig.camera.intrinsics.focal_length.get_initial_covariance();
    vision_datas[index].orig.center.initial_covariance         = orig.camera.intrinsics.center.get_initial_covariance();
    vision_datas[index].orig.k.initial_covariance              = orig.camera.intrinsics.k.get_initial_covariance();
    vision_datas[index].curr.Q.initial_covariance              = curr.camera.extrinsics.Q.get_initial_covariance();
    vision_datas[index].curr.T.initial_covariance              = curr.camera.extrinsics.T.get_initial_covariance();
    vision_datas[index].curr.focal_length.initial_covariance   = curr.camera.intrinsics.focal_length.get_initial_covariance();
    vision_datas[index].curr.center.initial_covariance         = curr.camera.intrinsics.center.get_initial_covariance();
    vision_datas[index].curr.k.initial_covariance              = curr.camera.intrinsics.k.get_initial_covariance();

    vision_datas[index].feature.use_single_index           = feature->single_index();
    vision_datas[index].Qr.use_single_index                = feature->group.Qr.single_index();
    vision_datas[index].Tr.use_single_index                = feature->group.Tr.single_index();
    vision_datas[index].Q.use_single_index                 = state.Q.single_index();
    vision_datas[index].T.use_single_index                 = state.T.single_index();
    vision_datas[index].orig.Q.use_single_index            = orig.camera.extrinsics.Q.single_index();
    vision_datas[index].orig.T.use_single_index            = orig.camera.extrinsics.T.single_index();
    vision_datas[index].orig.focal_length.use_single_index = orig.camera.intrinsics.focal_length.single_index();
    vision_datas[index].orig.center.use_single_index       = orig.camera.intrinsics.center.single_index();
    vision_datas[index].orig.k.use_single_index            = orig.camera.intrinsics.k.single_index();
    vision_datas[index].curr.Q.use_single_index            = curr.camera.extrinsics.Q.single_index();
    vision_datas[index].curr.T.use_single_index            = curr.camera.extrinsics.T.single_index();
    vision_datas[index].curr.focal_length.use_single_index = curr.camera.intrinsics.focal_length.single_index();
    vision_datas[index].curr.center.use_single_index       = curr.camera.intrinsics.center.single_index();
    vision_datas[index].curr.k.use_single_index            = curr.camera.intrinsics.k.single_index();

    memcpy(vision_datas[index].dx_dp, dx_dp.data(), 2*sizeof(float));
    memcpy(vision_datas[index].dx_dQr, dx_dQr.data(), 6*sizeof(float));
    memcpy(vision_datas[index].dx_dTr, dx_dTr.data(), 6*sizeof(float));
    memcpy(vision_datas[index].dx_dQ, dx_dQ.data(), 6*sizeof(float));
    memcpy(vision_datas[index].dx_dT, dx_dT.data(), 6*sizeof(float));
    memcpy(vision_datas[index].orig.dx_dQ, orig.dx_dQ.data(), 6*sizeof(float));
    memcpy(vision_datas[index].orig.dx_dT, orig.dx_dT.data(), 6*sizeof(float));
    memcpy(vision_datas[index].orig.dx_dF, orig.dx_dF.data(), 2*sizeof(float));
    memcpy(vision_datas[index].orig.dx_dc, orig.dx_dc.data(), 4*sizeof(float));
    memcpy(vision_datas[index].orig.dx_dk, orig.dx_dk.data(), 8*sizeof(float));
    memcpy(vision_datas[index].curr.dx_dQ, curr.dx_dQ.data(), 6*sizeof(float));
    memcpy(vision_datas[index].curr.dx_dT, curr.dx_dT.data(), 6*sizeof(float));
    memcpy(vision_datas[index].curr.dx_dF, curr.dx_dF.data(), 2*sizeof(float));
    memcpy(vision_datas[index].curr.dx_dc, curr.dx_dc.data(), 4*sizeof(float));
    memcpy(vision_datas[index].curr.dx_dk, curr.dx_dk.data(), 8*sizeof(float));

    return &(vision_datas[index]);
}
#endif

void observation_accelerometer::predict()
{
    Rt = state.Q.v.conjugate().toRotationMatrix();
    Ra = extrinsics.Q.v.toRotationMatrix();
    v3 acc = root.world.up * root.g.v + state.a.v;
    xcc = Rt * acc;
    if(state.non_orientation.estimate)
        xcc += state.w.v.cross(state.w.v.cross(extrinsics.T.v)) + state.dw.v.cross(extrinsics.T.v);
    pred = Ra.transpose() * xcc + intrinsics.a_bias.v;
}

void observation_accelerometer::cache_jacobians()
{
    v3 acc = root.world.up * root.g.v + state.a.v;
    da_dacc = Ra.transpose() * Rt;
    da_dQ = Ra.transpose() * Rt * skew(acc);
    if(extrinsics.estimate)
    {
        da_dTa = Ra.transpose() * (skew(state.w.v) * skew(state.w.v) + skew(state.dw.v));
        da_dQa = Ra.transpose() * skew(xcc);
    }
    if (state.non_orientation.estimate) {
        da_ddw = Ra.transpose() * skew(-extrinsics.T.v);
        da_dw = Ra.transpose() * (skew(extrinsics.T.v.cross(state.w.v)) - skew(state.w.v) * skew(extrinsics.T.v));
    } else {
        da_ddw = m3::Zero();
        da_dw = m3::Zero();
        da_dTa = m3::Zero(); // FIXME: remove extrinsics.T from the state when orientation_only
    }
}

void observation_accelerometer::project_covariance(matrix &dst, const matrix &src) const
{
    int i=0;
    i = project_covariance<4>(dst,src,i);
    i = project_covariance<1>(dst,src,i);
}

template<int N>
int observation_accelerometer::project_covariance(matrix &dst, const matrix &src, int j) const
{
    for(; j < dst.cols()/N*N; j+=N) {
        const m<3,N> cov_a_bias = intrinsics.a_bias.from_row<N>(src, j);
        const m<3,N> scov_Q = state.Q.from_row<N>(src, j);
        const m<3,N> cov_a = state.a.from_row<N>(src, j);
        const m<3,N> cov_w = state.w.from_row<N>(src, j);
        const m<3,N> cov_dw = state.dw.from_row<N>(src, j);
        const m<1,N> cov_g = root.g.from_row<N>(src, j);
        col<N>(dst, j) = cov_a_bias + da_dQ * scov_Q + da_dw * cov_w + da_ddw * cov_dw + da_dacc * (cov_a + root.world.up * cov_g);
        if(extrinsics.estimate) {
            const m<3,N> scov_Qa = extrinsics.Q.from_row<N>(src, j);
            const m<3,N> cov_Ta = extrinsics.T.from_row<N>(src, j);
            col<N>(dst, j) += da_dQa * scov_Qa + da_dTa * cov_Ta;
        }
    }
    return j;
}

bool observation_accelerometer::measure()
{
    source.meas_stdev.data(meas);
    return observation_spatial::measure();
}

#ifdef ENABLE_SHAVE_PROJECT_OBSERVATION_COVARIANCE
observation_data* observation_accelerometer::getData(int index)
{
    __attribute__((section(".cmx_direct.bss")))
    static observation_accelerometer_data accel_data;
    accel_data.size = size;
    
    accel_data.a_bias.index    = intrinsics.a_bias.index;
    accel_data.Q.index         = state.Q.index;
    accel_data.a.index         = state.a.index;
    accel_data.w.index         = state.w.index;
    accel_data.dw.index        = state.dw.index;
    accel_data.g.index         = state.g.index;
    accel_data.eQ.index        = extrinsics.Q.index;
    accel_data.eT.index        = extrinsics.T.index;
    
    accel_data.a_bias.initial_covariance   = intrinsics.a_bias.get_initial_covariance();
    accel_data.Q.initial_covariance        = state.Q.get_initial_covariance();
    accel_data.a.initial_covariance        = state.a.get_initial_covariance();
    accel_data.w.initial_covariance        = state.w.get_initial_covariance();
    accel_data.dw.initial_covariance       = state.dw.get_initial_covariance();
    accel_data.g.initial_covariance        = state.g.get_initial_covariance();
    accel_data.eQ.initial_covariance       = extrinsics.Q.get_initial_covariance();
    accel_data.eT.initial_covariance       = extrinsics.T.get_initial_covariance();
    
    accel_data.a_bias.use_single_index = intrinsics.a_bias.single_index();
    accel_data.Q.use_single_index      = state.Q.single_index();
    accel_data.a.use_single_index      = state.a.single_index();
    accel_data.w.use_single_index      = state.w.single_index();
    accel_data.dw.use_single_index     = state.dw.single_index();
    accel_data.g.use_single_index      = state.g.single_index();
    accel_data.eQ.use_single_index     = extrinsics.Q.single_index();
    accel_data.eT.use_single_index     = extrinsics.T.single_index();
    
    memcpy(accel_data.da_dQ, da_dQ.data(), 9*sizeof(float));
    memcpy(accel_data.da_dw, da_dw.data(), 9*sizeof(float));
    memcpy(accel_data.da_ddw, da_ddw.data(), 9*sizeof(float));
    memcpy(accel_data.da_dacc, da_dacc.data(), 9*sizeof(float));
    memcpy(accel_data.worldUp, state.world.up.data(), 3*sizeof(float));
    memcpy(accel_data.da_dQa, da_dQa.data(), 9*sizeof(float));
    memcpy(accel_data.da_dTa, da_dTa.data(), 9*sizeof(float));
    
    accel_data.e_estimate = extrinsics.estimate;
    
    return &accel_data;
}
#endif

void observation_gyroscope::predict()
{
    Rw = extrinsics.Q.v.toRotationMatrix();
    pred = intrinsics.w_bias.v + Rw.transpose() * state.w.v;
}

void observation_gyroscope::cache_jacobians()
{
    if(extrinsics.estimate) {
        dw_dQw = Rw.transpose() * skew(state.w.v);
    }
}

void observation_gyroscope::project_covariance(matrix &dst, const matrix &src) const
{
    int i=0;
    i = project_covariance<4>(dst,src,i);
    i = project_covariance<1>(dst,src,i);
}

template<int N>
    int observation_gyroscope::project_covariance(matrix &dst, const matrix &src, int j) const
{
    for(; j < dst.cols()/N*N; j+=N) {
        const m<3,N> cov_w = state.w.from_row<N>(src, j);
        const m<3,N> cov_wbias = intrinsics.w_bias.from_row<N>(src, j);
        col<N>(dst, j) = cov_wbias + Rw.transpose() * cov_w;
        if(extrinsics.estimate) {
            const m<3,N> scov_Qw = extrinsics.Q.from_row<N>(src, j);
            col<N>(dst, j) += dw_dQw * scov_Qw;
        }
    }
    return j;
}

#ifdef ENABLE_SHAVE_PROJECT_OBSERVATION_COVARIANCE
observation_data* observation_gyroscope::getData(int index)
{  
    __attribute__((section(".cmx_direct.bss")))
    static observation_gyroscope_data gyro_data;
    gyro_data.size = size;
    
    gyro_data.w.index      = state.w.index;
    gyro_data.w_bias.index = intrinsics.w_bias.index;
    gyro_data.Q.index      = extrinsics.Q.index;
    
    gyro_data.w.initial_covariance         = state.w.get_initial_covariance();
    gyro_data.w_bias.initial_covariance    = intrinsics.w_bias.get_initial_covariance();
    gyro_data.Q.initial_covariance         = extrinsics.Q.get_initial_covariance();
    
    gyro_data.w.use_single_index       = state.w.single_index();
    gyro_data.w_bias.use_single_index  = intrinsics.w_bias.single_index();
    gyro_data.Q.use_single_index       = extrinsics.Q.single_index();
    
    memcpy(gyro_data.RwT, Rw.transpose().data(), 9*sizeof(float));
    memcpy(gyro_data.dw_dQw, dw_dQw.data(), 9*sizeof(float));
    
    gyro_data.e_estimate = extrinsics.estimate;
    
    return &gyro_data;
}
#endif

void observation_velocimeter::predict()
{
    pred = extrinsics.Q.v.toRotationMatrix().transpose() * ( state.Q.v.toRotationMatrix().transpose() * state.V.v ) + skew( - extrinsics.Q.v.toRotationMatrix().transpose() * extrinsics.T.v ) * extrinsics.Q.v.toRotationMatrix().transpose() * state.w.v;
}

void observation_velocimeter::cache_jacobians()
{
    dv_dQ = extrinsics.Q.v.toRotationMatrix().transpose() * state.Q.v.toRotationMatrix().transpose() * skew( state.V.v );
    dv_dw = skew( - extrinsics.Q.v.toRotationMatrix().transpose() * extrinsics.T.v ) * extrinsics.Q.v.toRotationMatrix().transpose();
    dv_dV = extrinsics.Q.v.toRotationMatrix().transpose() * state.Q.v.toRotationMatrix().transpose();

    if(extrinsics.estimate) {
        dv_dTv = extrinsics.Q.v.conjugate().toRotationMatrix() * skew(state.w.v);
        dv_dQv = skew(pred) * extrinsics.Q.v.conjugate().toRotationMatrix();
    }
}

void observation_velocimeter::compute_measurement_covariance()
{
    source.inn_stdev.data(inn);
    f_t ot = 8;
    m_cov = v3::Constant(std::max(inn.squaredNorm()/ot, source.measurement_variance));
}

void observation_velocimeter::project_covariance(matrix &dst, const matrix &src) const
{
    int i=0;
    i = project_covariance<4>(dst,src,i);
    i = project_covariance<1>(dst,src,i);
}

template<int N>
    int observation_velocimeter::project_covariance(matrix &dst, const matrix &src, int j) const
{
    for(; j < dst.cols()/N*N; ++j) {
        const m<3,N> scov_Q = state.Q.from_row<N>(src, j);
        const m<3,N> cov_w = state.w.from_row<N>(src, j);
        const m<3,N> cov_V = state.V.from_row<N>(src, j);

        col<N>(dst, j) = dv_dQ * scov_Q + dv_dw * cov_w + dv_dV * cov_V;

        if (extrinsics.estimate) {
            const m<3,N> scov_Qv = extrinsics.Q.from_row<N>(src, j);
            const m<3,N> cov_Tv = extrinsics.T.from_row<N>(src, j);
            col<N>(dst,j) += dv_dQv * scov_Qv + dv_dTv * cov_Tv;
        }
    }
    return j;
}

#ifdef ENABLE_SHAVE_PROJECT_OBSERVATION_COVARIANCE
observation_data* observation_velocimeter::getData(int index)
{
    __attribute__((section(".cmx_direct.bss")))
    static observation_velocimeter_data velo_data;
    velo_data.size = size;

    velo_data.Q.index   = state.Q.index;
    velo_data.w.index   = state.w.index;
    velo_data.V.index   = state.V.index;
    velo_data.eQ.index  = extrinsics.Q.index;
    velo_data.eT.index  = extrinsics.Q.index;

    velo_data.Q.initial_covariance  = state.Q.get_initial_covariance();
    velo_data.w.initial_covariance  = state.w.get_initial_covariance();
    velo_data.V.initial_covariance  = state.V.get_initial_covariance();
    velo_data.eQ.initial_covariance = extrinsics.Q.get_initial_covariance();
    velo_data.eT.initial_covariance = extrinsics.T.get_initial_covariance();

    velo_data.Q.use_single_index    = state.Q.single_index();
    velo_data.w.use_single_index    = state.w.single_index();
    velo_data.V.use_single_index    = state.V.single_index();
    velo_data.eQ.use_single_index   = extrinsics.Q.single_index();
    velo_data.eT.use_single_index   = extrinsics.T.single_index();

    velo_data.dv_dQ     = dv_dQ.data();
    velo_data.dv_dV     = dv_dV.data();
    velo_data.dv_dw     = dv_dw.data();
    velo_data.dv_dTv    = dv_dTv.data();
    velo_data.dv_dQv    = dv_dQv.data();

    memcpy(velo_data.dv_dQ, dv_dQ.data(), 9*sizeof(float));
    memcpy(velo_data.dv_dV, dv_dV.data(), 9*sizeof(float));
    memcpy(velo_data.dv_dw, dv_dw.data(), 9*sizeof(float));
    memcpy(velo_data.dv_dTv, dv_dTv.data(), 9*sizeof(float));
    memcpy(velo_data.dv_dQv, dv_dQv.data(), 9*sizeof(float));

    velo_data.e_estimate = extrinsics.estimate;

    return &velo_data;
}
#endif
