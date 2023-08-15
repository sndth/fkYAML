/**
 * @file Iterator.hpp
 * @brief Implementation of the YAML node iterator.
 *
 * Copyright (c) 2023 fktn
 * Distributed under the MIT License (https://opensource.org/licenses/MIT)
 */

#ifndef FK_YAML_ITERATOR_HPP_
#define FK_YAML_ITERATOR_HPP_

#include <cstddef>
#include <iterator>

#include "fkYAML/Exception.hpp"

/**
 * @namespace fkyaml
 * @brief namespace for fkYAML library.
 */
namespace fkyaml
{

/**
 * @struct SequenceIteratorTag
 * @brief A tag which tells Iterator will contain sequence value iterator.
 */
struct SequenceIteratorTag
{
};

/**
 * @struct MappingIteratorTag
 * @brief A tag which tells Iterator will contain mapping value iterator.
 */
struct MappingIteratorTag
{
};

/**
 * @struct IteratorTraits
 * @brief The template definitions of type informations used in @ref Iterator class
 *
 * @tparam ValueType The type of iterated elements.
 */
template <typename ValueType>
struct IteratorTraits
{
    /** A type of iterated elements. */
    using value_type = ValueType;
    /** A type to represent difference between iterators. */
    using difference_type = std::ptrdiff_t;
    /** A type to represent iterator sizes. */
    using size_type = std::size_t;
    /** A type of an element pointer. */
    using pointer = value_type*;
    /** A type of reference to an element. */
    using reference = value_type&;
};

/**
 * @brief A specialization of @ref IteratorTraits for constant value types.
 *
 * @tparam ValueType The type of iterated elements.
 */
template <typename ValueType>
struct IteratorTraits<const ValueType>
{
    /** A type of iterated elements. */
    using value_type = ValueType;
    /** A type to represent difference between iterators. */
    using difference_type = std::ptrdiff_t;
    /** A type to represent iterator sizes. */
    using size_type = std::size_t;
    /** A type of a constant element pointer. */
    using pointer = const value_type*;
    /** A type of constant reference to an element. */
    using reference = const value_type&;
};

/**
 * @class Iterator
 * @brief A class which holds iterators either of sequence or mapping type
 *
 * @tparam ValueType The type of iterated elements.
 */
template <typename ValueType>
class Iterator
{
public:
    /** A type for iterator traits of instantiated @Iterator template class. */
    using ItrTraitsType = IteratorTraits<ValueType>;

    /** A type for iterator category tag. */
    using iterator_category = std::bidirectional_iterator_tag;
    /** A type of iterated element. */
    using value_type = typename ItrTraitsType::value_type;
    /** A type to represent differences between iterators. */
    using difference_type = typename ItrTraitsType::difference_type;
    /** A type to represent container sizes. */
    using size_type = typename ItrTraitsType::size_type;
    /** A type of an element pointer. */
    using pointer = typename ItrTraitsType::pointer;
    /** A type of reference to an element. */
    using reference = typename ItrTraitsType::reference;

private:
    /**
     * @enum InnerIteratorType
     * @brief Definitions of iterator types for iterators internally held.
     */
    enum class InnerIteratorType
    {
        SEQUENCE, //!< sequence iterator type.
        MAPPING,  //!< mapping iterator type.
    };

    /** A type of non-const version of iterated elements. */
    using NonConstValueType = typename std::remove_const<ValueType>::type;

    /**
     * @struct IteratorHolder
     * @brief The actual storage for iterators internally held in @ref Iterator.
     */
    struct IteratorHolder
    {
        /** A sequence iterator object. */
        typename NonConstValueType::sequence_type::iterator sequence_iterator {};
        /** A mapping iterator object. */
        typename NonConstValueType::mapping_type::iterator mapping_iterator {};
    };

public:
    /**
     * @brief Construct a new Iterator object with sequence iterator object.
     *
     * @param[in] itr An sequence iterator object.
     */
    Iterator(SequenceIteratorTag /* unused */, const typename ValueType::sequence_type::iterator& itr) noexcept
        : m_inner_iterator_type(InnerIteratorType::SEQUENCE)
    {
        m_iterator_holder.sequence_iterator = itr;
    }

    /**
     * @brief Construct a new Iterator object with mapping iterator object.
     *
     * @param[in] itr An mapping iterator object.
     */
    Iterator(MappingIteratorTag /* unused */, const typename ValueType::mapping_type::iterator& itr) noexcept
        : m_inner_iterator_type(InnerIteratorType::MAPPING)
    {
        m_iterator_holder.mapping_iterator = itr;
    }

    /**
     * @brief Copy constructor of the Iterator class.
     *
     * @param other An Iterator object to be copied with.
     */
    Iterator(const Iterator& other) noexcept
        : m_inner_iterator_type(other.m_inner_iterator_type)
    {
        switch (m_inner_iterator_type)
        {
        case InnerIteratorType::SEQUENCE:
            m_iterator_holder.sequence_iterator = other.m_iterator_holder.sequence_iterator;
            break;
        case InnerIteratorType::MAPPING:
            m_iterator_holder.mapping_iterator = other.m_iterator_holder.mapping_iterator;
            break;
        }
    }

