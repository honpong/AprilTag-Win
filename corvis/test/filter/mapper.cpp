#include "gtest/gtest.h"
#include "mapper.h"
#include "corvis_dictionary.h"
#include "util.h"

void finish_second_node(mapper & map, const transformation & g_node)
{
    map.node_finished(4, g_node);
}

void fill_map_two_nodes(mapper & map, const transformation & g_first, const transformation & g_feature)
{
    map.add_node(0);
    map.add_node(1);
    map.add_node(2); // add a few empty nodes to make tf-idf happy
    map.add_node(3);
    map.add_node(4);
    map.add_edge(0,1);
    map.add_edge(1,2);
    map.add_edge(2,3);
    map.add_edge(3,4);
    for(int i = 0; i < corvis_num_centers; i++) {
        const float variance = 0.1*0.1;
        descriptor d;
        v3 position(i, i % 4, 2);
        memcpy(d.d, corvis_centers + corvis_dimension*i, corvis_dimension*sizeof(float));

        float norm = 0;
        for(int j = 0; j < corvis_dimension; j++)
            norm += d.d[j]*d.d[j];
        norm = sqrt(norm);

        for(int j = 0; j < corvis_dimension; j++)
            d.d[j] /= norm;

        // to transform features to the frame, we should use
        // the inverse of the transform of the camera
        map.add_feature(0, i, invert(g_first)*position, variance, d);
        map.add_feature(4, i+corvis_num_centers, invert(g_feature)*position, variance, d);
    }
    map.node_finished(0, g_first);
    map.node_finished(1, g_first);
    map.node_finished(2, g_first);
    map.node_finished(3, g_first);
    // need 10 nodes of padding for find_closure to try a node
    for(int i = 5; i < 5 + 10; i++) {
        map.add_node(i);
        map.node_finished(i, g_first);
    }
}

void transformation_test(const transformation & g_first, const transformation & g_second, const transformation & g_apparent)
{
    mapper map;
    fill_map_two_nodes(map, g_first, g_second);
    int max = 20;
    int suppression = 2;
    transformation offset, relative;
    bool result;

    // This should not find any closures
    // We do this to make sure we find the closure 4-0 instead of 0-4
    result = map.find_closure(max, suppression, offset);
    EXPECT_FALSE(result);

    finish_second_node(map, g_apparent);

    result = map.find_closure(max, suppression, offset);

    transformation expected = g_second*invert(g_apparent);
    EXPECT_TRUE(result);
    EXPECT_QUATERNION_NEAR(expected.Q, offset.Q, 4*F_T_EPS);
    EXPECT_V3_NEAR(expected.T, offset.T, 1e-5);
}

TEST(Mapper, I)
{
    transformation_test(transformation(), transformation(), transformation());
}

TEST(Mapper, T)
{
    transformation_test(transformation(), transformation(quaternion(), v3(0.1,1.2,0.3)), transformation());
}

TEST(Mapper, Drift)
{
    transformation g_first(quaternion(), v3(-0.8,-0.2,0.5));
    transformation g_second(quaternion(), v3(0.1,1.2,0.3));
    transformation g_apparent(quaternion(), v3(0.5, -0.7, 4.3));
    transformation_test(g_first, g_second, g_apparent);
}

TEST(Mapper, Serialize)
{
    mapper map;
    transformation g;
    fill_map_two_nodes(map, g, g);
    finish_second_node(map, g);
    std::string json;
    bool result = map.serialize(json);
    EXPECT_TRUE(result);
    EXPECT_TRUE(json.length() > 0);
    mapper map2;
    mapper::deserialize(json, map2);
    const aligned_vector<map_node> & nodes1 = map.get_nodes();
    const aligned_vector<map_node> & nodes2 = map2.get_nodes();

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
