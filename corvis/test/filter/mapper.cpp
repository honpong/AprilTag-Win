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
    transformation offset;
    bool result = map.find_closure(max, suppression, offset);
    EXPECT_TRUE(result);
    EXPECT_QUATERNION_NEAR(g.Q, offset.Q, 4*F_T_EPS);
    EXPECT_V4_NEAR(g.T, offset.T, 4*F_T_EPS);
}

TEST(Mapper, T)
{
    mapper map;
    transformation g(quaternion(), v4(0.1,1.2,0.3,0));
    fill_map_two_nodes(map, g);
    int max = 20;
    int suppression = 2;
    transformation offset;
    bool result = map.find_closure(max, suppression, offset);
    EXPECT_TRUE(result);
    EXPECT_QUATERNION_NEAR(g.Q, offset.Q, 4*F_T_EPS);
    EXPECT_V4_NEAR(g.T, offset.T, 4*F_T_EPS);
}

TEST(Mapper, Serialize)
{
    mapper map;
    transformation g;
    fill_map_two_nodes(map, g);
    std::string json;
    bool result = map.serialize(json);
    EXPECT_TRUE(result);
    EXPECT_TRUE(json.length() > 0);
    mapper map2;
    mapper::deserialize(json, map2);
    const vector<map_node> & nodes1 = map.get_nodes();
    const vector<map_node> & nodes2 = map2.get_nodes();

    EXPECT_EQ(nodes1.size(), nodes2.size());
    for(int i = 0; i < nodes1.size(); i++) {
        EXPECT_EQ(nodes1[i].features.size(), nodes2[i].features.size());
        EXPECT_EQ(nodes1[i].edges.size(), nodes2[i].edges.size());
        auto f1 = nodes1[i].features.begin();
        auto f2 = nodes2[i].features.begin();
        for( ; f1 != nodes1[i].features.end() || f2 != nodes2[i].features.end(); ++f1, ++f2) {
            for(int j = 0; j < descriptor_size; j++) {
                float d1 = (*f1)->dvec(j);
                float d2 = (*f2)->dvec(j);
                EXPECT_EQ(d1, d2);
            }
        }
    }
}
