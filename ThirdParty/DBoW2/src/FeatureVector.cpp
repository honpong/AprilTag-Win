/**
 * File: FeatureVector.cpp
 * Date: November 2011
 * Author: Dorian Galvez-Lopez
 * Description: feature vector
 * License: see the LICENSE.txt file
 *
 */

#include "DBoW2/FeatureVector.h"
//#include <map>
//#include <vector>
//#include <iostream>

namespace DBoW2 {

void FeatureVector::addFeature(NodeId id, unsigned int i_feature) //g
{
  FeatureVector::iterator vit = this->lower_bound(id);
  
  if(vit != this->end() && vit->first == id)
  {
    vit->second.push_back(i_feature);
  }
  else
  {
    vit = this->insert(vit, FeatureVector::value_type(id, std::vector<unsigned int>()));
    vit->second.push_back(i_feature);
  }
}

} // namespace DBoW2
