#pragma once

#include <vector>

class histogram
{
public:
    std::vector<unsigned int> v;
    histogram(unsigned int max): v(max, 0) {}
    void data(unsigned int x) { if(x >= v.size()) ++v[v.size()-1]; else ++v[x]; }
    friend std::ostream& operator<<(std::ostream &stream, const histogram &h) {
        for(unsigned int i = 0; i < h.v.size(); ++i)
            stream << i << " " << h.v[i] << "\n";
        return stream;
    }
};
