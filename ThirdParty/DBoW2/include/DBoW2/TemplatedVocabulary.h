/**
 * File: TemplatedVocabulary.h
 * Date: February 2011
 * Author: Dorian Galvez-Lopez
 * Description: templated vocabulary 
 * License: see the LICENSE.txt file
 *
 */

#ifndef __D_T_TEMPLATED_VOCABULARY__
#define __D_T_TEMPLATED_VOCABULARY__

#include <cassert>

#include <vector>
#include <numeric>
#include <cstring>
#include <algorithm>

#include "FeatureVector.h"
#include "BowVector.h"
#include "ScoringObject.h"

#pragma warning (disable : 4996)
#if !defined(WIN32) &&!defined(WIN64)
#pragma GCC diagnostic ignored "-Wunused-result"
#endif

namespace DBoW2 {

/// @param TDescriptor class of descriptor
/// @param F class of descriptor functions
template<class TDescriptor, class F>
/// Generic Vocabulary
class TemplatedVocabulary
{		
public:
  TemplatedVocabulary(int k = 10, int L = 5, 
    WeightingType weighting = TF_IDF, ScoringType scoring = L1_NORM);
  
  /**
   * Destructor
   */
  virtual ~TemplatedVocabulary();  
  
  /**
   * Returns the number of words in the vocabulary
   * @return number of words
   */
  virtual inline unsigned int size() const;
  
  /**
   * Returns whether the vocabulary is empty (i.e. it has not been trained)
   * @return true iff the vocabulary is empty
   */
  virtual inline bool empty() const;
  
  virtual int transform(const std::vector<TDescriptor>& features,
      BowVector &v, FeatureVector &fv, int levelsup) const;

  inline double score(const BowVector &a, const BowVector &b) const;
  
  bool loadFromTextFile(const std::string &filename);

  bool loadFromMemory(const char *pBinaries, size_t size);

  ScoringType getScoringType() { return m_scoring; }
 
protected:

  /// Pointer to descriptor
  typedef const TDescriptor *pDescriptor;

  /// Tree node
  struct Node 
  {
    /// Node id
    NodeId id;
    /// Weight if the node is a word
    WordValue weight;
    /// Children 
    std::vector<NodeId> children;
    /// Parent node (undefined in case of root)
    NodeId parent;
    /// Node descriptor
    TDescriptor descriptor;

    /// Word id if the node is a word
    WordId word_id;

    /**
     * Empty constructor
     */
    Node(): id(0), weight(0), parent(0), word_id(0){}
    
    /**
     * Constructor
     * @param _id node id
     */
    Node(NodeId _id): id(_id), weight(0), parent(0), word_id(0){}

    /**
     * Returns whether the node is a leaf node
     * @return true iff the node is a leaf
     */
    inline bool isLeaf() const { return children.empty(); } // g
  };

protected:

  /**
   * Creates an instance of the scoring object accoring to m_scoring
   */
  void createScoringObject();
 
  /**
   * Returns the word id associated to a feature
   * @param feature
   * @param id (out) word id
   * @param weight (out) word weight
   * @param nid (out) if given, id of the node "levelsup" levels up
   * @param levelsup
   */
  virtual void transform(const TDescriptor &feature, 
    WordId &id, WordValue &weight, NodeId* nid = NULL, int levelsup = 0) const;
  
protected:

  /// Branching factor
  int m_k;
  
  /// Depth levels 
  int m_L;
  
  /// Weighting method
  WeightingType m_weighting;
  
  /// Scoring method
  ScoringType m_scoring;
  
  /// Object for computing scores
  GeneralScoring* m_scoring_object;
  
  /// Tree nodes
  std::vector<Node> m_nodes;
  
