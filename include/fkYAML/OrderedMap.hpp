/**
 *   __ _  __     __      __  __ _
 *  / _| | \ \   / //\   |  \/  | |
 * | |_| | _\ \_/ //  \  | \  / | |
 * |  _| |/ /\   // /\ \ | |\/| | |
 * | | |   <  | |/ ____ \| |  | | |____
 * |_| |_|\_\ |_/_/    \_\_|  |_|______|
 *
 * @file Deserializer.hpp
 * @brief Implementation of a minimal map-like container which preserves insertion order.
 * @version 0.0.0
 *
 * Copyright (c) 2023 fktn
 * Distributed under the MIT License (https://opensource.org/licenses/MIT)
 */

#ifndef FK_YAML_ORDERED_MAP_HPP_
#define FK_YAML_ORDERED_MAP_HPP_

#include <functional>
#include <initializer_list>
#include <memory>
#include <utility>
#include <vector>

#include "fkYAML/VersioningMacros.hpp"
#include "fkYAML/Exception.hpp"
#include "fkYAML/TypeTraits.hpp"

/**
 * @namespace fkyaml
 * @brief namespace for fkYAML library.
 */
FK_YAML_NAMESPACE_BEGIN

/**
 * @brief A minimal map-like container which preserves insertion order.
 *
 * @tparam Key A type for keys.
 * @tparam Value A type for values.
 * @tparam IgnoredCompare A placeholder for key comparison. This will be ignored.
 * @tparam Allocator A class for allocators.
 */
template <
    typename Key, typename Value, typename IgnoredCompare = std::less<Key>,
    typename Allocator = std::allocator<std::pair<const Key, Value>>>
class OrderedMap : public std::vector<std::pair<const Key, Value>, Allocator>
{
public:
    /** A type for keys. */
    using key_type = Key;
    /** A type for values. */
    using mapped_type = Value;
    /** A type for internal key-value containers */
    using Container = std::vector<std::pair<const Key, Value>, Allocator>;
    /** A type for key-value pairs */
    using value_type = typename Container::value_type;
    /** A type for non-const iterators */
    using iterator = typename Container::iterator;
    /** A type for const iterators. */
    using const_iterator = typename Container::const_iterator;
    /** A type for size parameters used in this class. */
    using size_type = typename Container::size_type;
    /** A type for comparison between keys. */
    using key_compare = std::equal_to<Key>;

public:
    /**
     * @brief Construct a new OrderedMap object.
     */
    OrderedMap() noexcept(noexcept(Container()))
        : Container(),
          m_compare()
    {
    }

    /**
     * @brief Construct a new OrderedMap object with an initializer list.
     *
     * @param init An initializer list to construct the inner container object.
     */
    OrderedMap(std::initializer_list<value_type> init)
        : Container {init},
          m_compare()
    {
    }

public:
    /**
     * @brief A subscript operator for OrderedMap objects.
     *
     * @tparam KeyType A type for the input key.
     * @param key A key to the target value.
     * @return mapped_type& Reference to a mapped_type object associated with the given key.
     */
    template <typename KeyType, fkyaml::enable_if_t<IsUsableAsKeyType<key_compare, key_type, KeyType>::value, int> = 0>
    mapped_type& operator[](KeyType&& key) noexcept
    {
        return emplace(std::forward<KeyType>(key), mapped_type()).first->second;
    }

public:
    /**
     * @brief Emplace a new key-value pair if the new key does not exist.
     *
     * @param key A key to be emplaced to this OrderedMap object.
     * @param value A value to be emplaced to this OrderedMap object.
     * @return std::pair<iterator, bool> A result of emplacement of the new key-value pair.
     */
    template <typename KeyType, fkyaml::enable_if_t<IsUsableAsKeyType<key_compare, key_type, KeyType>::value, int> = 0>
    // NOLINTNEXTLINE(readability-identifier-naming)
    std::pair<iterator, bool> emplace(KeyType&& key, const mapped_type& value) noexcept
    {
        for (auto itr = this->begin(); itr != this->end(); ++itr)
        {
            if (m_compare(itr->first, key))
            {
                return {itr, false};
            }
        }
        this->emplace_back(key, value);
        return {std::prev(this->end()), true};
    }

    /**
     * @brief Find a value associated to the given key. Throws an exception if the search fails.
     *
     * @tparam KeyType A type for the input key.
     * @param key A key to find a value with.
     * @return mapped_type& The value associated to the given key.
     */
    template <typename KeyType, fkyaml::enable_if_t<IsUsableAsKeyType<key_compare, key_type, KeyType>::value, int> = 0>
    // NOLINTNEXTLINE(readability-identifier-naming)
    mapped_type& at(KeyType&& key)
    {
        for (auto itr = this->begin(); itr != this->end(); ++itr)
        {
            if (m_compare(itr->first, key))
            {
                return itr->second;
            }
        }
        throw Exception("key not found.");
    }

    /**
     * @brief Find a value associated to the given key. Throws an exception if the search fails.
     *
     * @tparam KeyType A type for the input key.
     * @param key A key to find a value with.
     * @return const mapped_type& The value associated to the given key.
     */
    template <typename KeyType, fkyaml::enable_if_t<IsUsableAsKeyType<key_compare, key_type, KeyType>::value, int> = 0>
    // NOLINTNEXTLINE(readability-identifier-naming)
    const mapped_type& at(KeyType&& key) const
    {
        for (auto itr = this->begin(); itr != this->end(); ++itr)
        {
            if (m_compare(itr->first, key))
            {
                return itr->second;
            }
        }
        throw Exception("key not found.");
    }

    /**
     * @brief Find a value with the given key.
     *
     * @tparam KeyType A type for the input key.
     * @param key A key to find a value with.
     * @return iterator The iterator for the found value, or the result of end().
     */
    template <typename KeyType, fkyaml::enable_if_t<IsUsableAsKeyType<key_compare, key_type, KeyType>::value, int> = 0>
    // NOLINTNEXTLINE(readability-identifier-naming)
    iterator find(KeyType&& key) noexcept
    {
        for (auto itr = this->begin(); itr != this->end(); ++itr)
        {
            if (m_compare(itr->first, key))
            {
                return itr;
            }
        }
        return this->end();
    }

    /**
     * @brief Find a value with the given key.
     *
     * @tparam KeyType A type for the input key.
     * @param key A key to find a value with.
     * @return const_iterator The constant iterator for the found value, or the result of end().
     */
    template <typename KeyType, fkyaml::enable_if_t<IsUsableAsKeyType<key_compare, key_type, KeyType>::value, int> = 0>
    // NOLINTNEXTLINE(readability-identifier-naming)
    const_iterator find(KeyType&& key) const noexcept
    {
        for (auto itr = this->begin(); itr != this->end(); ++itr)
        {
            if (m_compare(itr->first, key))
            {
                return itr;
            }
        }
        return this->end();
    }

private:
    key_compare m_compare; /** The object for comparing keys. */
};

FK_YAML_NAMESPACE_END

#endif /* FK_YAML_ORDERED_MAP_HPP_ */
