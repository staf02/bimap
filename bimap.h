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
  template <class T, class Tag>
  struct base_element : intrusive::set_element<Tag> {
    T elem;
    explicit base_element(T const& elem)
        : elem(elem), intrusive::set_element<Tag>() {}
    explicit base_element(T&& elem)
        : elem(std::move(elem)), intrusive::set_element<Tag>() {}
  };

public:
  using left_t = base_element<Left, struct left_tag>;
  using right_t = base_element<Right, struct right_tag>;

private:
  struct pair : left_t, right_t {
    explicit pair(Left const& l, Right const& r) : left_t(l), right_t(r) {}
    explicit pair(Left&& l, Right const& r)
        : left_t(std::move(l)), right_t(r) {}
    explicit pair(Left const& l, Right&& r)
        : left_t(l), right_t(std::move(r)) {}
    explicit pair(Left&& l, Right&& r)
        : left_t(std::move(l)), right_t(std::move(r)) {}
  };

  intrusive::set<left_t, Left, left_tag, CompareLeft> l;
  intrusive::set<right_t, Right, right_tag, CompareRight> r;
  size_t size_{0};

public:
  using node_t = pair;

  struct left_iterator;
  struct right_iterator;

  struct left_iterator {

    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Left const;
    using pointer = Left const*;
    using reference = Left const&;

    left_iterator() = default;

    left_iterator(typename intrusive::set<left_t, Left, left_tag,
                                          CompareLeft>::const_iterator it)
        : it(it) {}

    reference operator*() const {
      return *(it);
    }

    pointer operator->() const {
      return &(*it);
    }

    left_iterator& operator++() {
      it++;
      return *this;
    }

    left_iterator operator++(int) {
      left_iterator old = *this;
      ++(*this);
      return old;
    }

    left_iterator& operator--() {
      it--;
      return *this;
    }

    left_iterator operator--(int) {
      left_iterator old = *this;
      --(*this);
      return old;
    }

    right_iterator flip() const {
      right_t* new_ptr = static_cast<right_t*>(static_cast<node_t*>(it.ptr));
      return right_iterator(new_ptr);
    }

    bool operator==(left_iterator const& other) const {
      return it == other.it;
    }

    bool operator!=(left_iterator const& other) const {
      return it != other.it;
    }

    intrusive::set_element<left_tag>* get_ptr() {
      return it.ptr;
    }

    typename intrusive::set<left_t, Left, left_tag, CompareLeft>::const_iterator
        it;
    explicit left_iterator(left_t* ptr)
        : it(static_cast<intrusive::set_element<left_tag>*>(ptr)) {}
  };

  struct right_iterator {

    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Right const;
    using pointer = Right const*;
    using reference = Right const&;

    right_iterator() = default;

    right_iterator(typename intrusive::set<right_t, Right, right_tag,
                                           CompareRight>::const_iterator it)
        : it(it) {}

    reference operator*() const {
      return (*it);
    }

    pointer operator->() const {
      return &(*it);
    }

    right_iterator& operator++() {
      it++;
      return *this;
    }

    right_iterator operator++(int) {
      right_iterator old = *this;
      ++(*this);
      return old;
    }

    right_iterator& operator--() {
      it--;
      return *this;
    }

    right_iterator operator--(int) {
      right_iterator old = *this;
      --(*this);
      return old;
    }

    left_iterator flip() const {
      left_t* new_ptr = static_cast<left_t*>(static_cast<node_t*>(it.ptr));
      return left_iterator(new_ptr);
    }

    bool operator==(right_iterator const& other) const {
      return it == other.it;
    }

    bool operator!=(right_iterator const& other) const {
      return it != other.it;
    }

    intrusive::set_element<right_tag>* get_ptr() {
      return it.ptr;
    }

    typename intrusive::set<right_t, Right, right_tag,
                            CompareRight>::const_iterator it;
    explicit right_iterator(right_t* ptr)
        : it(static_cast<intrusive::set_element<right_tag>*>(ptr)) {}
  };

  // Создает bimap не содержащий ни одной пары.
  bimap(CompareLeft compare_left = CompareLeft(),
        CompareRight compare_right = CompareRight())
      : l(), r() {}

  // Конструкторы от других и присваивания
  bimap(bimap const& other) {
    auto it = other.begin_left();
    while (it != other.end_left()) {
      auto right_it = it.flip();
      insert(*it, *right_it);
      it++;
    }
  }

  bimap(bimap&& other) noexcept
      : l(std::move(other.l)), r(std::move(other.r)),
        size_(std::move(other.size_)) {}

  bimap& operator=(bimap const& other) {
    if (*this != other) {
      bimap<Left, Right, CompareLeft, CompareRight>(other).swap(*this);
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

  // Деструктор. Вызывается при удалении объектов bimap.
  // Инвалидирует все итераторы ссылающиеся на элементы этого bimap
  // (включая итераторы ссылающиеся на элементы следующие за последними).
  ~bimap() {
    erase_left(begin_left(), end_left());
  }

  // Вставка пары (left, right), возвращает итератор на left.
  // Если такой left или такой right уже присутствуют в bimap, вставка не
  // производится и возвращается end_left().
  left_iterator insert(Left left, Right right) {
    if (find_left(left) == end_left() && find_right(right) == end_right()) {
      node_t* tmp = new pair(std::move(left), std::move(right));
      auto it = l.insert(*static_cast<left_t*>(tmp));
      r.insert(*static_cast<right_t*>(tmp));
      size_++;
      return left_iterator(it);
    }
    return end_left();
  }

  // Удаляет элемент и соответствующий ему парный.
  // erase невалидного итератора неопределен.
  // erase(end_left()) и erase(end_right()) неопределены.
  // Пусть it ссылается на некоторый элемент e.
  // erase инвалидирует все итераторы ссылающиеся на e и на элемент парный к e.
  left_iterator erase_left(left_iterator it) {
    return erase_left(it, std::next(it));
  }
  // Аналогично erase, но по ключу, удаляет элемент если он присутствует, иначе
  // не делает ничего Возвращает была ли пара удалена
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

  // erase от ренжа, удаляет [first, last), возвращает итератор на последний
  // элемент за удаленной последовательностью

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

  // Возвращает итератор по элементу. Если не найден - соответствующий end()
  left_iterator find_left(Left const& left) const {
    return left_iterator(l.find(left));
  }

  right_iterator find_right(Right const& right) const {
    return right_iterator(r.find(right));
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует -- бросает std::out_of_range
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

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует, добавляет его в bimap и на противоположную
  // сторону кладет дефолтный элемент, ссылку на который и возвращает
  // Если дефолтный элемент уже лежит в противоположной паре - должен поменять
  // соответствующий ему элемент на запрашиваемый (смотри тесты)
  template <typename T = Right, std::enable_if_t<std::is_default_constructible_v<T>, int> = 0>
  Right const& at_left_or_default(Left const& key) {
    erase_right(Right());
    erase_left(key);
    return *(insert(key, Right()).flip());
  }

  template <typename T = Left, std::enable_if_t<std::is_default_constructible_v<T>, int> = 0>
  Left const& at_right_or_default(Right const& key) {
    erase_left(Left());
    erase_right(key);
    return *(insert(Left(), key));
  }

  // lower и upper bound'ы по каждой стороне
  // Возвращают итераторы на соответствующие элементы
  // Смотри std::lower_bound, std::upper_bound.
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

  // Возващает итератор на минимальный по порядку left.
  left_iterator begin_left() const {
    return left_iterator(l.begin());
  }
  // Возващает итератор на следующий за последним по порядку left.
  left_iterator end_left() const {
    return left_iterator(l.end());
  }

  // Возващает итератор на минимальный по порядку right.
  right_iterator begin_right() const {
    return right_iterator(r.begin());
  }
  // Возващает итератор на следующий за последним по порядку right.
  right_iterator end_right() const {
    return right_iterator(r.end());
  }

  // Проверка на пустоту
  bool empty() const {
    return size_ == 0;
  }

  // Возвращает размер бимапы (кол-во пар)
  std::size_t size() const {
    return size_;
  }

  // операторы сравнения
  friend bool operator==(bimap const& a, bimap const& b) {
    if (a.size() != b.size()) {
      return false;
    }
    auto a_it = a.begin_left(), b_it = b.begin_left();
    while (a_it != a.end_left()) {
      auto a_right_it = a_it.flip(), b_right_it = b_it.flip();
      if (*a_it != *b_it || *a_right_it != *b_right_it) {
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
