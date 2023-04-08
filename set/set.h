#include <cassert>  // assert
#include <iterator> // std::reverse_iterator
#include <utility>  // std::pair, std::swap

struct base_node {
  base_node() : left(nullptr), right(nullptr), parent(nullptr) {}

  base_node* left;
  base_node* right;
  base_node* parent;
};

template <typename T>
struct node : base_node {
  node(T const& init_val) : val(init_val){};

  T val;
};

template <typename T>
struct set {
  struct iterator {
    friend set;
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T const;
    using pointer = T const*;
    using reference = T const&;

    iterator() = default;
    iterator(iterator const& other) = default;

    reference operator*() const {
      return (static_cast<node<T>*>(item))->val;
    } // O(1) nothrow

    pointer operator->() const {
      return &((static_cast<node<T>*>(item))->val);
    } // O(1) nothrow

    iterator& operator++() & {
      if (item->right != nullptr) {
        item = minimum(item->right);
        return (*this);
      }
      base_node* pr = item->parent;
      while (pr != nullptr && item == pr->right) {
        item = pr;
        pr = pr->parent;
      }
      item = pr;
      return (*this);
    } //      nothrow

    iterator operator++(int) & {
      iterator tmp(*this);
      ++(*this);
      return tmp;
    } //      nothrow

    iterator& operator--() & {
      if (item->left != nullptr) {
        item =  maximum(item->left);
        return (*this);
      }
      base_node* pr = item->parent;
      while (pr != nullptr && item == pr->left) {
        item = pr;
        pr = pr->parent;
      }
      item = pr;
      return (*this);
    } //      nothrow

    iterator operator--(int) & {
      iterator tmp(*this);
      --(*this);
      return tmp;
    } //      nothrow

    friend bool operator==(iterator first, iterator second) {
      return first.item == second.item;
    }

    friend bool operator!=(iterator first, iterator second) {
      return first.item != second.item;
    }

    friend void swap(iterator first, iterator second) {
      using std::swap;
      swap(first.item, second.item);
    }

  private:
    iterator(base_node* other) : item(other) {}
    iterator(const base_node* other) : item(const_cast<base_node*>(other)) {}

    base_node* get_node() {
      return item;
    }

    static base_node* minimum(base_node* ver) {
      while (ver->left != nullptr) {
        ver = ver->left;
      }
      return ver;
    }

    static base_node* maximum(base_node* ver) {
      while (ver->right != nullptr) {
        ver = ver->right;
      }
      return ver;
    }

    base_node* item;
  };

  using const_iterator = iterator;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  set() = default; // O(1) nothrow
  set(set const& other) : set() {
    for (iterator it = other.begin(); it != other.end(); it++) {
      insert(*it);
    }
  } // O(n) strong

  set& operator=(set const& other) {
    if (&other != this) {
      set(other).swap(*this);
    }
    return *this;
  } // O(n) strong

  ~set() {
    clear();
  } // O(n) nothrow

  void clear() {
    rec_clear(fake_.left);
    fake_.left = nullptr;
  } // O(n) nothrow

  bool empty() {
    return (fake_.left == nullptr);
  } // O(1) nothrow

  const_iterator begin() const {
    if (fake_.left == nullptr)
      return end();
    base_node* cur = iterator::minimum(fake_.left);
    return const_iterator(cur);
  } //      nothrow

  const_iterator end() const {
    return const_iterator(&fake_);
  } //      nothrow

  const_reverse_iterator rbegin() const {
    return std::reverse_iterator(end());
  } //      nothrow
  const_reverse_iterator rend() const {
    return std::reverse_iterator(begin());
  } //      nothrow

