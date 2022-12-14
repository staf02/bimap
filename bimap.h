#pragma once

#include "intrusive-set.h"
#include <cassert>  // assert
#include <iterator> // std::reverse_iterator
#include <random>
#include <utility> // std::pair, std::swap

template <typename Left, typename Right, typename CompareLeft = std::less<Left>, 
    typename CompareRight = std::less<Right>>
struct bimap {

private:
    template <class T, class Type>
    struct base_element {
        T elem;

        explicit base_element(T const& elem) : elem(elem) {}
        explicit base_element(T&& elem) : elem(std::move(elem)) {}
    };

public:
    using left_t = base_element<Left, struct l_tmp>;
    using right_t = base_element<Right, struct r_tmp>;

private:
    struct root_pair : intrusive::set_element<struct left_tag>,
        intrusive::set_element<struct right_tag> {
        root_pair() = default;
    };

    struct pair : root_pair, left_t, right_t {
        explicit pair(Left l, Right r)
            : left_t(std::move(l)), right_t(std::move(r)) {}
        pair() = default;
    };

public:
    using node_t = pair;

private:
    intrusive::set<node_t, left_t, Left, left_tag, CompareLeft> l;
    intrusive::set<node_t, right_t, Right, right_tag, CompareRight> r;
    root_pair root;
    size_t size_{ 0 };

public:
    template <class T, class Base, class Tag, class Compare, class OtherT,
        class OtherBase, class OtherTag, class OtherCompare>
    struct bimap_iterator {
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Base const;
        using pointer = Base const*;
        using reference = Base const&;

        bimap_iterator() = default;

        reference operator*() const {
            return static_cast<T*>((static_cast<node_t*>(it)))->elem;
        }

        pointer operator->() const {
            return &(static_cast<T*>((static_cast<node_t*>(it)))->elem);
        }

        bimap_iterator& operator++() {
            it = static_cast<intrusive::set_element<Tag>*>(
                intrusive::set<node_t, Base, Tag, Compare>::get_next(
                    static_cast<intrusive::set_element_base*>(it)));
            return *this;
        }

        bimap_iterator operator++(int) {
            bimap_iterator old = *this;
            ++(*this);
            return old;
        }

        bimap_iterator& operator--() {
            it = static_cast<intrusive::set_element<Tag>*>(
                intrusive::set<node_t, Base, Tag, Compare>::get_prev(
                    static_cast<intrusive::set_element_base*>(it)));
            return *this;
        }

        bimap_iterator operator--(int) {
            bimap_iterator old = *this;
            --(*this);
            return old;
        }

        bimap_iterator<OtherT, OtherBase, OtherTag, OtherCompare, T, Base, Tag,
            Compare>
            flip() const {
            return bimap_iterator<OtherT, OtherBase, OtherTag, OtherCompare, T, Base,
                Tag, Compare>(
                    static_cast<intrusive::set_element<OtherTag>*>(
                        static_cast<node_t*>(it)));
        }

        bool operator==(bimap_iterator const& other) const {
            return it == other.it;
        }

        bool operator!=(bimap_iterator const& other) const {
            return it != other.it;
        }

    private:
        friend struct bimap;

        intrusive::set_element<Tag>* get_ptr() {
            return it;
        }

        intrusive::set_element<Tag>* it;

        explicit bimap_iterator(intrusive::set_element<Tag>* ptr) : it(ptr) {}
    };

    using left_iterator = bimap_iterator<left_t, Left, left_tag, CompareLeft,
        right_t, Right, right_tag, CompareRight>;
    using right_iterator = bimap_iterator<right_t, Right, right_tag, CompareRight,
        left_t, Left, left_tag, CompareLeft>;

public:
    // ?????????????? bimap ???? ???????????????????? ???? ?????????? ????????.
    bimap(CompareLeft left_cmp = CompareLeft(),
        CompareRight right_cmp = CompareRight())
        : l(static_cast<intrusive::set_element<left_tag>*>(&root), left_cmp),
        r(static_cast<intrusive::set_element<right_tag>*>(&root), right_cmp) {}

    // ???????????????????????? ???? ???????????? ?? ????????????????????????
    bimap(bimap const& other)
        : l(static_cast<intrusive::set_element<left_tag>*>(&root), CompareLeft()),
        r(static_cast<intrusive::set_element<right_tag>*>(&root),
            CompareRight()) {
        auto it = other.begin_left();
        while (it != other.end_left()) {
            auto right_it = it.flip();
            insert(*it, *right_it);
            it++;
        }
    }

    bimap(bimap&& other) noexcept = default;

    bimap& operator=(bimap const& other) {
        if (*this != other) {
            auto tmp = bimap<Left, Right, CompareLeft, CompareRight>(other);
            (*this).swap(tmp);
        }
        return *this;
    }

    bimap& operator=(bimap&& other) noexcept {
        if (*this != other) {
            bimap<Left, Right, CompareLeft, CompareRight>(std::move(other))
                .swap(*this);
        }
        return *this;
    }

    void swap(bimap& other) {
        l.swap(other.l);
        r.swap(other.r);
        std::swap(size_, other.size_);
    }

