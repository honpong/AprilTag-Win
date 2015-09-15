#include "gtest/gtest.h"
#include "mapper.h"
#include "corvis_dictionary.h"
#include "util.h"

void fill_map_two_nodes(mapper & map, const transformation & g)
{
    map.add_node(0);
    map.add_node(1);
    map.add_node(2); // add a few empty nodes to make tf-idf happy
    map.add_node(3);
    map.add_node(4);
    for(int i = 0; i < corvis_num_centers; i++) {
        const float variance = 0.1*0.1;
        descriptor d;
        v4 position(i, i % 4, 2, 0);
        memcpy(d.d, corvis_centers + corvis_dimension*i, corvis_dimension*sizeof(float));
        map.add_feature(0, i, position, variance, d);
        map.add_feature(1, i+corvis_num_centers, g*position, variance, d);
    }
    transformation Gidentity;
    map.node_finished(0, Gidentity);
    map.node_finished(1, Gidentity);
    map.node_finished(2, Gidentity);
    map.node_finished(3, Gidentity);
    map.node_finished(4, Gidentity);
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
    EXPECT_EQ(matches[0].to, 0);
    EXPECT_QUATERNION_NEAR(g.Q, matches[0].g.Q, 4*F_T_EPS);
    EXPECT_V4_NEAR(g.T, matches[0].g.T, 4*F_T_EPS);
}

TEST(Mapper, T)
{
    mapper map;
    transformation g(quaternion(), v4(0.1,1.2,0.3,0));
    fill_map_two_nodes(map, g);
    int max = 20;
    int suppression = 2;
    vector<map_match> matches;
    bool result = map.get_matches(1, matches, max, suppression);
    EXPECT_TRUE(result);
    EXPECT_TRUE(matches.size() > 0);
    EXPECT_EQ(matches[0].to, 0);
    EXPECT_QUATERNION_NEAR(g.Q, matches[0].g.Q, 4*F_T_EPS);
    EXPECT_V4_NEAR(g.T, matches[0].g.T, 4*F_T_EPS);
}