  std::pair<iterator, bool> insert(T const& new_val) {
    auto* new_ver = new node(new_val);
    base_node* cur = &fake_;
    while (cur != nullptr) {
      try {
        if (compare(new_ver, cur)) {
          if (cur->right != nullptr) {
            cur = cur->right;
          } else {
            new_ver->parent = cur;
            cur->right = new_ver;
            break;
          }
        } else if (compare(cur, new_ver)) {
          if (cur->left != nullptr) {
            cur = cur->left;
          } else {
            new_ver->parent = cur;
            cur->left = new_ver;
            break;
          }
        } else {
          delete static_cast<node<T>*>(new_ver);
          return std::make_pair(iterator(cur), false);
        }
      } catch (...) {
        delete static_cast<node<T>*>(new_ver);
        throw;
      }
    }
    return std::make_pair(iterator(new_ver), true);
  } // O(h) strong

  iterator erase(iterator del_ver) {
    base_node* cur = del_ver.get_node();

    base_node* pr = cur->parent;
    if (cur->left == nullptr && cur->right == nullptr) {
      delete_item(cur, nullptr);
    } else if (cur->left == nullptr || cur->right == nullptr) {
      if (cur->left == nullptr) {
        cur->right->parent = pr;
        delete_item(cur, cur->right);
      } else {
        cur->left->parent = pr;
        delete_item(cur, cur->left);
      }
    } else {
      base_node* successor = (++del_ver).get_node();
      if (successor->parent->left == successor) {
        successor->parent->left = successor->right;
      } else {
        successor->parent->right = successor->right;
      }
      if (successor->right != nullptr) {
        successor->right->parent = successor->parent;
      }
      successor->left = cur->left;
      successor->right = cur->right;
      successor->parent = cur->parent;

      if (successor->left) {
        successor->left->parent = successor;
      }
      if (successor->right) {
        successor->right->parent = successor;
      }
      if (successor->parent->left == cur) {
        successor->parent->left = successor;
      } else {
        successor->parent->right = successor;
      }

      std::swap(successor, cur);
      delete static_cast<node<T>*>(successor);
    }
    return iterator(cur);
  } // O(h) nothrow

  const_iterator find(T const& find_item) const {
    const_iterator lb = lower_bound(find_item);
    if (lb != end() && get_value(lb.get_node()) == find_item) {
      return lb;
    }
    return end();
  } // O(h) strong

  const_iterator lower_bound(T const& find_item) const {
    base_node* answer = nullptr;
    base_node* cur = fake_.left;
    while (cur != nullptr) {
      if (get_value(cur) >= find_item) {
        answer = cur;
        cur = cur->left;
      } else {
        cur = cur->right;
      }
    }
    if (!answer) return end();
    return const_iterator(answer);
  } // O(h) strong

  const_iterator upper_bound(T const& find_item) const {
    const_iterator lb = lower_bound(find_item);
    if (lb != end() && get_value(lb.get_node()) == find_item) {
      lb++;
    }
    return lb;
  } // O(h) strong

  friend void swap(set& first, set& second) {
    first.swap(second);
  }

  void swap(set& other) {
    using std::swap;
    swap(fake_.left, other.fake_.left);
    if (!empty()) {
      fake_.left->parent = &fake_;
    }
    if (!other.empty()) {
      other.fake_.left->parent = &other.fake_;
    }
  } // O(1) nothrow

private:
  void delete_item(base_node* del_item, base_node* change_item) {
    base_node* pr = del_item->parent;
    if (pr->left == del_item) {
      pr->left = change_item;
    } else {
      pr->right = change_item;
    }
    delete static_cast<node<T>*>(del_item);
  }

  bool compare(base_node* first, base_node* second) const { // true - first more second
    if (first == &fake_) {
      return true;
    } else if (second == &fake_) {
      return false;
    }
    return (get_value(first) > get_value(second));
  }

  const T& get_value(base_node* cur_node) const {
    return (static_cast<node<T>*>(cur_node))->val;
  }

  void rec_clear(base_node* cur) {
    if (cur == nullptr) return;
    rec_clear(cur->left);
    rec_clear(cur->right);
    delete static_cast<node<T>*>(cur);
  }

  base_node fake_;
};