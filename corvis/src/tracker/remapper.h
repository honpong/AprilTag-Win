#pragma once

#include <vec4.h>
#include <matrix.h>

class remapper {
    v<MAXSTATESIZE> initial_covariance, process_covariance;

    struct update {
        enum type : uint16_t { move, add } type;
        uint16_t size, to, from;
        update(enum type type_, uint16_t size_, uint16_t to_                ): type(type_), size(size_), to(to_), from(-1   ) { assert(type_ ==  add); }
        update(enum type type_, uint16_t size_, uint16_t to_, uint16_t from_): type(type_), size(size_), to(to_), from(from_) { assert(type_ == move); }
        bool operator <(const update &b) { // return if this *must* happen before other
            return type == move && (b.type != move || to != b.to)
                // [from,from+size) overlaps [b.to,b.to+b.size)
                && from < b.to + b.size && b.to < from + size;
        }
    };

    bool compiled = false;
    std::vector <update> updates;

  public:
    remapper() {
        updates.reserve(MAXSTATESIZE);
    }

    void reset() {
        compiled = false;
        updates.clear();
    }

    template<int size>
    void add(int newindex, const v<size> &process_covariance_, const m<size, size> &initial_covariance_) {
        assert(updates.empty() ? newindex == 0 : updates.back().to + updates.back().size == newindex);

        if (updates.size() &&   updates.back().type == update::add &&
            updates.back().to + updates.back().size == newindex)
            updates.back().size += size; // coalesce adjacent adds
        else
            updates.emplace_back(update::add, size, newindex);

        initial_covariance.segment<size>(newindex) = initial_covariance_.diagonal();
        process_covariance.segment<size>(newindex) = process_covariance_;
    };

    void reindex(int newindex, int oldindex, int size) {
        assert(updates.empty() ? newindex == 0 : updates.back().to + updates.back().size == newindex);

        if (updates.size() &&     updates.back().type == update::move &&
            updates.back().to   + updates.back().size == newindex &&
            updates.back().from + updates.back().size == oldindex)
            updates.back().size += size; // coalesce adjacent moves
        else
            updates.emplace_back(update::move, size, newindex, oldindex);
    }

    void compile() {
        if (compiled) return; else compiled = true;

        // Assume updates[] initially ordered by .to w/ no holes
        { int   to_end = 0; for (const auto &u : updates) { assert(to_end == u.to); to_end = u.to+u.size; } }

        // Assume moves create no cycles (i.e. we *can* sort below)
        { int from_end = 0; for (const auto &u : updates) if (u.type == update::move) { assert(from_end <= u.from); from_end = u.from+u.size; } }

        // Sort based on operator< (i.e. "must happen before"); can't use std::sort because operator< is not transitive.
        for (auto unswapped = updates.begin(), i = updates.begin(); i != updates.end(); ++i) {
          again:
            for (auto j=i+1; j != updates.end(); ++j) {
                // avoid O(n^2) comparisons by not checking (unswapped) items that can't satisfy operator<
                if (j >= unswapped && (j->type == update::move && j->from >= i->to + i->size))
                    break; // => !(*k < *i) for all k >= j
                if (*j < *i) {
                    if (unswapped <= j+1) unswapped = j+1; // ensure [unswapped,updates.end()) remains ordered w.r.t ->to
                    std::iter_swap(i,j);
                    goto again;
                }
            }
        }
    }

    void remap_vector(int size, matrix &Pn) {
        remap_vector(size, Pn, Pn);
    }

    void remap_vector(int size, matrix &Pn_to, const matrix &Pn_from);

    void remap_matrix(int size, matrix &P) {
        remap_matrix(size, P, P);
    }

    void remap_matrix(int size, matrix &P_to, const matrix &P_from);
};
