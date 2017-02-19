#ifndef SBT_MAP_H
#define SBT_MAP_H

#include <algorithm>
#include <vector>

namespace sbt {

// Map implementation on top of a sorted std::vector.
template <typename K, typename V>
class Map
{
public:
  typedef K Key;
  typedef V Value;

  class Item
  {
  public:
    Item(const Key &Key, const Value &Val) :
      IKey(Key),
      IVal(Val)
    {}

    Item(Key &&Key, Value &&Val) :
      IKey(std::move(Key)),
      IVal(std::move(Val))
    {}

    Item(const Item &) = delete;
    Item(Item &&) = default;

    Item &operator=(const Item &) = delete;
    Item &operator=(Item &&) = default;

    operator const Value &() const
    {
      return IVal;
    }

    Key IKey;
    Value IVal;
  };

  typedef std::vector<Item> Vec;
  typedef typename Vec::const_iterator CIter;
  typedef typename Vec::iterator Iter;

  Map() = default;

  Map(Vec &&VV) :
    Data(std::move(VV))
  {
    sort();
  }

  Value *operator[](const Key &Key)
  {
    return lookupVal(this, Key);
  }

  const Value *operator[](const Key &Key) const
  {
    return lookupVal(this, Key);
  }

  // insert/update
  void operator()(const Key &KK, Value &&Val)
  {
    Value *DV = lookupVal(this, KK);
    if (DV) {
      *DV = std::move(Val);
    } else {
      auto I = Data.emplace(Data.end(), Item(KK, std::move(Val)));
      std::inplace_merge(Data.begin(), I, Data.end(),
        [](const Item &I1, const Item &I2){ return I1.IKey < I2.IKey; });
    }
  }

  CIter begin() const
  {
    return Data.begin();
  }

  CIter end() const
  {
    return Data.end();
  }

  Iter begin()
  {
    return Data.begin();
  }

  Iter end()
  {
    return Data.end();
  }

private:
  Vec Data;

  // methods

  void sort()
  {
    std::sort(Data.begin(), Data.end());
  }

  // Implementation of const and non-const versions of operator[]
  template <typename T>
  static auto lookup(T thiz, const Key &KK) -> decltype(&thiz->Data[0])
  {
    auto &Data = thiz->Data;
    if (Data.empty())
      return nullptr;
    const Item &Dummy = Data.front();

    auto I = std::lower_bound(Data.begin(), Data.end(), Dummy,
      [&KK](const Item &II, const Item &){ return II.IKey < KK; });
    if (I == Data.end())
      return nullptr;
    else
      return I->IKey == KK? &*I : nullptr;
  }

  template <typename T>
  static auto lookupVal(T thiz, const Key &Key) -> decltype(&thiz->Data[0].IVal)
  {
    auto I = lookup(thiz, Key);
    if (I)
      return &I->IVal;
    else
      return nullptr;
  }
};

} // sbt

#endif
