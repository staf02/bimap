#pragma once

#include <cassert>  // assert
#include <iterator> // std::reverse_iterator
#include <random>
#include <utility> // std::pair, std::swap
#include "bimap.h"

namespace intrusive {

struct default_tag;

template <typename T, typename Base, typename Tag = default_tag,
          typename Compare = std::less<Base>>
struct set;

struct set_element_base {
  set_element_base() : y(rand()){};

  set_element_base(set_element_base&& other) {
    std::swap(other.left, left);
    std::swap(other.right, right);
    std::swap(other.parent, parent);
    other.unlink();
  }

  set_element_base(set_element_base const& other) = delete;

  ~set_element_base() {
    unlink();
  }

  template <typename T, typename Base, typename Tag, typename Compare>
  friend struct set;

private:
  set_element_base* parent{nullptr};
  set_element_base* left{nullptr};
  set_element_base* right{nullptr};
  int y;

  template <typename T, typename Tag>
  friend struct intrusive_list;

  void unlink() {
    left = right = parent = nullptr;
  }
};

template <typename Tag = default_tag>
struct set_element : set_element_base {};

template <typename T, typename Base, typename Tag, typename Compare>
struct set {
private:
  set_element<Tag> root;

  static Base& get_value(set_element_base* tr) {
    return (static_cast<T*>(tr))->elem;
  }

  std::pair<set_element_base*, set_element_base*>
  static split(set_element_base* tr, Base const& value, bool lower = false) {
    if (tr == nullptr) {
      return {nullptr, nullptr};
    }
    if ((!lower &&
         (comparator(value, get_value(tr)) || value == get_value(tr))) ||
        (lower && comparator(value, get_value(tr)))) {
      auto p = split(tr->left, value, lower);
      set_left(tr, p.second);
      return {p.first, tr};
    } else {
      auto p = split(tr->right, value, lower);
      set_right(tr, p.first);
      return {tr, p.second};
    }
  }

  static set_element_base* merge(set_element_base* left,
                                 set_element_base* right) {
    if (!left || !right) {
      if (left) {
        return left;
      }
      return right;
    }
    if (left->y < right->y) {
      set_right(left, merge(left->right, right));
      return left;
    } else {
      set_left(right, merge(left, right->left));
      return right;
    }
  }

  static set_element_base* get_min(set_element_base* tr) {
    if (tr == nullptr) {
      return nullptr;
    }
    while (tr->left != nullptr) {
      tr = tr->left;
    }
    return tr;
  }

  static set_element_base* get_max(set_element_base* tr) {
    if (tr == nullptr) {
      return nullptr;
    }
    while (tr->right != nullptr) {
      tr = tr->right;
    }
    return tr;
  }

  static set_element_base* get_next(set_element_base* ptr) {
    if (ptr->right != nullptr) {
      ptr = get_min(ptr->right);
    } else {
      while (ptr->parent->right == ptr) {
        ptr = ptr->parent;
      }
      ptr = ptr->parent;
    }
    return ptr;
  }

  static set_element_base* get_prev(set_element_base* ptr) {
    if (ptr->left != nullptr) {
      ptr = get_max(ptr->left);
    } else {
      while (ptr->parent->left == ptr) {
        ptr = ptr->parent;
      }
      ptr = ptr->parent;
    }
    return ptr;
  }

  static void set_left(set_element_base* ptr, set_element_base* left) {
    ptr->left = left;
    if (left != nullptr) {
      left->parent = ptr;
    }
  }

  static void set_right(set_element_base* ptr, set_element_base* right) {
    ptr->right = right;
    if (right != nullptr) {
      right->parent = ptr;
    }
  }

public:
  template <typename DATA_TYPE>
  struct iterator;

  using const_iterator = iterator<Base>;
  using reverse_iterator = std::reverse_iterator<iterator<Base>>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  // O(1) nothrow
  set() : root() {}

  set(set const& other) = delete;

  set& operator=(set const& other) = delete;

  set(set&& other) : root(std::move(other.root)) {}

  // O(n) nothrow
  ~set() = default;

  const_iterator begin() const {
    if (root.left == nullptr) {
      return end();
    }
    auto ptr = static_cast<set_element<Tag>*>(get_min(root.left));
    return const_iterator(ptr);
  }

  const_iterator end() const {
    return const_iterator(const_cast<set_element<Tag>*>(&root));
  }

  // O(h) nothrow
  iterator<Base> insert(T& value) {
    auto p = split(root.left, value.elem);
    auto p1 = split(p.second, value.elem, true);
    auto tmp = p1.first;
    bool result = false;
    if (p1.first == nullptr) {
      tmp = static_cast<set_element_base*>(&value);
      result = true;
    }
    p.second = merge(tmp, p1.second);
    set_left(&root, merge(p.first, p.second));
    return iterator<Base>(static_cast<set_element<Tag>*>(tmp));
  }

  // O(h) nothrow
  iterator<Base> erase(set_element_base* ptr) {
    if (ptr == &root) {
      return end();
    }
    if (ptr->parent->left == ptr) {
      set_left(ptr->parent, merge(ptr->left, ptr->right));
    } else {
      set_right(ptr->parent, merge(ptr->left, ptr->right));
    }
    ptr->unlink();
    return end();
  }

  // O(h) strong
  const_iterator find(Base const& value) const {
    auto it = lower_bound(value);
    if (it != end() && *it == value) {
      return it;
    }
    return end();
  }

  const_iterator lower_bound(Base const& value) const {
    auto p = split(root.left, value);
    auto m = get_min(p.second);
    const_iterator res = m == nullptr
                           ? end()
                           : const_iterator(static_cast<set_element<Tag>*>(m));
    set_left(const_cast<set_element<Tag>*>(&root), merge(p.first, p.second));
    return res;
  }

  const_iterator upper_bound(Base const& value) const {
    auto it = lower_bound(value);
    if (it != end()) {
      if (*it == value) {
        it++;
      }
    }
    return it;
  }

  void swap(set& other) {
    auto ptr = root.left;
    set_left(&root, other.root.left);
    set_left(&other.root, ptr);
  }

  friend void swap(set& a, set& b) noexcept {
    a.swap(b);
  }

  template <typename E>
  struct iterator {
  public:

    set_element<Tag>* ptr{nullptr};

    explicit iterator(set_element<Tag>* ptr) : ptr(ptr) {}

    friend set;

    template <class Left, class Right, class CompareLeft, class CompareRight>
    friend struct bimap;

  public:

    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = E const;
    using pointer = E const*;
    using reference = E const&;

    iterator() = default;

    iterator(iterator const& other) : ptr(other.ptr) {}

    reference operator*() const {
      return (*static_cast<T*>(ptr)).elem;
    }

    pointer operator->() const {
      return static_cast<T*>(ptr)->elem;
    }

    iterator& operator++() & {
      ptr = static_cast<set_element<Tag>*>(
          get_next(static_cast<set_element_base*>(ptr)));
      return *this;
    }

    iterator operator++(int) & {
      iterator old = *this;
      ++(*this);
      return old;
    }

    iterator& operator--() & {
      ptr = static_cast<set_element<Tag>*>(
          get_prev(static_cast<set_element_base*>(ptr)));
      return *this;
    }

    iterator operator--(int) & {
      iterator old = *this;
      --(*this);
      return old;
    }

    bool operator==(const_iterator const& other) const {
      return ptr == other.ptr;
    }

    bool operator!=(const_iterator const& other) const {
      return ptr != other.ptr;
    }
  };
};
} // namespace intrusive
