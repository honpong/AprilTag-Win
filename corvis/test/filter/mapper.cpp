#include "gtest/gtest.h"
#include "mapper.h"
#include "corvis_dictionary.h"
#include "util.h"

void fill_map_two_nodes(mapper & map, transformation & g)
{
    transformation n1;
    transformation n2 = g;
    map.add_node(0, n1);
    map.add_node(1, n2);
    map.add_node(2, n2); // add a few empty nodes to make tf-idf happy
    map.add_node(3, n2);
    map.add_node(4, n2);
    for(int i = 0; i < corvis_num_centers; i++) {
        const float variance = 0.1*0.1;
        descriptor d;
        v4 position(i, i % 4, 2, 0);
        memcpy(d.d, corvis_centers + corvis_dimension*i, corvis_dimension*sizeof(float));
        map.add_feature(0, i, position, variance, d);
        map.add_feature(1, i+corvis_num_centers, g*position, variance, d);
    }

}

TEST(Mapper, I)
{
    mapper map;
    transformation g;
    fill_map_two_nodes(map, g);
    int max = 20;
    int suppression = 2;
    vector<map_match> matches;
    bool result = map.get_matches(1, matches, max, suppression);
    EXPECT_TRUE(result);
    EXPECT_TRUE(matches.size() > 0);
    EXPECT_EQ(matches[0].id, 0);
    EXPECT_QUATERNION_NEAR(g.Q, matches[0].g.Q, 4*F_T_EPS);
    EXPECT_V4_NEAR(g.T, matches[0].g.T, 4*F_T_EPS);
}

TEST(Mapper, RT)
{
    mapper map;
    transformation g(rotation_vector(0,0,0.3456), v4(0.1,1.2,0.3,0));
    fill_map_two_nodes(map, g);
    int max = 20;
    int suppression = 2;
    vector<map_match> matches;
    bool result = map.get_matches(1, matches, max, suppression);
    EXPECT_TRUE(result);
    EXPECT_TRUE(matches.size() > 0);
    EXPECT_EQ(matches[0].id, 0);
    EXPECT_QUATERNION_NEAR(g.Q, matches[0].g.Q, 4*F_T_EPS);
    EXPECT_V4_NEAR(g.T, matches[0].g.T, 4*F_T_EPS);
}
