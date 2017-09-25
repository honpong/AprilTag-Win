#pragma once

template <int MAXSTATESIZE_, int MAXOBSERVATIONSIZE_ = 2*MAXSTATESIZE_, int MAXFAKESTATESIZE_ = 6>
class kalman_storage {
  public:
    static constexpr int maxstatesize = MAXSTATESIZE_;
    static constexpr int maxobservationsize = MAXOBSERVATIONSIZE_;
    static constexpr int maxfakestatesize = MAXFAKESTATESIZE_;

    matrix iP { iP_storage };
    matrix iQ { iQ_storage };
    matrix x  {  x_storage };
    matrix P  {  P_storage };
    matrix Q  {  Q_storage };
    matrix FP { FP_storage };
    matrix R  {  R_storage };
    matrix y  {  y_storage };
    matrix HP { HP_storage };
    matrix S  {  S_storage };

  protected:
    static constexpr int a = EIGEN_MAX_ALIGN_BYTES;
    static constexpr size_t padded(size_t s) { return (s + a - 1) & ~(a-1); }
    alignas(a) f_t iP_storage[maxstatesize];
    alignas(a) f_t iQ_storage[maxstatesize];
    alignas(a) f_t  x_storage[maxstatesize];
    alignas(a) f_t  P_storage[maxstatesize+1/*x*/][padded(maxstatesize)];
    alignas(a) f_t  Q_storage[maxstatesize];
    union {
    alignas(a) f_t FP_storage[maxstatesize][padded(maxstatesize + maxfakestatesize)];
    struct {
    alignas(a) f_t  R_storage[maxobservationsize];
    alignas(a) f_t  y_storage[maxobservationsize];
    alignas(a) f_t HP_storage[maxobservationsize][padded(maxstatesize+1/*y*/)];
    alignas(a) f_t  S_storage[maxobservationsize][padded(maxobservationsize)];
    };
    };
};
