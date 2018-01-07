#pragma once   // TODO: use header guards.
#include <cassert>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <type_traits>

#include "external/callable/callable.hpp"

namespace rapidfuzz {

class RawQueue {
public:
  RawQueue(const uint8_t *Data, size_t Size)
      : data_(Data), size_(Size / sizeof(uint8_t)), index_(0){};

  /** Takes one element of type T from queue.
  *
  * Throws if empty.
  *
  * NOTE: Little endianess means that uint8_t {1, 0, 0, 0} == int {1}.
  */
  template <typename T> T pop() {
    assert(data_);
    std::lock_guard<std::mutex> lock(data_mutex_);

    static_assert(sizeof(uint8_t), "Index is wrong");

    constexpr size_t sizeof_T = sizeof(T) / sizeof(uint8_t);
    if (!has_size(sizeof_T)) {
      // TODO: Thou shall not use plain runtime_error!
      throw  std::runtime_error("Queue depleted!");
    }
    assert(index_ < size_);


    // std::cout << "S: " << size_ << ", I: " << index_ << std::endl;
    
    //T val;
    //std::memcpy(reinterpret_cast<T*>(&val), &(data_[index_]), sizeof(T));
     // std::memcpy(                       &val , &(data_[index_]), sizeof(T));

    const T val = *reinterpret_cast<const T *>(&(data_[index_]));
    index_ += sizeof_T;

    return val;
  }

protected:
  bool has_size(size_t requested) {
  return index_ + requested < size_; // TODO: I think off by one.
  }

private:

  const uint8_t *data_; ///< Warning: Ownership resides outside of RawQueue.
  std::mutex data_mutex_;
  const size_t size_;
  size_t index_;
};

template <> std::string RawQueue::pop<std::string>() {
  std::lock_guard<std::mutex> lock(data_mutex_);
  assert(data_);
  assert(index_ < size_);
  size_t string_length = static_cast<size_t>(data_[index_]); // Up-to 255 ought to be enough.
  const size_t new_index =
      index_ + string_length + 1; // +1 b/c first value is length of string.

  if (new_index > size_) {
    // TODO: Thou shall not use plain runtime_error!
    std::runtime_error("Queue depleted!");
  }

  const std::string val(reinterpret_cast<const char *>(&(data_[index_ + 1])),
                        string_length);
  index_ = new_index;
  return val;
};


// TODO: read on foldables as done by RapidCheck.
template <> std::vector<int> RawQueue::pop<std::vector<int>>() {
	size_t vec_length;
	{ std::lock_guard<std::mutex> lock(data_mutex_);
   if (!has_size(1)) throw std::runtime_error("Queue depleted"); 
   vec_length = static_cast<size_t>(data_[index_]);
  
  // Optional, I'd say.
  const size_t new_index = index_ + vec_length + 1;
  // TODO: same as std::string implementation
  if (new_index > size_) throw std::runtime_error("Queue depleted!");
}  // end lock_guard


  std::vector<int> val;
  val.reserve(vec_length);
  for (int i = 0; i < vec_length; i++) {
    val.push_back(pop<int>());
  }

  return val;
}

template <typename F, typename... Args>
decltype(auto) call(RawQueue *Data, F &&f, Args &&... args) {
  if constexpr (std::is_invocable<F, Args...>::value) {
    return std::invoke(f, args...);
  } else {
    assert(Data);
    constexpr size_t n_already = sizeof...(args);
    constexpr size_t n_needed = callable_traits<F>::argc;
    static_assert(n_needed >= n_already, "Too many arguments!");
    constexpr size_t pos = n_already;
    typedef typename callable_traits<F>::template argument_type<pos> T;
    auto val = Data->pop<T>();
    return call(Data, f, args..., val);
  }
}

} // namespace rapidfuzz

