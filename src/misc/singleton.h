#pragma once

namespace misc
{
  template <typename T>
  class Singleton
  {
    // Make it non-copyable.
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

  protected:
    Singleton() = default;

  public:
    /// Access to the unique instance.
    static T& instance()
    {
      static T instance;
      return instance;
    }
  };
} // namespace misc
