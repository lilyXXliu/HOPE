#ifndef OPE_TREE_H
#define OPE_TREE_H

#include <common.hpp>
#include <string>
#include <utility>
#include "art_dic_N.hpp"

namespace ope {
class ArtDicTree {
 public:
  ArtDicTree();

  ~ArtDicTree();

  Code lookup(const char *symbol, int symbol_len, int &prefix_len) const;

  bool build(const std::vector<SymbolCode> &symbol_code_list);

  static int getN4Num();

  static int getN16Num();

  static int getN48Num();

  static int getN256Num();

 private:
  N *root;

  void insert(LeafInfo *leafinfo);

  bool prefixMatch(N *node, const uint8_t *key, int key_size, int &key_level, int &node_level, uint8_t *common_prefix) const;

  N *spawn(uint8_t *common_prefix, N *node, std::string key, N *val, int node_level, int key_level, N *parent_node,
           uint8_t parent_key);

  void addLeaf(int insertkey_level, std::string key, N *node, N *val, N *parent_node, uint8_t parent_key);

  LeafInfo *getLeftBottom(N *node) const;

  LeafInfo *getRightBottom(N *node) const;

  static int getCommonPrefixLen(std::string &str1, std::string &str2);

  static std::string getPrevString(const std::string &str);

  static void printTree(N *node);
};

ArtDicTree::ArtDicTree() : root(new N256(nullptr, 0)){};

ArtDicTree::~ArtDicTree() {
  N::deleteChildren(root);
  N::deleteNode(root);
}

Code ArtDicTree::lookup(const char *symbol, const int symbol_len, int &prefix_len) const {
  N *node = nullptr;
  N *next_node = root;
  int key_level = 0;
  int node_level = 0;
  uint8_t common_prefix[maxPrefixLen];

  while (true) {
    node = next_node;
    if (prefixMatch(node, (uint8_t *)symbol, symbol_len, key_level, node_level, common_prefix)) {
      if (key_level == symbol_len) {
        auto leaf_info = reinterpret_cast<LeafInfo *>(N::getValueFromLeaf(node->getPrefixLeaf()));
        if (leaf_info == nullptr) {
          N *next = N::getFirstChild(node);
          auto nowLeaf = getLeftBottom(next);
          leaf_info = nowLeaf->prev_leaf;
        }
        prefix_len = leaf_info->prefix_len;
        return leaf_info->symbol_code->second;
      }
      const uint8_t &next_level_key_chr = static_cast<uint8_t>(symbol[key_level]);
      next_node = N::getChild(next_level_key_chr, node);
      if (next_node == nullptr) {
        // Get previous child
        N *prev = N::getPrevChild(node, next_level_key_chr);
        LeafInfo *leaf_info = nullptr;
        if (prev == nullptr) {
          N *next = N::getNextChild(node, next_level_key_chr);
          assert(next != nullptr);
          LeafInfo *next_leaf_info = getLeftBottom(next);
          leaf_info = next_leaf_info->prev_leaf;
        } else {
          leaf_info = getRightBottom(prev);
        }
        prefix_len = leaf_info->prefix_len;
        return leaf_info->symbol_code->second;
      }
      if (N::isLeaf(next_node)) {
        auto leaf_info = reinterpret_cast<LeafInfo *>(N::getValueFromLeaf(next_node));
        prefix_len = leaf_info->prefix_len;
        return leaf_info->symbol_code->second;
      }
    } else {
      LeafInfo *leaf_info = nullptr;
      if (key_level == symbol_len ||
          static_cast<uint8_t>(symbol[key_level]) < static_cast<uint8_t>(node->getPrefix()[node_level])) {
        N *prev = N::getFirstChild(node);
        leaf_info = getLeftBottom(prev)->prev_leaf;
      } else {
        N *next = N::getLastChild(node);
        leaf_info = getRightBottom(next);
      }
      prefix_len = leaf_info->prefix_len;
      return leaf_info->symbol_code->second;
    }
    key_level++;
  }
}

bool ArtDicTree::build(const std::vector<SymbolCode> &symbol_code_list) {
  // REQUIRE: symbol has been sorted
  LeafInfo *prev_leaf = nullptr;

  for (auto iter = symbol_code_list.begin(); iter != symbol_code_list.end(); iter++) {
    auto lf = new LeafInfo();
    std::string start_interval = iter->first;
    if (iter != symbol_code_list.end() - 1) {
      std::string end_interval = getPrevString((iter + 1)->first);
      lf->prefix_len = (uint32_t)getCommonPrefixLen(start_interval, end_interval);
      // assert(lf->prefix_len > 0)
    } else {
      lf->prefix_len = 1;
    }
    lf->prev_leaf = prev_leaf;
    lf->symbol_code = &(*iter);
    if (iter - symbol_code_list.begin() == 86) std::cout << "";
    if (iter == symbol_code_list.end() - 1) std::cout << "";
    insert(lf);
    prev_leaf = lf;
  }
  return true;
}

void ArtDicTree::insert(LeafInfo *leafInfo) {
  const SymbolCode *symbol_code = leafInfo->symbol_code;
  std::string key = symbol_code->first;
  N *node = nullptr;
  N *next_node = root;
  N *parent_node = nullptr;
  uint8_t parent_key = 0;
  uint8_t node_key = 0;
  int key_level = 0;
  int node_level = 0;
  N *val = N::setLeaf(reinterpret_cast<N *>(reinterpret_cast<int64_t>(leafInfo)));

  while (true) {
    parent_node = node;
    parent_key = node_key;
    node = next_node;
    uint8_t common_prefix[maxPrefixLen];
    if (prefixMatch(node, (uint8_t *)key.c_str(), (uint32_t)key.size(), key_level, node_level, common_prefix)) {
      // left_key == prefix
      if (key_level == (int)key.size()) {
        // N::insertOrUpdateNode(node, parent_node, parent_key, 0, val);
        node->setPrefixLeaf(val);
        return;
      }

      node_key = reinterpret_cast<uint8_t &>(key[key_level]);
      next_node = N::getChild(node_key, node);
      if (next_node == nullptr) {
        addLeaf(key_level, key, node, val, parent_node, parent_key);
        return;
      } else if (N::isLeaf(next_node)) {
        uint8_t prefix[maxPrefixLen];
        N *new_node = new N4(prefix, 0);
        new_node->setPrefixLeaf(next_node);
        N::insertOrUpdateNode(node, parent_node, parent_key, reinterpret_cast<uint8_t &>(key[key_level]), new_node);
        addLeaf(key_level + 1, key, new_node, val, node, reinterpret_cast<uint8_t &>(key[key_level]));
        return;
      }
      key_level++;
    } else {
      auto new_node = spawn(common_prefix, node, key, val, node_level, key_level, parent_node, parent_key);
      N::change(parent_node, parent_key, new_node);
      return;
    }
  }
}

void ArtDicTree::printTree(N *node) {
  if (N::isLeaf(node)) {
    std::cout << "Leaf: " << node << std::endl;
    return;
  }
  std::cout << "Prefix:";
  for (int i = 0; i < (int)node->prefix_len; i++) std::cout << node->getPrefix()[i];
  std::cout << " Prefix_len:" << node->prefix_len << " Child cnt:" << (int)node->count << std::endl;
  uint8_t keys[300];
  N *children[300];
  int n = 0;
  N::getChildren(node, 0, 255, keys, children, n);
  for (int i = 0; i < node->count; i++) {
    std::cout << "Child " << i << std::endl;
    printTree(children[i]);
  }
}

void ArtDicTree::addLeaf(int insertkey_level, std::string key, N *node, N *val, N *parent_node, uint8_t parent_key) {
  if (insertkey_level == (int)key.size()) {
    // N::insertOrUpdateNode(node, parent_node, parent_key, 0, val);
    if (node->prefix_len != 0) std::cout << "[Error] Prefix is not empty" << std::endl;
    node->setPrefixLeaf(val);
    return;
  }

  if (insertkey_level == (int)key.size() - 1) {
    N::insertOrUpdateNode(node, parent_node, parent_key, (uint8_t)(key[insertkey_level]), val);
    return;
  }
  auto key_cstr = reinterpret_cast<const uint8_t *>(key.c_str());
  int tmp_level = insertkey_level + 1;
  N *leaf =
      new N4((uint8_t *)(key_cstr + tmp_level),
             (uint32_t)(key.length() - tmp_level) < maxPrefixLen ? (uint32_t)(key.length() - tmp_level) : maxPrefixLen);
  N::insertOrUpdateNode(node, parent_node, parent_key, (uint8_t)(key[insertkey_level]), leaf);
  //        printTree(root);
  tmp_level += maxPrefixLen;
  if (tmp_level < (int)key.size()) {
    //            std::cout << "I am spawing " << tmp_level << std::endl;
    addLeaf(tmp_level, key, leaf, val, node, (uint8_t)(key[insertkey_level]));
  } else {
    leaf->setPrefixLeaf(val);
  }
}

// break the node
N *ArtDicTree::spawn(uint8_t *common_prefix, N *node, std::string key, N *val, int node_level, int key_level,
                     N *parent_node, uint8_t parent_key) {
  auto node_new = new N4(common_prefix, (uint32_t)node_level);
  // Insert given value
  addLeaf(key_level, std::move(key), node_new, val, parent_node, parent_key);
  // The order of the next two lines cannot be changed!
  node_new->insert(node->getPrefix()[node_level], node);
  assert(node_level < (int)node->prefix_len);
  node->setPrefix(node->getPrefix() + node_level + 1, (int)node->prefix_len - node_level - 1);
  return node_new;
}

bool ArtDicTree::prefixMatch(N *node, const uint8_t *key, int key_size, int &key_level, int &node_level,
                             uint8_t *common_prefix) const {
  int i = 0;
  uint8_t *node_prefix = node->getPrefix();
  for (; i < (int)node->prefix_len; i++) {
    if (key_level + i >= key_size || key[key_level + i] != node_prefix[i]) {
      node_level = i;
      key_level += i;
      return false;
    }
    common_prefix[i] = node_prefix[i];
  }
  node_level = i;
  key_level += node_level;
  return true;
}

LeafInfo *ArtDicTree::getLeftBottom(N *node) const {
  while (true) {
    if (N::isLeaf(node)) return reinterpret_cast<LeafInfo *>(N::getValueFromLeaf(node));
    node = N::getFirstChild(node);
  }
}

LeafInfo *ArtDicTree::getRightBottom(N *node) const {
  while (true) {
    if (N::isLeaf(node)) return reinterpret_cast<LeafInfo *>(N::getValueFromLeaf(node));
    node = N::getLastChild(node);
  }
}

int ArtDicTree::getCommonPrefixLen(std::string &str1, std::string &str2) {
  int min_len = (int)std::min(str1.size(), str2.size());
  int i = 0;
  for (; i < min_len; i++) {
    if (str1[i] != str2[i]) return i;
  }
  return i;
}

std::string ArtDicTree::getPrevString(const std::string &str) {
  bool end_with_startchr = false;
  for (int i = (int)str.size() - 1; i >= 0; i--) {
    if (uint8_t(str[i]) == 0) {
      end_with_startchr = true;
      continue;
    }
    std::string addchr;
    if (!end_with_startchr) {
      char prev_chr = str[i] - 1;
      addchr = std::string(1, prev_chr);
    } else
      addchr = std::string(1, str[i]);
    return str.substr(0, i) + addchr;
  }
  assert(false);
  return std::string();
}

int ArtDicTree::getN4Num() { return cnt_N4; }

int ArtDicTree::getN16Num() { return cnt_N16; }

int ArtDicTree::getN48Num() { return cnt_N48; }

int ArtDicTree::getN256Num() { return cnt_N256; }
}  // namespace ope

#endif  // OPE_TREE_H
