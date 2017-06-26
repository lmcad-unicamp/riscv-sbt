#ifndef SBT_MAP_H
#define SBT_MAP_H

#include <algorithm>
#include <type_traits>
#include <vector>

namespace sbt {

// Map implementation on top of a sorted std::vector.
//
// LLVM recommends using a sorted vector instead of a
// std::map for better performance, for several use cases.
// Thus, this class exposes a map-like interface on top of a sorted vector.
template <typename K, typename V>
class Map
{
public:
  typedef K Key;
  typedef V Value;

  class Item
  {
  public:
    // copy key, move value
    Item(const Key& key, Value&& val) :
      key(key),
      val(std::move(val))
    {}

    // move key and value
    Item(Key&& key, Value&& val) :
      key(std::move(key)),
      val(std::move(val))
    {}

    // allow move only
    Item(const Item&) = delete;
    Item(Item&&) = default;

    Item& operator=(const Item&) = delete;
    Item& operator=(Item&&) = default;

    // XXX this is a helper to iterate through the values of the map with
    //     a range for loop, although this is probably not the best way to
    //     accomplish this...
    operator const Value&() const
    {
      return val;
    }

    // data
    Key key;
    Value val;
  };

  typedef std::vector<Item> Vec;
  typedef typename Vec::const_iterator CIter;
  typedef typename Vec::iterator Iter;

  Map() = default;

  // allow move only
  Map(const Map&) = delete;
  Map(Map&&) = default;

  Map& operator=(const Map&) = delete;
  Map& operator=(Map&&) = default;

  // construct from vector
  Map(Vec&& vec) :
    _data(std::move(vec))
  {
    sort();
  }

  // value lookup
  // returns null if not found
  Value* operator[](const Key& key)
  {
    return lookupVal(this, key);
  }

  const Value* operator[](const Key& key) const
  {
    return lookupVal(this, key);
  }

  // insert/update
  void operator()(const Key& key, Value&& val)
  {
    // static asserts to help with compiler errors
    static_assert(std::is_copy_constructible<Key>::value,
      "key is not copy constructible");
    static_assert(std::is_copy_assignable<Key>::value,
      "key is not copy assignable");
    static_assert(std::is_move_constructible<Value>::value,
      "value is not move constructible");
    static_assert(std::is_move_assignable<Value>::value,
      "value is not move assignable");

    Value* dv = lookupVal(this, key);
    if (dv)
      *dv = std::move(val);
    else
      insert(Item(key, std::move(val)));
  }

  void operator()(Key&& key, Value&& val)
  {
    static_assert(std::is_move_constructible<Key>::value,
      "key is not move constructible");
    static_assert(std::is_move_assignable<Key>::value,
      "key is not move assignable");
    static_assert(std::is_move_constructible<Value>::value,
      "value is not move constructible");
    static_assert(std::is_move_assignable<Value>::value,
      "value is not move assignable");

    Value* dv = lookupVal(this, key);
    if (dv)
      *dv = std::move(val);
    else
      insert(Item(std::move(key), std::move(val)));
  }

  // iterators

  CIter begin() const
  {
    return _data.begin();
  }

  CIter end() const
  {
    return _data.end();
  }

  Iter begin()
  {
    return _data.begin();
  }

  Iter end()
  {
    return _data.end();
  }

  // lower_bound

  Iter lower_bound(const Key& key)
  {
    return std::lower_bound(begin(), end(), key,
      [](const Item& i, const Key& key) {
        return i.key < key; });
  }

  CIter lower_bound(const Key& key) const
  {
    return std::lower_bound(begin(), end(), key,
      [](const Item& i, const Key& key) {
        return i.IKey < key; });
  }

  //

  bool empty() const
  {
    return _data.empty();
  }

  size_t size() const
  {
    return _data.size();
  }

  void erase(Iter it)
  {
      _data.erase(it, end());
  }

private:
  Vec _data;

  // methods

  // insert (sorted) a new item at the end
  void insert(Item&& item)
  {
      auto it = _data.emplace(_data.end(), std::move(item));
      std::inplace_merge(_data.begin(), it, _data.end(),
        [](const Item& i1, const Item& i2){ return i1.key < i2.key; });
  }

  // sort
  void sort()
  {
    std::sort(_data.begin(), _data.end());
  }

  // implementation of const and non-const versions of operator[]
  template <typename T>
  static auto lookup(T thiz, const Key& key) -> decltype(&thiz->_data[0])
  {
    auto& data = thiz->_data;
    if (data.empty())
      return nullptr;
    const Item& dummy = data.front();

    auto it = std::lower_bound(data.begin(), data.end(), dummy,
      [&key](const Item& i, const Item&){ return i.key < key; });
    if (it == data.end())
      return nullptr;
    else
      return it->key == key? &*it : nullptr;
  }

  template <typename T>
  static auto lookupVal(T thiz, const Key& key) -> decltype(&thiz->_data[0].val)
  {
    auto it = lookup(thiz, key);
    if (it)
      return &it->val;
    else
      return nullptr;
  }
};

} // sbt

#endif
