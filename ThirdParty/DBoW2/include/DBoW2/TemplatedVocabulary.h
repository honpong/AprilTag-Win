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

#include <numeric>
#include <fstream>
#include <random>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "endianness.h"

namespace DBoW2 {

typedef uint32_t WordId;
typedef float WordValue;
typedef std::unordered_map<WordId, WordValue> BowVector;
typedef std::unordered_map<WordId, std::vector<size_t>> FeatureVector;

enum ScoringType {
  L1_NORM,
  L2_NORM,
  CHI_SQUARE,
  KL,
  BHATTACHARYYA,
  DOT_PRODUCT,
  LAST_VALUE  // control value
};

/**
 * Templated vocabulary
 * @param T Descriptor class that implements:
 *   static float distance(const T&, const T&): computes distance.
 *   T mean(const std::vector<const T*>&): computes mean of descriptors.
 * @param type ScoringType value
 */
template<typename T, int type>
class TemplatedVocabulary {
    static_assert(type >= 0 && type < ScoringType::LAST_VALUE,
                  "Wrong type in TemplatedVocabulary<T, type>");

 public:
    using descriptor_type = T;
    static constexpr ScoringType scoring_type = static_cast<ScoringType>(type);

    TemplatedVocabulary();
    TemplatedVocabulary(const TemplatedVocabulary<T, type>&) = delete;
    TemplatedVocabulary<T, type>& operator=(const TemplatedVocabulary<T, type>&) = delete;

    /** True iff vocabulary has not been created.
     */
    bool empty() const;

    /** Returns number of words in the tree (not nodes).
     */
    size_t size() const;

    /** Resets the vocabulary and clears the tree.
     */
    void clear();

    /** Returns the scoring type.
     */
    constexpr ScoringType getScoringType() const;

    /** Returns branching factor (k).
     */
    int getBranchingFactor() const;

    /** Returns current depth levels of the tree (L).
     */
    int getDepthLevels() const;

    /** Resets the vocabulary and creates it from a collection of descriptors.
     * The BowVectors that were already created cannot be matched against
     * the ones obtained after training.
     * @param descriptors training descriptors given in covisibility groups.
     * @param k branching factor.
     * @param L depth levels.
     */
    void train(const std::vector<std::vector<T>>& descriptors, int k, int L);

    /** Resets the vocabulary and creates it from a collection of descriptors.
     * The BowVectors that were already created cannot be matched against
     * the ones obtained after training.
     * @param begin_group iterator to the first group of descriptors.
     * @param end_group iterator to the last+1 group of descriptors.
     * @param group_op function such that group_op(*begin_group) returns
     * a std container of structs that contain descriptors. The container must
     * be a valid parameter for std::begin and std::end.
     * @param descriptor_op function such that descriptor_op(*it) returns
     * a reference to a descriptor, where "it" is an iterator that traverses
     * the container returned by group_op.
     * @param k branching factor.
     * @param L depth levels.
     */
    template<typename It, typename UnaryGroupOp, typename UnaryDescriptorOp>
    void train(It begin_group, It end_group,
               UnaryGroupOp group_op, UnaryDescriptorOp descriptor_op,
               int k, int L);

    /** Returns the normalized tf-idf histogram of some descriptors.
     */
    BowVector transform(const std::vector<T>& descriptors) const;

    /** Returns the normalized tf-idf histogram of some descriptors.
     * @param begin iterator to first struct containing the descriptor.
     * @param end iterator to last+1 struct containing the descriptor.
     * @param unary_op function such that unary_op(*begin) returns the descriptor.
     */
    template<typename It, typename UnaryOp>
    BowVector transform(It begin, It end, UnaryOp unary_op) const;

    /** Returns the normalized tf-idf histogram of some descriptors
     * and their indices arranged by words.
     * @param[out] fv indices of descriptors arranged by words.
     * @param levelsup number of levels to go up the tree from the leaves
     * to select the indexing nodes (higher = coarser groups).
     */
    BowVector transform(const std::vector<T>& descriptors,
                        FeatureVector& fv, int levelsup = 0) const;

