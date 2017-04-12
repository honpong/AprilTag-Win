#include "covariance.h"

void remapper::remap_vector(int size, matrix &Pn_to, const matrix &Pn_from) {
    compile();
    Pn_to.resize(std::max(Pn_from.cols(),size)); // in case &Pn_to == &Pn_from
    for (auto &c : updates)
        if (c.type == update::add)
            std::memcpy(&Pn_to[c.to], &process_covariance[c.to], c.size*sizeof(process_covariance[c.to]));
        else if (&Pn_to != &Pn_from || c.to != c.from)
            std::memmove(&Pn_to[c.to], &Pn_from[c.from], c.size*sizeof(Pn_from[0]));
    Pn_to.resize(size);
}

void remapper::remap_matrix(int size, matrix &P_to, const matrix &P_from) {
    compile();
    P_to.resize(std::max(P_from.rows(),size), std::max(P_from.cols(),size)); // in case &P_to == &P_from
    for (auto &r : updates) {
        if (r.from <= r.to && r.to < r.from+r.size) {
            for (int i=r.size-1; i>=0; i--)
                if (r.type == update::add) {
                    P_to.row_segment(r.to+i,0, i) = 0;
                    P_to            (r.to+i,r.to+i) = initial_covariance[r.to+i];
                    P_to.row_segment(r.to+i,r.to+i+1, size-(r.to+i+1)) = 0;
                } else {
                    for (auto &c : updates)
                        if (c.type == update::add)
                            P_to.row_segment(r.to+i,c.to, c.size) = 0;
                        else if (&P_to != &P_from || r.from != r.to || c.from != c.to)
                            P_to.row_segment(r.to+i,c.to, c.size) = P_from.row_segment(r.from+i,c.from, c.size);
                }
        } else {
            for (int i=0; i<r.size; i++)
                if (r.type == update::add) {
                    P_to.row_segment(r.to+i,0, r.to+i) = 0;
                    P_to            (r.to+i,r.to+i) = initial_covariance[r.to+i];
                    P_to.row_segment(r.to+i,r.to+i+1, size-(r.to+i+1)) = 0;
                } else
                    for (auto &c : updates)
                        if (c.type == update::add)
                            P_to.row_segment(r.to+i,c.to, c.size) = 0;
                        else if (&P_to != &P_from || r.from != r.to || c.from != c.to)
                            P_to.row_segment(r.to+i,c.to, c.size) = P_from.row_segment(r.from+i,c.from, c.size);
        }
    }
    P_to.resize(size, size);
}
