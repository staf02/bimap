#pragma once

#include <cassert> // assert
#include <climits>
#include <iterator> // std::reverse_iterator
#include <random>
#include <utility> // std::pair, std::swap

namespace intrusive {
struct default_tag;

template <typename U, typename T, typename Base, typename Tag = default_tag,
    typename Compare = std::less<Base>>
    struct set;

struct set_element_base {

    static std::default_random_engine engine;
    static std::uniform_int_distribution<uint32_t> random_elem;

    set_element_base() : y(random_elem(engine)) {};

    set_element_base(set_element_base&& other) {
        std::swap(other.left, left);
        std::swap(other.right, right);
        std::swap(other.parent, parent);
        other.unlink();
    }

    void swap(set_element_base& other) {
        std::swap(other.left, left);
        std::swap(other.right, right);
        std::swap(other.parent, parent);
    }

    set_element_base(set_element_base const& other) = delete;

    bool has_parent() {
        return parent != nullptr;
    }

    ~set_element_base() {
        unlink();
    }

    template <typename U, typename T, typename Base, typename Tag,
        typename Compare>
    friend struct set;

private:
    set_element_base* parent{ nullptr };
    set_element_base* left{ nullptr };
    set_element_base* right{ nullptr };
    int y;

    template <typename T, typename Tag>
    friend struct intrusive_list;

    void unlink() {
        left = right = parent = nullptr;
    }
};

template <typename Tag = default_tag>
struct set_element : set_element_base {};

template <typename U, typename T, typename Base, typename Tag, typename Compare>
struct set : Compare {
public:
    set_element<Tag>* root;

    static Base& get_value(set_element_base* tr) {
        return (static_cast<T*>(
            static_cast<U*>(static_cast<set_element<Tag>*>(tr))))
            ->elem;
    }

    static std::pair<set_element_base*, set_element_base*>
        split(set_element_base* tr, Base const& value, Compare const& comparator,
            bool lower = true) {
        if (tr == nullptr) {
            return { nullptr, nullptr };
        }
        if ((comparator(value, get_value(tr))) ||
            (lower && value == get_value(tr))) {
            auto p = split(tr->left, value, comparator, lower);
            set_left(tr, p.second);
            return { p.first, tr };
        }
        else {
            auto p = split(tr->right, value, comparator, lower);
            set_right(tr, p.first);
            return { tr, p.second };
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
        }
        else {
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

    static set_element_base* get_next(set_element_base* tr) {
        if (tr->right != nullptr) {
            tr = get_min(tr->right);
        }
        else {
            while (tr->parent->right == tr) {
                tr = tr->parent;
            }
            tr = tr->parent;
        }
        return tr;
    }

    static set_element_base* get_prev(set_element_base* tr) {
        if (tr->left != nullptr) {
            tr = get_max(tr->left);
        }
        else {
            while (tr->parent->left == tr) {
                tr = tr->parent;
            }
            tr = tr->parent;
        }
        return tr;
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
    set() : root(nullptr), Compare(Compare()) {}

    set(Compare const& comparator) : root(nullptr), Compare(comparator) {}

    set(set_element<Tag>* root, Compare const& comparator)
        : root(root), Compare(comparator) {}

    void set_root(set_element<Tag>* new_root) {
        root = new_root;
    }

    set(set const& other) = delete;

    set& operator=(set const& other) = delete;

    set(set&& other) : root(std::move(other.root)) {}

    // O(n) nothrow
    ~set() = default;

    set_element<Tag>* begin() const {
        if (root->left == nullptr) {
            return end();
        }
        return static_cast<set_element<Tag>*>(get_min(root->left));
    }

    set_element<Tag>* end() const {
        return const_cast<set_element<Tag>*>(root);
    }

    // O(h) nothrow
    set_element<Tag>* insert(T* value) {
        auto p = split(root->left, value->elem,
            *static_cast<Compare*>(const_cast<set*>(this)));
        auto p1 = split(p.second, value->elem,
            *static_cast<Compare*>(const_cast<set*>(this)), false);
        auto tmp = p1.first;
        if (p1.first == nullptr) {
            tmp = static_cast<set_element_base*>(
                static_cast<set_element<Tag>*>(static_cast<U*>(value)));
        }
        p.second = merge(tmp, p1.second);
        set_left(root, merge(p.first, p.second));
        return static_cast<set_element<Tag>*>(tmp);
    }

    // O(h) nothrow
    void erase(set_element_base* ptr) {
        if (ptr == root) {
            return;
        }
        if (ptr->parent->left == ptr) {
            set_left(ptr->parent, merge(ptr->left, ptr->right));
        }
        else {
            set_right(ptr->parent, merge(ptr->left, ptr->right));
        }
        ptr->unlink();
        return;
    }

    // O(h) strong
    set_element<Tag>* find(Base const& value) const {
        auto it = lower_bound(value);
        if (it != end() && get_value(it) == value) {
            return it;
        }
        return end();
    }

    set_element<Tag>* lower_bound(Base const& value) const {
        auto p = split(root->left, value,
            *static_cast<Compare*>(const_cast<set*>(this)));
        auto m = get_min(p.second);
        auto res = m == nullptr ? end() : static_cast<set_element<Tag>*>(m);
        set_left(const_cast<set_element<Tag>*>(root), merge(p.first, p.second));
        return res;
    }

    set_element<Tag>* upper_bound(Base const& value) const {
        auto it = lower_bound(value);
        if (it != end()) {
            if (get_value(it) == value) {
                it++;
            }
        }
        return it;
    }

    void swap(set& other) {
        auto ptr = root->left;
        set_left(root, other.root->left);
        set_left(other.root, ptr);
    }

    friend void swap(set& a, set& b) noexcept {
        a.swap(b);
    }
};
}