    /**
     * @brief Move constructor of the Iterator class.
     *
     * @param other An Iterator object to be moved from.
     */
    Iterator(Iterator&& other) noexcept
        : m_inner_iterator_type(other.m_inner_iterator_type)
    {
        switch (m_inner_iterator_type)
        {
        case InnerIteratorType::SEQUENCE:
            m_iterator_holder.sequence_iterator = std::move(other.m_iterator_holder.sequence_iterator);
            break;
        case InnerIteratorType::MAPPING:
            m_iterator_holder.mapping_iterator = std::move(other.m_iterator_holder.mapping_iterator);
            break;
        }
    }

    ~Iterator() = default;

public:
    /**
     * @brief A copy assignment operator of the Iterator class.
     *
     * @param rhs An Iterator object to be copied with.
     * @return Iterator& Reference to this Iterator object.
     */
    Iterator& operator=(const Iterator& rhs) noexcept // NOLINT(cert-oop54-cpp)
    {
        if (&rhs == this)
        {
            return *this;
        }

        m_inner_iterator_type = rhs.m_inner_iterator_type;
        switch (m_inner_iterator_type)
        {
        case InnerIteratorType::SEQUENCE:
            m_iterator_holder.sequence_iterator = rhs.m_iterator_holder.sequence_iterator;
            break;
        case InnerIteratorType::MAPPING:
            m_iterator_holder.mapping_iterator = rhs.m_iterator_holder.mapping_iterator;
            break;
        }

        return *this;
    }

    /**
     * @brief A move assignment operator of the Iterator class.
     *
     * @param rhs An Iterator object to be moved from.
     * @return Iterator& Reference to this Iterator object.
     */
    Iterator& operator=(Iterator&& rhs) noexcept
    {
        if (&rhs == this)
        {
            return *this;
        }

        m_inner_iterator_type = rhs.m_inner_iterator_type;
        switch (m_inner_iterator_type)
        {
        case InnerIteratorType::SEQUENCE:
            m_iterator_holder.sequence_iterator = std::move(rhs.m_iterator_holder.sequence_iterator);
            break;
        case InnerIteratorType::MAPPING:
            m_iterator_holder.mapping_iterator = std::move(rhs.m_iterator_holder.mapping_iterator);
            break;
        }

        return *this;
    }

    /**
     * @brief An arror operator of the Iterator class.
     *
     * @return pointer A pointer to the Node object internally referenced by the actual iterator object.
     */
    pointer operator->() noexcept
    {
        switch (m_inner_iterator_type)
        {
        case InnerIteratorType::SEQUENCE:
            return &(*(m_iterator_holder.sequence_iterator));
        case InnerIteratorType::MAPPING:
            return &(m_iterator_holder.mapping_iterator->second);
        default:
            return nullptr;
        }
    }

    /**
     * @brief A dereference operator of the Iterator class.
     *
     * @return reference Reference to the Node object internally referenced by the actual iterator object.
     */
    reference operator*() noexcept
    {
        switch (m_inner_iterator_type)
        {
        case InnerIteratorType::SEQUENCE:
            return *(m_iterator_holder.sequence_iterator);
        case InnerIteratorType::MAPPING:
            return *(m_iterator_holder.mapping_iterator).second;
        }
    }

    /**
     * @brief A compound assignment operator by sum of the Iterator class.
     *
     * @param i The difference from this Iterator object with which it moves forward.
     * @return Iterator& Reference to this Iterator object.
     */
    Iterator& operator+=(difference_type i)
    {
        switch (m_inner_iterator_type)
        {
        case InnerIteratorType::SEQUENCE:
            std::advance(m_iterator_holder.sequence_iterator, i);
            break;
        case InnerIteratorType::MAPPING:
            throw Exception("Cannot use offsets with operators of the mapping container type.");
        }
        return *this;
    }

    /**
     * @brief A plus operator of the Iterator class.
     *
     * @param i The difference from this Iterator object.
     * @return Iterator An Iterator object which has been added @a i.
     */
    Iterator operator+(difference_type i) const
    {
        auto result = *this;
        result += i;
        return result;
    }

    /**
     * @brief An pre-increment operator of the Iterator class.
     *
     * @return Iterator& Reference to this Iterator object.
     */
    Iterator& operator++() noexcept
    {
        switch (m_inner_iterator_type)
        {
        case InnerIteratorType::SEQUENCE:
            std::advance(m_iterator_holder.sequence_iterator, 1);
            break;
        case InnerIteratorType::MAPPING:
            std::advance(m_iterator_holder.mapping_iterator, 1);
            break;
        }
        return *this;
    }