    ~bimap() {
        auto it = begin_left();
        while (it != end_left()) {
            auto tmp = std::next(it);
            auto ptr = it.get_ptr();
            node_t* parent_ptr = static_cast<node_t*>(ptr);
            l.erase(static_cast<intrusive::set_element<left_tag>*>(parent_ptr));
            delete parent_ptr;
            it = tmp;
            size_--;
        }
    }

    left_iterator insert(Left left, Right right) {
        if (find_left(left) == end_left() && find_right(right) == end_right()) {
            node_t* tmp = new pair(std::move(left), std::move(right));
            auto it = l.insert(static_cast<left_t*>(tmp));
            r.insert(static_cast<right_t*>(tmp));
            size_++;
            return left_iterator(it);
        }
        return end_left();
    }

    left_iterator erase_left(left_iterator it) {
        return erase_left(it, std::next(it));
    }

    bool erase_left(Left const& left) {
        auto it = find_left(left);
        if (it != end_left()) {
            erase_left(it);
            return true;
        }
        return false;
    }

    right_iterator erase_right(right_iterator it) {
        return erase_right(it, std::next(it));
    }

    bool erase_right(Right const& right) {
        auto it = find_right(right);
        if (it != end_right()) {
            erase_right(it);
            return true;
        }
        return false;
    }

private:
    template <class IteratorType>
    IteratorType erase_range(IteratorType first, IteratorType last) {
        auto it = first;
        while (it != last) {
            auto tmp = std::next(it);
            auto ptr = it.get_ptr();
            node_t* parent_ptr = static_cast<node_t*>(ptr);
            l.erase(static_cast<intrusive::set_element<left_tag>*>(parent_ptr));
            r.erase(static_cast<intrusive::set_element<right_tag>*>(parent_ptr));
            delete parent_ptr;
            it = tmp;
            size_--;
        }
        return last;
    }

public:
    left_iterator erase_left(left_iterator first, left_iterator last) {
        return erase_range<left_iterator>(first, last);
    }

    right_iterator erase_right(right_iterator first, right_iterator last) {
        return erase_range<right_iterator>(first, last);
    }

    // ???????????????????? ???????????????? ???? ????????????????. ???????? ???? ???????????? - ?????????????????????????????? end()
    left_iterator find_left(Left const& left) const {
        return left_iterator(l.find(left));
    }

    right_iterator find_right(Right const& right) const {
        return right_iterator(r.find(right));
    }

    // ???????????????????? ?????????????????????????????? ?????????????? ???? ????????????????
    // ???????? ???????????????? ???? ???????????????????? -- ?????????????? std::out_of_range
    Right const& at_left(Left const& key) const {
        auto it = find_left(key);
        if (it == end_left()) {
            throw std::out_of_range("");
        }
        return *(it.flip());
    }

    Left const& at_right(Right const& key) const {
        auto it = find_right(key);
        if (it == end_right()) {
            throw std::out_of_range("");
        }
        return *(it.flip());
    }

    template <typename T = Right,
        std::enable_if_t<std::is_default_constructible_v<T>, int> = 0>
    Right const& at_left_or_default(Left const& key) {
        auto it = find_left(key);
        if (it != end_left()) {
            return *it.flip();
        }
        erase_right(Right());
        return *(insert(key, Right()).flip());
    }

    template <typename T = Left,
        std::enable_if_t<std::is_default_constructible_v<T>, int> = 0>
    Left const& at_right_or_default(Right const& key) {
        auto it = find_right(key);
        if (it != end_right()) {
            return *it.flip();
        }
        erase_left(Left());
        return *(insert(Left(), key));
    }

    left_iterator lower_bound_left(const Left& left) const {
        return left_iterator(l.lower_bound(left));
    }
    left_iterator upper_bound_left(const Left& left) const {
        return left_iterator(l.upper_bound(left));
    }

    right_iterator lower_bound_right(const Right& left) const {
        return right_iterator(r.lower_bound(left));
    }

    right_iterator upper_bound_right(const Right& left) const {
        return right_iterator(r.upper_bound(left));
    }

    left_iterator begin_left() const {
        return left_iterator(l.begin());
    }

    left_iterator end_left() const {
        return left_iterator(l.end());
    }

    right_iterator begin_right() const {
        return right_iterator(r.begin());
    }

    right_iterator end_right() const {
        return right_iterator(r.end());
    }

    bool empty() const {
        return size_ == 0;
    }

    std::size_t size() const {
        return size_;
    }

public:
    friend bool operator==(bimap const& a, bimap const& b) {
        if (a.size() != b.size()) {
            return false;
        }
        auto a_it = a.begin_left(), b_it = b.begin_left();
        while (a_it != a.end_left()) {
            auto a_right_it = a_it.flip(), b_right_it = b_it.flip();
            if (!(*a_it == *b_it) || !(*a_right_it == *b_right_it)) {
                return false;
            }
            a_it++;
            b_it++;
        }
        return true;
    }

    friend bool operator!=(bimap const& a, bimap const& b) {
        return !(a == b);
    }
};
