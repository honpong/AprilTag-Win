#pragma once

#include <random>

// Swap n random items to the front of the given sequence returning the end iterator
template <typename ForwardIt, typename Generator>
ForwardIt random_n(ForwardIt begin, ForwardIt end, size_t n, Generator &gen) {
    auto r = begin;
    for (; r != end && n; ++r, --n) {
        std::uniform_int_distribution<size_t> rand_in_rest(0, end - r - 1); // [0,end-r)
        std::iter_swap(r, r + rand_in_rest(gen));
    }
    return r;
}

// Models should compare by their fitness and allow being called to determine if the argument "fits the model well enough"
template <size_t N, typename Model, typename State, typename ForwardIt, typename Generator>
    Model ransac(State &state, ForwardIt begin, ForwardIt end, Generator &gen, int max_iterations = 10,
                 float confidence = .99f, unsigned min_matches = 1, unsigned max_matches = std::numeric_limits<unsigned>::max()) {
    static_assert(N > 0,"N>0"); assert(begin + N <= end); assert(0 <= confidence && confidence < 1);
    Model best_model(state, begin, begin);
    for (int n=max_iterations, i = 0; i < n; i++) {
        // Split [begin,end) into up to N random maybe_inliers and the rest: [begin,maybe_inliers) and [maybe_inliers, end)
        auto maybe_inliers = random_n(begin, end, N, gen);
        Model maybe_model(state, begin, maybe_inliers);
        // Split [maybe_inliers, end) into [maybe_inliers, also_inliers) and [also_inliers, end)
        auto also_inliers = maybe_inliers;
        for (auto mi = maybe_inliers; mi != end; ++mi)
            if (maybe_model(state, mi)) // if the maybe-inlier *mi fits the model well enough add it to also_inliers
                std::iter_swap(also_inliers++, mi);
        if (also_inliers - begin >= min_matches) {
            if (also_inliers == maybe_inliers) {
                if (best_model < maybe_model)
                    best_model = std::move(maybe_model); // FIXME: remove this case?  Seems like a pointless optimization
            } else {
                Model maybe_better_model(state, begin, also_inliers);
                if (best_model < maybe_better_model)
                    best_model = std::move(maybe_better_model);
            }
        }
        if (also_inliers - begin >= max_matches)
            break;
        float inlier_fraction = (float)(also_inliers-begin)/(end-begin);
        float n_ = logf(1-confidence)/logf(1 - powf(inlier_fraction,N));
        n = std::min<int>(std::round(n_), n);
    }
    return best_model;
}
