#ifndef __UTIL_H
#define __UTIL_H

static inline void test_m4_near(const m4 &a, const m4 &b, const f_t bounds)
{
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
#ifdef F_T_IS_DOUBLE
            EXPECT_NEAR(a[i][j], b[i][j], bounds) << "Where i is " << i << " and j is " << j;
#else
            EXPECT_NEAR(a[i][j], b[i][j], bounds) << "Where i is " << i << " and j is " << j;
            
#endif
        }
    }
}

static inline void test_v4_near(const v4 &a, const v4 &b, const f_t bounds)
{
    for(int i = 0; i < 4; ++i) {
#ifdef F_T_IS_DOUBLE
        EXPECT_NEAR(a[i], b[i], bounds) << "Where i is " << i;
#else
        EXPECT_NEAR(a[i], b[i], bounds) << "Where i is " << i;
        
#endif
    }
}

#endif