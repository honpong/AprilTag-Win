#include "gtest/gtest.h"
#include "covariance.h"
#include "remapper.h"
#include <random>
#include <limits>
#include <cctype>

namespace rc { namespace testing {


static int primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61,
                       67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137,
                       139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211,
                       223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283,
                       293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379,
                       383, 389, 397, 401, 409, 419, 421, 431, 433, 439, 443, 449, 457, 461,
                       463, 467, 479, 487, 491, 499, 503, 509, 521, 523, 541}; // Prime /@ Range[1, 100]

template<int size_> static m<size_,size_> special_cov(int n) {
    m<size_,size_> m;
    for (int i=0; i<size_; i++)
        for (int j=0; j<size_; j++)
            m(i,j) = primes[n+j]*primes[n+i];
    return m;
}

template<int size_> static v<size_> special_noise(int n) {
    v<size_> v;
    for (int i=0; i<size_; i++)
        v(i) = primes[n+i];
    return v;
}

struct state {
    struct item { int pindex, size, index; bool in, was_in; };
    int max_size = 0; bool in;
    std::vector<item> items;
    matrix *mat, *vec;
    const char *str;
    state(const char *initial, const char *old = nullptr) : str(initial) {
        int current = -1, index = 0;
        for (int i=0; initial[i]; i++, max_size++) {
            bool in = (bool)islower(initial[i]), was_in = old ? (bool)islower(old[i]) : true;
            if (current == initial[i])
                items.back().size++;
            else
                items.push_back(item{max_size, 1, in ? index : -1, in, was_in});
            current = initial[i];
            if (in)
                index++;
        }
        mat = new matrix(max_size, max_size);
        vec = new matrix(max_size);

        {
            int size=0;
            for (auto &r : items) {
                if (r.in)
                    for (int i=0; i<r.size; i++, size++)
                        (*vec)[size] = primes[r.pindex+i];
            }
            vec->resize(size);
            mat->resize(size, size);
        }

        {
            int rsize = 0;
            for (auto &r : items)
                if (r.in)
                    for (int i=0; i<r.size; i++, rsize++) {
                        int csize = 0;
                        for (auto &c : items)
                            if (c.in)
                                for (int j=0; j<c.size; j++, csize++)
                                    (*mat)(rsize,csize) = (c.was_in && r.was_in) || rsize == csize ? primes[r.pindex+i]*primes[c.pindex+j] : 0;
                    }
        }
    }
    void print() {
        int i=0;
        for (auto &r : items)
            std::cout << i++ <<  ") " << r.pindex << " " << r.size << " " << r.index << " " << r.in << "\n";
    }
    void remap(const char *new_state) {
        state ref(new_state, str);
        ASSERT_STRCASEEQ(ref.str, str);
        ASSERT_EQ(ref.items.size(), items.size()) << ref.str << " != " << str;

        matrix iP(1,100), iQ(1,100);
        remapper rm(100, iP, iQ);

        int i=0;
        for (int j=0; j< items.size() && j<ref.items.size(); j++) {
            auto &r = items[j];
            if (ref.items[j].in) {
                if (r.in)
                    rm.reindex(i, r.index, r.size);
                else {
                    switch(r.size) {
                    case 1: rm.add<1>(i, special_noise<1>(r.pindex), special_cov<1>(r.pindex)); break;
                    case 2: rm.add<2>(i, special_noise<2>(r.pindex), special_cov<2>(r.pindex)); break;
                    case 3: rm.add<3>(i, special_noise<3>(r.pindex), special_cov<3>(r.pindex)); break;
                    case 4: rm.add<4>(i, special_noise<4>(r.pindex), special_cov<4>(r.pindex)); break;
                    case 5: rm.add<5>(i, special_noise<5>(r.pindex), special_cov<5>(r.pindex)); break;
                    case 6: rm.add<6>(i, special_noise<6>(r.pindex), special_cov<6>(r.pindex)); break;
                    case 7: rm.add<7>(i, special_noise<7>(r.pindex), special_cov<7>(r.pindex)); break;
                    case 8: rm.add<8>(i, special_noise<8>(r.pindex), special_cov<8>(r.pindex)); break;
                    default: ASSERT_FALSE("add more cases") << "size = " << r.size;
                    }
                }
                r.index = i;
                i+=r.size;
            } else
                r.index = -1;
        }

        int size = i;

        matrix v_remapped_into(max_size);
        rm.remap_vector(size, v_remapped_into, *vec);
        EXPECT_EQ(v_remapped_into.map(), ref.vec->map()) << str << " -> " << ref.str << " " << v_remapped_into.map() << " -> " << ref.vec->map();

        matrix m_remapped_into(max_size, max_size);
        rm.remap_matrix(size, m_remapped_into, *mat);
        EXPECT_EQ(m_remapped_into.map(), ref.mat->map()) << str << " -> " << ref.str << " " << m_remapped_into.map() << " -> " << ref.mat->map();

        rm.remap_vector(size, *vec);
        EXPECT_EQ(vec->map(), ref.vec->map()) << str << " -> " << ref.str << " " << vec->map() << " -> " << ref.vec->map();

        rm.remap_matrix(size, *mat);
        EXPECT_EQ(mat->map(), ref.mat->map()) << str << " -> " << ref.str << " " << mat->map() << " != " << ref.mat->map();
    }
};

TEST(Remapper, Remap)
{
    { state s("aaBBccccDDDDeeeee"); s.remap("aaBBccccDDDDeeeee"); }
    { state s("aaBBccccDDDDeeeee"); s.remap("aaBBCCCCDDDDeeeee"); }
    { state s("aaBBccccDDDDeeeee"); s.remap("aabbCCCCddddeeeee"); }
    { state s("aaBBccccDDDDeeeee"); s.remap("aabbccccddddeeeee"); }
    { state s("aaabbbbbbbbccddeeee"); s.remap("aaaBBBBBBBBccDDeeee"); }
    { state s("aaaXXXXXXXXbbYYcccc"); s.remap("aaaxxxxxxxxbbyycccc"); }
    { state s("aaaXbbcdddYZZeeef"); s.remap("aaaxBBcdddyzzeeef"); }
    { state s("aaaXbbcdddYZZZZZeee"); s.remap("AAAxBBcdddyzzzzzeee"); }
    { state s("aaaXbbcYdddeeeZZf"); s.remap("AAAxBBCyDDDeeezzf"); }
    { state s("ABcdEfgHij"); s.remap("abcdeFghIj"); }
    { state s("XXXXaBcdeFghhhh"); s.remap("xxxxabCdEfgHHHH"); }
}

} /*testing*/ } /*rc*/
