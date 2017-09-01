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
    /// Children 
    NodeId nChildren;

    ///Inner nodes hold the first_child_id, and leaves hold the word_weight.
    union
    {
        ///Children IDs are consecutive, so keeping only the first.
        NodeId first_child_id;

        /// Weight if the node is a word
        WordValue weight;
    };
    /// Node descriptor
    TDescriptor descriptor;

    /**
     * Empty constructor
     */
    Node(): nChildren(0), weight(0){}

    /**
     * Returns whether the node is a leaf node
     * @return true iff the node is a leaf
     */
    inline bool isLeaf() const { return nChildren == 0; } // g
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
  Node* m_nodes;

  /// Words of the vocabulary (tree leaves)
  /// this condition holds: m_words[wid]->word_id == wid
  std::vector<Node*> m_words;

  /// For each leaf, point to the corresponding word
  std::vector<WordId> m_node_to_word;
  
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
  // level at which the node must be stored in nid, if given
  const int nid_level = m_L - levelsup;
  if(nid_level <= 0 && nid != NULL) *nid = 0; // root

  NodeId final_id = 0; // root
  int current_level = 0;

  do
  {
    ++current_level;
    int nChildren = m_nodes[final_id].nChildren;
    NodeId first_child_id = m_nodes[final_id].first_child_id;
    final_id = first_child_id;
 
    double best_d = F::distance(feature, m_nodes[final_id].descriptor);

    for(NodeId current_child = 1; current_child < nChildren; current_child++)
    {
      NodeId id = first_child_id + current_child;

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
  word_id = m_node_to_word[final_id];
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

    // reading
    {
        NodeId nNodes = 0;
        const char* pSrc = pBinaries;

        memcpy(&nNodes, pSrc, sizeof(NodeId)*1);
        pSrc += sizeof(NodeId);

        m_node_to_word.resize(nNodes);
        m_words.reserve(nNodes); //There are less words than nodes, but not much less (about 90%) and it worth to have less allocation
        m_nodes = (Node*)pSrc;

        WordId word_ids = 0;
        for(unsigned int i = 0; i < nNodes; i++)
        {
            if(m_nodes[i].isLeaf())
            {
                m_node_to_word[i] = word_ids++;
                m_words.push_back(&m_nodes[i]);
            }
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
    unsigned int nNodes = 0;
    Node* nodes = nullptr;//m_nodes;
    std::vector<Node*>& words = m_words;
    // reading
    {
        FILE* pf = fopen(filename.c_str(), "rb");
        if (nullptr != pf)
        {

            fread(&nNodes, sizeof(unsigned int), 1, pf);
            if (nNodes > 0)
            {
                nodes = new Node[nNodes];
                m_node_to_word.resize(nNodes);
            }
            else nNodes = 0;

            WordId word_ids = 0;
            for (unsigned int i = 0; i < nNodes; ++i)
            {
                fread(&nodes[i].nChildren, sizeof(int), 1, pf);
                if (nodes[i].nChildren > 0)
                {
                    fread(&nodes[i].first_child_id, sizeof(NodeId), 1, pf);
                }
                else
                {
                    fread(&nodes[i].weight, sizeof(WordValue), 1, pf);

                    m_node_to_word[i] = word_ids++;
                    m_words.push_back(&nodes[i]);
                }

                fread(nodes[i].descriptor.data(), sizeof(char), 32, pf);
            }
            fclose(pf);
            bRet = true;
            m_nodes = nodes;
        }
    }
    return bRet;
}

}

#endif
