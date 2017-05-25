/**
 * File: FeatureVector.h
 * Date: November 2011
 * Author: Dorian Galvez-Lopez
 * Description: feature vector
 * License: see the LICENSE.txt file
 *
 */

#ifndef __D_T_FEATURE_VECTOR__
#define __D_T_FEATURE_VECTOR__

#include "BowVector.h"
#include <map>
#include <vector>
#include <iostream>

namespace DBoW2 {

/// Vector of nodes with indexes of local features
class FeatureVector: 
  public std::map<NodeId, std::vector<unsigned int> >
{
public:
    void addFeature(NodeId id, unsigned int i_feature);
};

}

#endif