    /** Returns the normalized tf-idf histogram of some descriptors
     * and their indices arranged by words.
     * @param begin iterator to first struct containing the descriptor.
     * @param end iterator to last+1 struct containing the descriptor.
     * @param unary_op function such that unary_op(*begin) returns the descriptor.
     * @param[out] fv indices of descriptors arranged by words.
     * @param levelsup number of levels to go up the tree from the leaves
     * to select the indexing nodes (higher = coarser groups).
     */
    template<typename It, typename UnaryOp>
    BowVector transform(It begin, It end, UnaryOp unary_op,
                        FeatureVector& fv, int levelsup = 0) const;

    /** Matches two sets of descriptors and returns a matching score.
     */
    float score(const typename std::vector<T>& lhs,
                const typename std::vector<T>& rhs) const;

    /** Matches two bags of words vectors and returns a matching score.
     */
    float score(const BowVector& lhs, const BowVector& rhs) const;

    /** Saves vocabulary to a binary file.
     */
    bool saveToBinaryFile(const std::string& filename) const;

    /** Loads the vocabulary from a binary file.
     */
    bool loadFromBinaryFile(const std::string& filename);

    /** Loads the vocabulary from a binary file that is already loaded in
     * memory.
     */
    bool loadFromMemory(const char *pBinaries, size_t size);

 private:
    enum Version {  // version of the binary file (for backwards compatibility).
        VERSION_1 = 1
    };

    struct Node {
        T descriptor;
        union {
            uint32_t children;  // number of valid children if inner node
            float weight;  // idf weight for words/leaves
        };
    };

    struct Cluster {
        std::vector<const T*> members;
        T centroid;
        Cluster() {}
        Cluster(const T& centroid_) : centroid(centroid_) {}
        Cluster(std::vector<const T*>&& members_, const T& centroid_) :
            members(members_), centroid(centroid_) {}
    };

    std::vector<Node> m_nodes_pool;
    const Node* m_nodes;  // points to m_nodes_pool or to external data

    int m_k;  // branching factor
    int m_L;  // max levels (L-1 levels of inner nodes + 1 last level of words)

 private:
    /** Creates the tree recursively with kmeans.
     */
    int trainHKMeansStep(WordId first_node_id,
                         const typename std::vector<const T*>& corpus,
                         int level);

    /** Initiates clusters with kmeans++.
     */
    std::vector<Cluster> trainInitialKMppClusters(
            const typename std::vector<const T*>& corpus);

    /** Sets the weights of the words after training.
     */
    template<typename It, typename UnaryGroupOp, typename UnaryDescriptorOp>
    void setWordWeights(It begin_group, It end_group, UnaryGroupOp group_op,
                        UnaryDescriptorOp descriptor_op);

    /** Transforms descriptors into words.
     */
    template<typename It, typename UnaryOp>
    BowVector transform(It begin, It end, UnaryOp unary_op,
                        FeatureVector* fv, int levelsup) const;

    /** Traverses the tree to find the word and feature_node of a descriptor.
     */
    WordId traverseTree(const T& descriptor,
                         WordId* fv_node = nullptr, int levelsup = 0) const;

    /** Returns the index of the parent of a node.
     * Undefined if node has no parent.
     */
    WordId parent(WordId child) const;

    /** Returns the index of the first child of a node.
     */
    WordId firstChild(WordId parent) const;

    /** Returns the number of inner nodes for a specific tree size.
     */
    static size_t calculateInnerNodes(int k, int L);

    /** Returns the number of leaves for a specific tree size.
     */
    static size_t calculateWords(int k, int L);

    /** Returns the number of inner nodes + leaves for a specific tree size.
     */
    static size_t calculateNodes(int k, int L);

    /** Returns pow(base, positive_exp) for integers.
     * @param positive_exp must be > 0
     */
    static size_t powInt(size_t base, size_t positive_exp);