  /// Words of the vocabulary (tree leaves)
  /// this condition holds: m_words[wid]->word_id == wid
  std::vector<Node*> m_words;
  
};

// --------------------------------------------------------------------------

template<class TDescriptor, class F>
TemplatedVocabulary<TDescriptor,F>::TemplatedVocabulary // g
  (int k, int L, WeightingType weighting, ScoringType scoring)
  : m_k(k), m_L(L), m_weighting(weighting), m_scoring(scoring),
  m_scoring_object(NULL)
{
  createScoringObject();
}

template<class TDescriptor, class F>
void TemplatedVocabulary<TDescriptor,F>::createScoringObject()
{
    delete m_scoring_object;

  m_scoring_object = NULL;
  
  switch(m_scoring)
  {
    case L1_NORM:        
      m_scoring_object = new L1Scoring;      
      break;
      
    case L2_NORM:
      m_scoring_object = new L2Scoring;
      break;
    
    case CHI_SQUARE:
      m_scoring_object = new ChiSquareScoring;
      break;
      
    case KL:
      m_scoring_object = new KLScoring;
      break;
      
    case BHATTACHARYYA:
      m_scoring_object = new BhattacharyyaScoring;
      break;
      
    case DOT_PRODUCT:
      m_scoring_object = new DotProductScoring;
      break;    
  }
}

template<class TDescriptor, class F>
TemplatedVocabulary<TDescriptor,F>::~TemplatedVocabulary()
{
  delete m_scoring_object;
}

template<class TDescriptor, class F>
inline unsigned int TemplatedVocabulary<TDescriptor,F>::size() const
{
    return static_cast<unsigned int>(m_words.size());
}

template<class TDescriptor, class F>
inline bool TemplatedVocabulary<TDescriptor,F>::empty() const // g
{
  return m_words.empty();
}

template<class TDescriptor, class F> 
int TemplatedVocabulary<TDescriptor,F>::transform(
  const std::vector<TDescriptor>& features,
  BowVector &v, FeatureVector &fv, int levelsup) const // g
{
  v.clear();
  fv.clear();
  
  if(empty()) // safe for subclasses
  {
    return 0;
  }
  
  int ioutliers = 0;
  // normalize 
  LNorm norm;
  bool must = m_scoring_object->mustNormalize(norm);
  
  typename std::vector<TDescriptor>::const_iterator fit;
  
  if(m_weighting == TF || m_weighting == TF_IDF)
  {
    unsigned int i_feature = 0;
    for(fit = features.begin(); fit < features.end(); ++fit, ++i_feature)
    {
      WordId id = 0;
      NodeId nid = 0;
      WordValue w = 0; 
      // w is the idf value if TF_IDF, 1 if TF
      
      transform(*fit, id, w, &nid, levelsup);
      
      if(w > 0) // not stopped
      { 
        v.addWeight(id, w);
        fv.addFeature(nid, i_feature);
      }
      else
      {
          ioutliers++;
      }
    }
    
    if(!v.empty() && !must)
    {
      // unnecessary when normalizing
        const double nd = static_cast<double>(v.size());
      for(BowVector::iterator vit = v.begin(); vit != v.end(); vit++) 
        vit->second /= nd;
    }
  
  }
  else // IDF || BINARY
  {
    unsigned int i_feature = 0;
    for(fit = features.begin(); fit < features.end(); ++fit, ++i_feature)
    {
      WordId id;
      NodeId nid;
      WordValue w;
      // w is idf if IDF, or 1 if BINARY
      
      transform(*fit, id, w, &nid, levelsup);
      
      if(w > 0) // not stopped
      {
        v.addIfNotExist(id, w);
        fv.addFeature(nid, i_feature);
      }
    }
  } // if m_weighting == ...
  
  if(must) v.normalize(norm);
  return ioutliers;
}

template<class TDescriptor, class F> 
inline double TemplatedVocabulary<TDescriptor,F>::score
  (const BowVector &v1, const BowVector &v2) const
{
  return m_scoring_object->score(v1, v2);
}

// --------------------------------------------------------------------------

template<class TDescriptor, class F>
void TemplatedVocabulary<TDescriptor,F>::transform(const TDescriptor &feature, 
  WordId &word_id, WordValue &weight, NodeId *nid, int levelsup) const // g
{ 
  // propagate the feature down the tree
  std::vector<NodeId> nodes;
  typename std::vector<NodeId>::const_iterator nit;

  // level at which the node must be stored in nid, if given
  const int nid_level = m_L - levelsup;
  if(nid_level <= 0 && nid != NULL) *nid = 0; // root

  NodeId final_id = 0; // root
  int current_level = 0;

  do
  {
    ++current_level;
    nodes = m_nodes[final_id].children;
    final_id = nodes[0];
 
    double best_d = F::distance(feature, m_nodes[final_id].descriptor);

    for(nit = nodes.begin() + 1; nit != nodes.end(); ++nit)
    {
      NodeId id = *nit;
      double d = F::distance(feature, m_nodes[id].descriptor);
      if(d < best_d)
      {
        best_d = d;
        final_id = id;
      }
    }
    
    if(nid != NULL && current_level == nid_level)
      *nid = final_id;
    
  } while( !m_nodes[final_id].isLeaf() );

  // turn node id into word id
  word_id = m_nodes[final_id].word_id;
  weight = m_nodes[final_id].weight;
}

template<class TDescriptor, class F>
bool TemplatedVocabulary<TDescriptor, F>::loadFromMemory(const char *pBinaries, size_t size)
{
   bool bRet = false;

    m_k = 10;
    m_L = 6;
    int n1 = 0;
    int n2 = 0;

    m_scoring = (ScoringType)n1;
    m_weighting = (WeightingType)n2;
    
    createScoringObject();
    
    std::vector<Node>& nodes = m_nodes;
    std::vector<Node*>& words = m_words;

    // reading
    {
        unsigned int nNodes = 0;
        const char* pSrc = pBinaries;
        
        memcpy(&nNodes, pSrc, sizeof(unsigned int)*1);
        pSrc += sizeof(unsigned int);        
        //fread(&nNodes, sizeof(unsigned int), 1, pf);
        
        nodes.resize(nNodes);
        for (unsigned int i = 0; i < nNodes; ++i)
        {
            memcpy(&nodes[i].id, pSrc, sizeof(NodeId) * 1);
            pSrc += sizeof(NodeId);
            //fread(&nodes[i].id, sizeof(NodeId), 1, pf);
            memcpy(&nodes[i].weight, pSrc, sizeof(WordValue) * 1);
            pSrc += sizeof(WordValue);
            //fread(&nodes[i].weight, sizeof(WordValue), 1, pf);
            memcpy(&nodes[i].parent, pSrc, sizeof(NodeId) * 1);
            pSrc += sizeof(NodeId);
            //fread(&nodes[i].parent, sizeof(NodeId), 1, pf);
            unsigned int nChildren = 0;

            memcpy(&nChildren, pSrc, sizeof(unsigned int) * 1);
            pSrc += sizeof(unsigned int);
            //fread(&nChildren, sizeof(unsigned int), 1, pf);
            if (nChildren > 0)
            {
                nodes[i].children.resize(nChildren);

                memcpy(&nodes[i].children[0], pSrc, sizeof(NodeId) * nChildren);
                pSrc += sizeof(NodeId) * nChildren;
                //fread(&nodes[i].children[0], sizeof(NodeId), nChildren, pf);
            }
            else
            {
                int wid = static_cast<int>(words.size());
                words.resize(wid + 1);
                nodes[i].word_id = wid;
                words[wid] = &nodes[i];
            }

            memcpy(&nodes[i].word_id, pSrc, sizeof(WordId) * 1);
            pSrc += sizeof(WordId);
            //fread(&nodes[i].word_id, sizeof(WordId), 1, pf);

            memcpy(nodes[i].descriptor.data(), pSrc, sizeof(char)*32);
            pSrc += sizeof(char) * 32;
            //fread(nodes[i].descriptor.data, sizeof(char), 32, pf);
        }
        bRet = true;
    }
      
    return bRet;
}

template<class TDescriptor, class F>
bool TemplatedVocabulary<TDescriptor, F>::loadFromTextFile(const std::string &filename)
{
    bool bRet = false;
    
    m_k = 10;
    m_L = 6;
    int n1 = 0;
    int n2 = 0;

    m_scoring = (ScoringType)n1;
    m_weighting = (WeightingType)n2;
    createScoringObject();

    std::vector<Node>& nodes = m_nodes;
    std::vector<Node*>& words = m_words;
    // reading
    {
        FILE* pf = fopen(filename.c_str(), "rb");
        if (nullptr != pf)
        {
            unsigned int nNodes = 0;
            fread(&nNodes, sizeof(unsigned int), 1, pf);
            if (nNodes > 0) nodes.resize(nNodes);
            else nNodes = 0;
            
            for (unsigned int i = 0; i < nNodes; ++i)
            {
                fread(&nodes[i].id, sizeof(NodeId), 1, pf);
                fread(&nodes[i].weight, sizeof(WordValue), 1, pf);
                fread(&nodes[i].parent, sizeof(NodeId), 1, pf);
                unsigned int nChildren = 0;
                fread(&nChildren, sizeof(unsigned int), 1, pf);
                if (nChildren > 0)
                {
                    nodes[i].children.resize(nChildren);
                    fread(&nodes[i].children[0], sizeof(NodeId), nChildren, pf);
                }
                else
                {
                    int wid = static_cast<int>(words.size());
                    words.resize(wid + 1);
                    nodes[i].word_id = wid;
                    words[wid] = &nodes[i];
                }
                fread(&nodes[i].word_id, sizeof(WordId), 1, pf);
                fread(nodes[i].descriptor.data(), sizeof(char), 32, pf);
            }
            fclose(pf);
            bRet = true;
        }        
    }
     
    return bRet;
}

}

#endif
