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

#include <fstream>
#include <random>
#include <vector>
#include <unordered_map>
#include "endianness.h"

namespace DBoW2 {

typedef uint32_t WordId;
typedef float WordValue;
typedef std::unordered_map<WordId, WordValue> BowVector;
typedef std::unordered_map<WordId, std::vector<unsigned int>> FeatureVector;

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

    /** Returns the normalized tf-idf histogram of some descriptors.
     */
    BowVector transform(const std::vector<T>& descriptors) const;

    /** Returns the normalized tf-idf histogram of some descriptors
     * and their indices arranged by words.
     * @param[out] fv indices of descriptors arranged by words.
     * @param levelsup number of levels to go up the tree from the leaves
     * to select the indexing nodes (higher = coarser groups).
     */
    BowVector transform(const std::vector<T>& descriptors,
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

    std::vector<Node> m_nodes_pool;
    const Node* m_nodes;  // points to m_nodes_pool or to external data

    int m_k;  // branching factor
    int m_L;  // max levels (L-1 levels of inner nodes + 1 last level of words)

 private:
    /** Transforms descriptors into words.
     */
    BowVector transform(const typename std::vector<T>& descriptors,
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
    m_k(0), m_L(0), m_nodes(nullptr) {}

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

template<typename T, int type>
BowVector TemplatedVocabulary<T, type>::transform(
        const std::vector<T>& descriptors) const {
    return transform(descriptors, nullptr, 0);
}

template<typename T, int type>
BowVector TemplatedVocabulary<T, type>::transform(
        const std::vector<T>& descriptors, FeatureVector& fv,
        int levelsup) const {
    return transform(descriptors, &fv, levelsup);
}

template<typename T, int type>
BowVector TemplatedVocabulary<T, type>::transform(
        const std::vector<T>& descriptors, FeatureVector* fv,
        int levelsup) const {
    if (fv) fv->clear();
    if (!m_nodes) return BowVector();

    BowVector bow;
    WordId fv_node;
    for (size_t descriptor_idx = 0; descriptor_idx < descriptors.size();
         ++descriptor_idx) {
        const auto& descriptor = descriptors[descriptor_idx];
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