    class SerializerVersion1 {
     public:
        using Voc = TemplatedVocabulary<T, type>;
        static bool save(const Voc& voc, std::ofstream& file);
        static bool load(Voc& voc, std::ifstream& file);
        static bool load(Voc& voc, const char* buffer, size_t buffer_size);
     private:
        static bool parseMeta(const uint32_t meta[3], int& k, int& L);
    };

    class SerializerLegacyVersion {
     public:
        using Voc = TemplatedVocabulary<T, type>;
        static bool load(Voc& voc, uint32_t number_of_nodes, std::ifstream& file);
    };
};

// --------------------------------------------------------------------------

template<typename T, int type>
TemplatedVocabulary<T, type>::TemplatedVocabulary() :
    m_nodes(nullptr), m_k(0), m_L(0) {}

template<typename T, int type>
constexpr ScoringType TemplatedVocabulary<T, type>::getScoringType() const {
    return static_cast<ScoringType>(type);
}

template<typename T, int type>
int TemplatedVocabulary<T, type>::getBranchingFactor() const {
    return m_k;
}

template<typename T, int type>
int TemplatedVocabulary<T, type>::getDepthLevels() const {
    return m_L;
}

template<typename T, int type>
void TemplatedVocabulary<T, type>::clear() {
    m_k = 0;
    m_L = 0;
    m_nodes_pool.clear();
    m_nodes = nullptr;
}

template<typename T, int type>
bool TemplatedVocabulary<T, type>::empty() const {
    return (m_nodes == nullptr);
}

template<typename T, int type>
size_t TemplatedVocabulary<T, type>::size() const {
    return (m_k > 0 && m_L > 0 ? powInt(m_k, m_L) : 0);
}

template<typename T, int type>
WordId TemplatedVocabulary<T, type>::parent(WordId child) const {
    return child / m_k + 1;
}

template<typename T, int type>
WordId TemplatedVocabulary<T, type>::firstChild(WordId parent) const {
    return (parent + 1) * m_k;
}

template<typename T> static inline constexpr const T& identity(const T& t) { return t; }

template<typename T, int type>
BowVector TemplatedVocabulary<T, type>::transform(
        const std::vector<T>& descriptors) const {
    return transform(descriptors.begin(), descriptors.end(), identity<T>,
                     nullptr, 0);
}

template<typename T, int type>
template<typename It, typename UnaryOp>
BowVector TemplatedVocabulary<T, type>::transform(It begin, It end,
                                                  UnaryOp unary_op) const {
    return transform(begin, end, unary_op, nullptr, 0);
}

template<typename T, int type>
BowVector TemplatedVocabulary<T, type>::transform(
        const std::vector<T>& descriptors, FeatureVector& fv,
        int levelsup) const {
    return transform(descriptors.begin(), descriptors.end(), identity<T>,
                     &fv, levelsup);
}

template<typename T, int type>
template<typename It, typename UnaryOp>
BowVector TemplatedVocabulary<T, type>::transform(
        It begin, It end, UnaryOp unary_op, FeatureVector& fv,
        int levelsup) const {
    return transform(begin, end, unary_op, &fv, levelsup);
}

template<typename T, int type>
template<typename It, typename UnaryOp>
BowVector TemplatedVocabulary<T, type>::transform(
        It begin, It end, UnaryOp unary_op, FeatureVector* fv,
        int levelsup) const {
    if (fv) fv->clear();
    if (!m_nodes) return BowVector();

    BowVector bow;
    WordId fv_node;
    for (size_t descriptor_idx = 0; begin != end; ++begin, ++descriptor_idx) {
        const auto& descriptor = unary_op(*begin);
        WordId word = traverseTree(descriptor, &fv_node, levelsup);
        if (m_nodes[word].weight > 0) bow[word] += m_nodes[word].weight;
        if (fv) (*fv)[fv_node].push_back(descriptor_idx);
    }

    float norm = 0.f;
    if (type == L2_NORM) {
        for (auto& pair : bow) norm += pair.second * pair.second;
    } else {
        for (auto& pair : bow) norm += std::fabs(pair.second);
    }
    if (norm > 0.f) {
        for (auto& pair : bow) pair.second /= norm;
    }
    return bow;
}

template<typename T, int type>
WordId TemplatedVocabulary<T, type>::traverseTree(
        const T& descriptor, WordId* fv_node, int levelsup) const {
    auto choose_best_node = [this](const T& descriptor,
            WordId begin, WordId end) {
        WordId best_node = 0;
        float best_distance = std::numeric_limits<float>::infinity();
        for (; begin != end; ++begin) {
            float distance = T::distance(descriptor, m_nodes[begin].descriptor);
            if (distance < best_distance) {
                best_distance = distance;
                best_node = begin;
            }
        }
        //assert(!std::isinf(best_distance));  // this can't happen
        return best_node;
    };

    WordId node = choose_best_node(descriptor, 0, m_k);
    if (fv_node) *fv_node = (levelsup < m_L ? node : 0);
    for (int level = 1; level < m_L && m_nodes[node].children > 0; ++level) {
        WordId begin = firstChild(node);
        WordId end = begin + m_nodes[node].children;
        node = choose_best_node(descriptor, begin, end);
        if (fv_node && level < m_L - levelsup)
            *fv_node = node;
    }
    return node;
}

template<typename T, int type>
void TemplatedVocabulary<T, type>::train(
        const std::vector<std::vector<T>>& descriptors, int k, int L) {
    train(descriptors.begin(), descriptors.end(),
          identity<std::vector<T>>, identity<T>, k, L);
}

template<typename T, int type>
template<typename It, typename UnaryGroupOp, typename UnaryDescriptorOp>
void TemplatedVocabulary<T, type>::train(It begin_group, It end_group,
                                         UnaryGroupOp group_op,
                                         UnaryDescriptorOp descriptor_op,
                                         int k, int L) {
    if (k <= 1 || L <= 0 || begin_group == end_group) {
        clear();
        return;
    }

    m_k = k;
    m_L = L;
    m_nodes_pool.resize(calculateNodes(m_k, m_L));
    m_nodes = &m_nodes_pool[0];

    std::vector<const T*> corpus;
    for (auto group_it = begin_group; group_it != end_group; ++group_it) {
        const auto& features = group_op(*group_it);
        for (auto it = std::begin(features); it != std::end(features); ++it) {
            const auto* item = &descriptor_op(*it);
            corpus.emplace_back(item);
        }
    }

    int nodes_first_level = trainHKMeansStep(0, corpus, 0);
    if (nodes_first_level < k) {
        // special case for very few training descriptors
        for (int i = nodes_first_level; i < k; ++i) {
            m_nodes_pool[i].descriptor = m_nodes[0].descriptor;
            m_nodes_pool[i].children = 0;
        }
    }

    setWordWeights(begin_group, end_group, group_op, descriptor_op);
}

template<typename T, int type>
int TemplatedVocabulary<T, type>::trainHKMeansStep(
        WordId first_node_id, const typename std::vector<const T*>& corpus,
        int level) {
    std::vector<Cluster> clusters;

    if (corpus.size() <= static_cast<size_t>(m_k)) {
        // trivial case: one cluster per feature
        clusters.reserve(m_k);
        for (const auto* descriptor : corpus)
            clusters.emplace_back(std::vector<const T*>{descriptor},
                                  *descriptor);
    } else {
        // kmeans
        auto same_members = [](const std::vector<const T*>& lhs,
                const std::vector<const T*>& rhs) {
            if (lhs.size() != rhs.size()) return false;
            for (size_t i = 0; i < lhs.size(); ++i) {
                if (lhs[i] != rhs[i]) return false;
            }
            return true;
        };

        auto same_clusters = [&same_members](
                const std::vector<Cluster>& lhs,
                const std::vector<Cluster>& rhs) {
            if (lhs.size() != rhs.size()) return false;
            for (size_t i = 0; i < lhs.size(); ++i) {
                if (!same_members(lhs[i].members, rhs[i].members))
                    return false;
            }
            return true;
        };

        std::vector<Cluster> previous_clusters;
        clusters = trainInitialKMppClusters(corpus);

        for (bool convergence = false; !convergence; ) {
            for (auto& cluster : clusters) cluster.members.clear();
            for (auto* item : corpus) {
                float best_distance = std::numeric_limits<float>::infinity();
                Cluster* best_cluster = nullptr;
                for (auto& cluster : clusters) {
                    float distance = T::distance(*item, cluster.centroid);
                    if (distance < best_distance) {
                        best_distance = distance;
                        best_cluster = &cluster;
                    }
                }
                //assert(best_cluster);
                best_cluster->members.push_back(item);
            }
            for (auto& cluster : clusters) {
                cluster.centroid = T::mean(cluster.members);
            }
            convergence = same_clusters(clusters, previous_clusters);
            if (!convergence) previous_clusters = clusters;
        }
    }

    // create nodes
    if (level < m_L - 1) {
        auto node_it = m_nodes_pool.begin() + first_node_id;
        WordId current_node_id = first_node_id;
        for (auto& cluster : clusters) {
            node_it->descriptor = cluster.centroid;
            node_it->children = trainHKMeansStep(firstChild(current_node_id),
                                                   cluster.members,
                                                   level + 1);
            ++node_it;
            ++current_node_id;
        }
    } else {
        auto node_it = m_nodes_pool.begin() + first_node_id;
        for (auto& cluster : clusters) {
            node_it->descriptor = cluster.centroid;
            node_it->weight = 0;
            ++node_it;
        }
    }
    return clusters.size();
}

template<typename T, int type>
std::vector<typename TemplatedVocabulary<T, type>::Cluster>
TemplatedVocabulary<T, type>::trainInitialKMppClusters(
        const std::vector<const T*>& corpus) {
    // Implements kmeans++ seeding algorithm:
    // 1. Choose one center uniformly at random from among the data points.
    // 2. For each data point x, compute D(x), the distance between x and the
    //    nearest center that has already been chosen.
    // 3. Add one new data point as a center. Each point x is chosen with
    // probability proportional to D(x)^2.
    // 4. Repeat Steps 2 and 3 until k centers have been chosen.
    std::mt19937 gen;
    std::vector<Cluster> clusters;

    auto remove_item = [](std::vector<const T*>& items, size_t idx) {
        std::swap(items[idx], items.back());
        items.pop_back();
    };

    auto rnd_uniform = [&gen](const std::vector<const T*>& items) {
        std::uniform_int_distribution<size_t> rnd(0, items.size() - 1);
        return rnd(gen);
    };

    auto available_corpus = corpus;
    size_t index = rnd_uniform(available_corpus);
    clusters.emplace_back(*available_corpus[index]);
    remove_item(available_corpus, index);

    std::vector<float> sq_distances;
    sq_distances.reserve(available_corpus.size());

    auto rnd_proportional_to_sq_d = [&gen](const std::vector<float>& sq_d) {
        float total = std::accumulate(sq_d.begin(), sq_d.end(), 0.f);
        std::uniform_real_distribution<float> rnd(0, total);  // [0, total)

        size_t index = 0;
        float target = rnd(gen);
        for (float traveled = 0;
             traveled <= target && index < sq_d.size(); ++index) {
            traveled += sq_d[index];
        }
        return index - 1;
    };

    for (int t = 1; t < m_k; ++t) {
        sq_distances.clear();
        for (auto* item : available_corpus) {
            float min_distance = std::numeric_limits<float>::infinity();
            for (auto& cluster : clusters) {
                float distance = T::distance(*item, cluster.centroid);
                if (distance < min_distance) {
                    min_distance = distance;
                }
            }
            sq_distances.push_back(min_distance * min_distance);
        }

        index = rnd_proportional_to_sq_d(sq_distances);
        clusters.emplace_back(*available_corpus[index]);
        remove_item(available_corpus, index);
    }
    return clusters;
}

template<typename T, int type>
template<typename It, typename UnaryGroupOp, typename UnaryDescriptorOp>
void TemplatedVocabulary<T, type>::setWordWeights(
        It begin_group, It end_group,
        UnaryGroupOp group_op, UnaryDescriptorOp descriptor_op) {
    std::unordered_map<WordId, int> document_counter;
    for (auto group_it = begin_group; group_it != end_group; ++group_it) {
        const auto& group = group_op(*group_it);
        std::unordered_set<WordId> words;
        for (auto it = std::begin(group); it != std::end(group); ++it) {
            words.emplace(traverseTree(descriptor_op(*it)));
        }
        for (WordId word : words) ++document_counter[word];
    }
    const int number_of_documents = std::distance(begin_group, end_group);
    const float log_documents = std::log(number_of_documents);
    for (auto& pair : document_counter) {
        // idf = log(|D| / |D containing word|)
        if (pair.second < number_of_documents) {
            m_nodes_pool[pair.first].weight =
                    log_documents - std::log(pair.second);
        }
    }
}

template<typename T, int type>
float TemplatedVocabulary<T, type>::score(const std::vector<T>& lhs,
                                          const std::vector<T>& rhs) const {
    return score(transform(lhs), transform(rhs));
}

template<typename T, int type>
float TemplatedVocabulary<T, type>::score(const BowVector& lhs_,
                                          const BowVector& rhs_) const {
    const bool swap = (type != ScoringType::KL && lhs_.size() > rhs_.size());
    const BowVector& lhs = (swap ? rhs_ : lhs_);
    const BowVector& rhs = (swap ? lhs_ : rhs_);

    float score = 0.f;
    for (auto& lhs_pair : lhs) {
        auto rhs_it = rhs.find(lhs_pair.first);
        if (rhs_it != rhs.end()) {
            auto vi = lhs_pair.second;
            auto wi = rhs_it->second;

            switch (type) {
            case ScoringType::L1_NORM:
                score += std::fabs(vi - wi) - std::fabs(vi) - std::fabs(wi);
                break;
            case ScoringType::L2_NORM:
            case ScoringType::DOT_PRODUCT:
                score += vi * wi;
                break;
            case ScoringType::BHATTACHARYYA:
                score += std::sqrt(vi * wi);
                break;
            case ScoringType::CHI_SQUARE:
            {
                float add = vi + wi;
                if (add > 0) score += (vi - wi) * (vi - wi) / add;
                break;
            }
            case ScoringType::KL:
            {
                constexpr float LOG_EPSILON = -36.043653389;
                score += vi * (LOG_EPSILON - std::log(wi));
                break;
            }
            default:
                break;
            }
        }
    }
    if (type == ScoringType::L1_NORM) {
        score = -score / 2.f;
    } else if (type == ScoringType::L2_NORM) {
        score = (score >= 1.f ? 1.f : 1.f - std::sqrt(1.f - score));
    }
    return score;
}

template<typename T, int type>
bool TemplatedVocabulary<T, type>::saveToBinaryFile(
        const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;

    // byte 0: version number
    // byes 1-3: all bits set.
    char header[4] = {static_cast<char>(VERSION_1), -1, -1, -1};
    file.write(header, 4);
    return SerializerVersion1::save(*this, file);
}

template<typename T, int type>
bool TemplatedVocabulary<T, type>::loadFromBinaryFile(
        const std::string& filename) {
    clear();
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;

    constexpr int header_bytes = 4;
    char buffer[header_bytes];
    file.read(buffer, header_bytes);

    if (buffer[3] & 0x80) {
        // the first byte encodes the version of the vocabulary format.
        int version = buffer[0];
        if (version == VERSION_1) {
            return SerializerVersion1::load(*this, file);
        } else {
            return false;
        }
    } else {
        // this is a legacy vocabulary file. The legacy format started with
        // 4 little endian bytes to encode the number of nodes in the tree,
        // such that the most significant bit was 0 always (since the largest
        // tree we used was around 1M nodes only).
        uint32_t number_of_nodes = le32toh(
                    *reinterpret_cast<const uint32_t*>(buffer));
        return SerializerLegacyVersion::load(*this, number_of_nodes, file);
    }
}

template<typename T, int type>
bool TemplatedVocabulary<T, type>::loadFromMemory(const char *pBinaries,
                                                  size_t size) {
    // only the latest version format (VERSION_1) is supported by this function
    clear();
    constexpr size_t header_bytes = 4;
    if (size < header_bytes) return false;
    const char* header = pBinaries;
    if (header[3] & 0x80) {
        int version = header[0];
        if (version != VERSION_1) return false;
    } else {
        return false;
    }
    return SerializerVersion1::load(*this, pBinaries + header_bytes,
                                    size - header_bytes);
}

template<typename T, int type>
bool TemplatedVocabulary<T, type>::SerializerVersion1::save(
        const TemplatedVocabulary<T, type>& voc, std::ofstream& file) {
    // little endian
    // bytes at offset+[0-3]: sizeof(Node)  // for sanity check
    // bytes at offset+[4-7]: k
    // bytes at offset+[8-11]: L
    // remaining: nodes in order (descriptor + children/weight field)
    uint32_t sizeof32 = htole32(static_cast<uint32_t>(sizeof(Node)));
    uint32_t k32 = htole32(static_cast<uint32_t>(voc.m_k));
    uint32_t L32 = htole32(static_cast<uint32_t>(voc.m_L));
    file.write(reinterpret_cast<char*>(&sizeof32), sizeof(uint32_t));
    file.write(reinterpret_cast<char*>(&k32), sizeof(uint32_t));
    file.write(reinterpret_cast<char*>(&L32), sizeof(uint32_t));

    if (voc.m_nodes != nullptr) {
        file.write(reinterpret_cast<const char*>(voc.m_nodes),
                   sizeof(Node) * calculateNodes(voc.m_k, voc.m_L));
    }

    file.close();  // make sure badbit is set in the case of problems
    return static_cast<bool>(file);
}

template<typename T, int type>
bool TemplatedVocabulary<T, type>::SerializerVersion1::load(
        TemplatedVocabulary<T, type>& voc, std::ifstream& file) {
    uint32_t meta[3];
    file.read(reinterpret_cast<char*>(meta), sizeof(uint32_t) * 3);
    if (!parseMeta(meta, voc.m_k, voc.m_L)) return false;
    voc.m_nodes_pool.resize(calculateNodes(voc.m_k, voc.m_L));
    voc.m_nodes = &voc.m_nodes_pool[0];
    file.read(reinterpret_cast<char*>(&voc.m_nodes_pool[0]),
            sizeof(Node) * voc.m_nodes_pool.size());
    file.close();
    return static_cast<bool>(file);
}

template<typename T, int type>
bool TemplatedVocabulary<T, type>::SerializerVersion1::load(
        TemplatedVocabulary<T, type>& voc, const char* buffer, size_t buffer_size) {
    if (buffer_size < sizeof(uint32_t) * 3) return false;
    const uint32_t* meta = reinterpret_cast<const uint32_t*>(buffer);
    if (!parseMeta(meta, voc.m_k, voc.m_L)) return false;
    voc.m_nodes_pool.clear();
    voc.m_nodes = reinterpret_cast<const Node*>(buffer + sizeof(uint32_t) * 3);
    // m_nodes points to the original address of the buffer given to
    // loadFromMemory + 16 bytes, so alignment should be ok
    return true;
}

template<typename T, int type>
bool TemplatedVocabulary<T, type>::SerializerVersion1::parseMeta(
        const uint32_t buffer[3], int& k, int& L) {
    // sanity check for minimum architecture interopability
    uint32_t sizeof_node = le32toh(buffer[0]);
    if (sizeof_node != sizeof(Node) || !host_is_little_endian())
        return false;
    k = le32toh(buffer[1]);
    L = le32toh(buffer[2]);
    return (k > 1 && L > 0);
}

template<typename T, int type>
bool TemplatedVocabulary<T, type>::SerializerLegacyVersion::load(
        TemplatedVocabulary<T, type>& voc,
        uint32_t number_of_nodes, std::ifstream& file) {
    if (sizeof(T) != 32) return false;  // this is not an orb descriptor
    if (number_of_nodes == 0) return true;

    voc.m_k = 10;  // hardcoded
    voc.m_L = 6;
    voc.m_nodes_pool.resize(calculateNodes(voc.m_k, voc.m_L));
    voc.m_nodes = &voc.m_nodes_pool[0];

    // each node encoded in little endian
    // 4 bytes: number of children
    // 4 bytes: id of first child, or weight (float)
    // 32 bytes: orb descriptor
    constexpr int buffer_size = 40;
    char buffer[buffer_size];

    file.read(buffer, buffer_size);  // skip root, next node is the first child

    std::function<void(WordId,WordId,int)> read_recursively =
            [&](WordId begin, WordId end, int current_level) {
        std::vector<int> group_children;
        group_children.reserve(voc.m_k);
        for (WordId id = begin; id != end; ++id) {
            file.read(buffer, buffer_size);
            group_children.push_back(le32toh(
                                         *reinterpret_cast<uint32_t*>(buffer)));
            voc.m_nodes_pool[id].children = le32toh(
                        *reinterpret_cast<uint32_t*>(buffer + 4));  // weight
            le8toh(reinterpret_cast<const uint8_t*>(buffer + 8), 32,
                   reinterpret_cast<uint8_t*>(&voc.m_nodes_pool[id].descriptor));
        }

        for (WordId id = begin; id != end; ++id) {
            int children = group_children[id - begin];

            if (current_level < voc.m_L - 1) {
                if (children > 0) {  // inner node
                    voc.m_nodes_pool[id].children = children;
                    WordId next_begin = voc.firstChild(id);
                    read_recursively(next_begin, next_begin + children,
                                     current_level + 1);
                } else {
                    // extend to the real leaf
                    float weight = voc.m_nodes[id].weight;
                    voc.m_nodes_pool[id].children = 1;
                    WordId node = voc.firstChild(id);
                    int level = current_level + 1;
                    for (; level < voc.m_L - 1; ++level, node = voc.firstChild(node)) {
                        voc.m_nodes_pool[node].children = 1;
                        voc.m_nodes_pool[node].descriptor = voc.m_nodes[id].descriptor;
                    }
                    voc.m_nodes_pool[node].weight = weight;
                    voc.m_nodes_pool[node].descriptor = voc.m_nodes[id].descriptor;
                }
            }
        }
    };

    if (number_of_nodes - 1 < voc.m_k) {
        read_recursively(0, number_of_nodes - 1, 0);
    } else {
        read_recursively(0, voc.m_k, 0);
    }

    if (number_of_nodes < voc.m_k) {
        // special case for very few nodes
        for (int i = number_of_nodes; i < voc.m_k; ++i) {
            voc.m_nodes_pool[i].descriptor = voc.m_nodes_pool[0].descriptor;
            voc.m_nodes_pool[i].children = 0;
        }
    }

    file.close();
    return static_cast<bool>(file);
}

template<typename T, int type>
size_t TemplatedVocabulary<T, type>::powInt(size_t base, size_t positive_exp) {
    // assume (exp > 0)
    size_t ret = 1;
    while (positive_exp) {
        if (positive_exp & 1) {
            ret *= base;
        }
        positive_exp >>= 1;
        base *= base;
    }
    return ret;
}

template<typename T, int type>
size_t TemplatedVocabulary<T, type>::calculateInnerNodes(int k, int L) {
    // sum (i = 1..L-1) k^i
    return (L > 0 ? (powInt(k, L) - 1) / (k - 1) - 1 : 0);
}

template<typename T, int type>
size_t TemplatedVocabulary<T, type>::calculateWords(int k, int L) {
    // k^L (if L > 0)
    return (L > 0 ? powInt(k, L) : 0);
}

template<typename T, int type>
size_t TemplatedVocabulary<T, type>::calculateNodes(int k, int L) {
    // sum (i = 1..L) k^i
    return (L > 0 ? (powInt(k, L + 1) - 1) / (k - 1) - 1 : 0);
}

}  // namespace DBoW2

#endif
