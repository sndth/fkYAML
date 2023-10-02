#ifndef FK_YAML_NODE_TYPE_TRAITS_HPP_
#define FK_YAML_NODE_TYPE_TRAITS_HPP_

#include <type_traits>

#include "fkYAML/VersioningMacros.hpp"

/**
 * @namespace fkyaml
 * @brief namespace for fkYAML library.
 */
FK_YAML_NAMESPACE_BEGIN

// forward declaration for fkyaml::BasicNode<...>
template <
    template <typename, typename...> class SequenceType, template <typename, typename, typename...> class MappingType,
    typename BooleanType, typename IntegerType, typename FloatNumberType, typename StringType>
class BasicNode;

/**
 * @struct IsBasicNode
 * @brief A struct to check the template parameter class is a kind of BasicNode template class.
 *
 * @tparam T A class to be checked if it's a kind of BasicNode template class.
 */
template <typename T>
struct IsBasicNode : std::false_type
{
};

/**
 * @brief A partial specialization of IsBasicNode for BasicNode template class.
 *
 * @tparam SequenceType A type for sequence node value containers.
 * @tparam MappingType A type for mapping node value containers.
 * @tparam BooleanType A type for boolean node values.
 * @tparam IntegerType A type for integer node values.
 * @tparam FloatNumberType A type for float number node values.
 * @tparam StringType A type for string node values.
 */
template <
    template <typename, typename...> class SequenceType, template <typename, typename, typename...> class MappingType,
    typename BooleanType, typename IntegerType, typename FloatNumberType, typename StringType>
struct IsBasicNode<BasicNode<SequenceType, MappingType, BooleanType, IntegerType, FloatNumberType, StringType>>
    : std::true_type
{
};

/**
 * @brief A helper structure for void_t.
 *
 * @tparam Types Any types to be transformed to void type.
 */
template <typename... Types>
struct MakeVoid
{
    using type = void;
};

/**
 * @brief A simple implementation to use std::void_t even with C++11-14.
 * @note std::void_t is available since C++17.
 *
 * @tparam Types Any types to be transformed to void type.
 */
template <typename... Types>
using void_t = typename MakeVoid<Types...>::type;

/**
 * @brief An alias template for std::enable_if::type with C++11.
 * @note std::enable_if_t is available since C++14.
 *
 * @tparam Condition A condition tested at compile time.
 * @tparam T The type defined only if Condition is true.
 */
template <bool Condition, typename T>
using enable_if_t = typename std::enable_if<Condition, T>::type;

/**
 * @brief Type trait to check if T and U are comparable types.
 *
 * @tparam Comparator An object type to compare T and U objects.
 * @tparam T A type for comparison.
 * @tparam U The other type for comparison.
 * @tparam typename Placeholder for determining T and U are comparable types.
 */
template <typename Comparator, typename T, typename U, typename = void>
struct IsComparable : std::false_type
{
};

/**
 * @brief A partial specialization of IsComparable if T and U are comparable types.
 *
 * @tparam Comparator An object type to compare T and U objects.
 * @tparam T A type for comparison.
 * @tparam U Ther other type for comparison.
 */
template <typename Comparator, typename T, typename U>
struct IsComparable<
    Comparator, T, U,
    void_t<
        decltype(std::declval<Comparator>()(std::declval<T>(), std::declval<U>())),
        decltype(std::declval<Comparator>()(std::declval<U>(), std::declval<T>()))>> : std::true_type
{
};

/**
 * @brief Type trait to check if KeyType can be used as key type.
 *
 * @tparam Comparator An object type to compare T and U objects.
 * @tparam ObjectKeyType The original key type.
 * @tparam KeyType A type to be used as key type.
 */
template <typename Comparator, typename ObjectKeyType, typename KeyType>
using IsUsableAsKeyType = typename std::conditional<
    IsComparable<Comparator, ObjectKeyType, KeyType>::value, std::true_type, std::false_type>::type;

FK_YAML_NAMESPACE_END

#endif /* FK_YAML_NODE_TYPE_TRAITS_HPP_ */
