#include "gtest/gtest.h"

#include "dictionary.h"
#include <vector>
using namespace std;

vector<descriptor> generate_training_data()
{
    size_t num_data = 100;
    vector<descriptor> train_data;
    for(size_t i = 0; i < num_data; i++) {
        descriptor d;
        for(int j = 0; j < descriptor_size; j++) {
            d.d[j] = i;
        }
        train_data.push_back(d);
    }
    return train_data;
}

TEST(Dictionary, Train)
{
    vector<descriptor> train_data = generate_training_data();

    const size_t dict_size = 2;
    dictionary dict(train_data, dict_size);

    int hist[dict_size];
    for(size_t i = 0; i < dict_size; i++) hist[i] = 0;

    for(auto point : train_data) {
        int id = dict.quantize(point);
        hist[id]++;
    }

    EXPECT_EQ(hist[0], hist[1]);
}

TEST(Dictionary, WriteRead)
{
    vector<descriptor> train_data = generate_training_data();

    int dict_size = 5;
    dictionary dict(train_data, dict_size);

    dict.write("test.dictionary");
    dictionary dict2("test.dictionary");
    EXPECT_EQ(dict_size, dict2.get_num_centers());

    for(auto point : train_data) {
        int id1 = dict.quantize(point);
        int id2 = dict2.quantize(point);
        EXPECT_EQ(id1, id2);
    }
}