    /**
     * @brief A post-increment opretor of the Iterator class.
     *
     * @return Iterator An Iterator object which has been incremented.
     */
    Iterator operator++(int) & noexcept // NOLINT(cert-dcl21-cpp)
    {
        auto result = *this;
        ++(*this);
        return result;
    }

    /**
     * @brief A compound assignment operator by difference of the Iterator class.
     *
     * @param i The difference from this Iterator object with which it moves backward.
     * @return Iterator& Reference to this Iterator object.
     */
    Iterator& operator-=(difference_type i)
    {
        return operator+=(-i);
    }

    /**
     * @brief A minus operator of the Iterator class.
     *
     * @param i The difference from this Iterator object.
     * @return Iterator An Iterator object from which has been subtracted @ i.
     */
    Iterator operator-(difference_type i)
    {
        auto result = *this;
        result -= i;
        return result;
    }

    /**
     * @brief A pre-decrement operator of the Iterator class.
     *
     * @return Iterator& Reference to this Iterator object.
     */
    Iterator& operator--() noexcept
    {
        switch (m_inner_iterator_type)
        {
        case InnerIteratorType::SEQUENCE:
            std::advance(m_iterator_holder.sequence_iterator, -1);
            break;
        case InnerIteratorType::MAPPING:
            std::advance(m_iterator_holder.mapping_iterator, -1);
            break;
        }
        return *this;
    }

    /**
     * @brief A post-decrement operator of the Iterator class
     *
     * @return Iterator An Iterator object which has been decremented.
     */
    Iterator operator--(int) & noexcept // NOLINT(cert-dcl21-cpp)
    {
        auto result = *this;
        --(*this);
        return result;
    }

    /**
     * @brief An equal-to operator of the Iterator class.
     *
     * @param rhs An Iterator object to be compared with this Iterator object.
     * @return true  This Iterator object is equal to the other.
     * @return false This Iterator object is not equal to the other.
     */
    bool operator==(const Iterator& rhs) const
    {
        if (m_inner_iterator_type != rhs.m_inner_iterator_type)
        {
            throw Exception("Cannot compare iterators of different container types.");
        }

        switch (m_inner_iterator_type)
        {
        case InnerIteratorType::SEQUENCE:
            return (m_iterator_holder.sequence_iterator == rhs.m_iterator_holder.sequence_iterator);
        case InnerIteratorType::MAPPING:
            return (m_iterator_holder.mapping_iterator == rhs.m_iterator_holder.mapping_iterator);
        default:
            return false;
        }
    }

    /**
     * @brief An not-equal-to operator of the Iterator class.
     *
     * @param rhs An Iterator object to be compared with this Iterator object.
     * @return true  This Iterator object is not equal to the other.
     * @return false This Iterator object is equal to the other.
     */
    bool operator!=(const Iterator& rhs) const
    {
        return !operator==(rhs);
    }

    /**
     * @brief A less-than operator of the Iterator class.
     *
     * @param rhs An Iterator object to be compared with this Iterator object.
     * @return true  This Iterator object is less than the other.
     * @return false This Iterator object is not less than the other.
     */
    bool operator<(const Iterator& rhs) const
    {
        if (m_inner_iterator_type != rhs.m_inner_iterator_type)
        {
            throw Exception("Cannot compare iterators of different container types.");
        }

        switch (m_inner_iterator_type)
        {
        case InnerIteratorType::SEQUENCE:
            return (m_iterator_holder.sequence_iterator < rhs.m_iterator_holder.sequence_iterator);
        case InnerIteratorType::MAPPING:
            throw Exception("Cannot compare order of iterators of the mapping container type");
        }
    }

    /**
     * @brief A less-than-or-equal-to operator of the Iterator class.
     *
     * @param rhs An Iterator object to be compared with this Iterator object.
     * @return true  This Iterator object is either less than or equal to the other.
     * @return false This Iterator object is neither less than nor equal to the other.
     */
    bool operator<=(const Iterator& rhs) const
    {
        return !rhs.operator<(*this);
    }

    /**
     * @brief A greater-than operator of the Iterator class.
     *
     * @param rhs An Iterator object to be compared with this Iterator object.
     * @return true  This Iterator object is greater than the other.
     * @return false This Iterator object is not greater than the other.
     */
    bool operator>(const Iterator& rhs) const
    {
        return !operator<=(rhs);
    }

    /**
     * @brief A greater-than-or-equal-to operator of the Iterator class.
     *
     * @param rhs An Iterator object to be compared with this Iterator object.
     * @return true  This Iterator object is either greater than or equal to the other.
     * @return false This Iterator object is neither greater than nor equal to the other.
     */
    bool operator>=(const Iterator& rhs) const
    {
        return !operator<(rhs);
    }

private:
    /** A type of the internally-held iterator. */
    InnerIteratorType m_inner_iterator_type;
    /** A holder of actual iterators. */
    mutable IteratorHolder m_iterator_holder;
};

} // namespace fkyaml

#endif /* FK_YAML_ITERATOR_HPP_ */