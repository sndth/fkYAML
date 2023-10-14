/**
 *  _______   __ __   __  _____   __  __  __
 * |   __| |_/  |  \_/  |/  _  \ /  \/  \|  |     fkYAML: A C++ header-only YAML library
 * |   __|  _  < \_   _/|  ___  |    _   |  |___  version 0.0.1
 * |__|  |_| \__|  |_|  |_|   |_|___||___|______| https://github.com/fktn-k/fkYAML
 *
 * SPDX-FileCopyrightText: 2023 Kensuke Fukutani <fktn.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 *
 * @file
 */

#ifndef FK_YAML_DETAIL_META_TYPE_TRAITS_HPP_
#define FK_YAML_DETAIL_META_TYPE_TRAITS_HPP_

#include <limits>
#include <type_traits>

#include <fkYAML/detail/macros/version_macros.hpp>
#include <fkYAML/detail/meta/detect.hpp>
#include <fkYAML/detail/meta/stl_supplement.hpp>

/**
 * @namespace fkyaml
 * @brief namespace for fkYAML library.
 */
FK_YAML_NAMESPACE_BEGIN

// forward declaration for basic_node<...>
template <
    template <typename, typename...> class SequenceType, template <typename, typename, typename...> class MappingType,
    typename BooleanType, typename IntegerType, typename FloatNumberType, typename StringType,
    template <typename, typename> class Converter>
class basic_node;

/**
 * @namespace detail
 * @brief namespace for internal implementations of fkYAML library.
 */
namespace detail
{

/**
 * @struct is_basic_node
 * @brief A struct to check the template parameter class is a kind of basic_node template class.
 *
 * @tparam T A class to be checked if it's a kind of basic_node template class.
 */
template <typename T>
struct is_basic_node : std::false_type
{
};

/**
 * @brief A partial specialization of is_basic_node for basic_node template class.
 *
 * @tparam SequenceType A type for sequence node value containers.
 * @tparam MappingType A type for mapping node value containers.
 * @tparam BooleanType A type for boolean node values.
 * @tparam IntegerType A type for integer node values.
 * @tparam FloatNumberType A type for float number node values.
 * @tparam StringType A type for string node values.
 * @tparam Converter A type for
 */
template <
    template <typename, typename...> class SequenceType, template <typename, typename, typename...> class MappingType,
    typename BooleanType, typename IntegerType, typename FloatNumberType, typename StringType,
    template <typename, typename> class Converter>
struct is_basic_node<
    basic_node<SequenceType, MappingType, BooleanType, IntegerType, FloatNumberType, StringType, Converter>>
    : std::true_type
{
};

/**
 * @brief Type trait to check if T and U are comparable types.
 *
 * @tparam Comparator An object type to compare T and U objects.
 * @tparam T A type for comparison.
 * @tparam U The other type for comparison.
 * @tparam typename Placeholder for determining T and U are comparable types.
 */
template <typename Comparator, typename T, typename U, typename = void>
struct is_comparable : std::false_type
{
};

/**
 * @brief A partial specialization of is_comparable if T and U are comparable types.
 *
 * @tparam Comparator An object type to compare T and U objects.
 * @tparam T A type for comparison.
 * @tparam U Ther other type for comparison.
 */
template <typename Comparator, typename T, typename U>
struct is_comparable<
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
using is_usable_as_key_type = typename std::conditional<
    is_comparable<Comparator, ObjectKeyType, KeyType>::value, std::true_type, std::false_type>::type;

/**
 * @brief Type trait to check if IntegralType is of non-boolean integral types.
 *
 * @tparam IntegralType A type to be checked.
 * @tparam typename N/A
 */
template <typename IntegralType, typename = void>
struct is_non_bool_integral : std::false_type
{
};

/**
 * @brief A partial specialization of is_non_bool_integral if IntegralType is of non-boolean integral types.
 *
 * @tparam IntegralType A type to be checked.
 */
template <typename IntegralType>
struct is_non_bool_integral<
    IntegralType,
    enable_if_t<conjunction<std::is_integral<IntegralType>, negation<std::is_same<bool, IntegralType>>>::value>>
    : std::true_type
{
};

/**
 * @brief Type traits to check if Types are all signed arithmetic types.
 *
 * @tparam Types Types to check if they are all signed arithmetic types.
 */
template <typename... Types>
using is_all_signed = conjunction<std::is_signed<Types>...>;

/**
 * @brief Type traits to check if Types are all unsigned arithmetic types.
 *
 * @tparam Types Types to check if they are all unsigned arithmetic types.
 */
template <typename... Types>
using is_all_unsigned = conjunction<std::is_unsigned<Types>...>;

/**
 * @brief Type trait implementation to check if TargetIntegerType and CompatibleIntegerType are compatible integer
 * types.
 *
 * @tparam TargetIntegerType A target integer type.
 * @tparam CompatibleIntegerType A compatible integer type.
 * @tparam typename N/A
 */
template <typename TargetIntegerType, typename CompatibleIntegerType, typename = void>
struct is_compatible_integer_type_impl : std::false_type
{
};

/**
 * @brief A partial specialization of is_compatible_integer_type_impl if TargetIntegerType and CompatibleIntegerType are
 * compatible integer types.
 *
 * @tparam TargetIntegerType A target integer type.
 * @tparam CompatibleIntegerType A compatible integer type.
 */
template <typename TargetIntegerType, typename CompatibleIntegerType>
struct is_compatible_integer_type_impl<
    TargetIntegerType, CompatibleIntegerType,
    enable_if_t<conjunction<
        std::is_integral<TargetIntegerType>, is_non_bool_integral<CompatibleIntegerType>,
        std::is_constructible<TargetIntegerType, CompatibleIntegerType>,
        disjunction<
            is_all_signed<TargetIntegerType, CompatibleIntegerType>,
            is_all_unsigned<TargetIntegerType, CompatibleIntegerType>>>::value>> : std::true_type
{
};

/**
 * @brief Type traits to check if TargetIntegerType and CompatibleIntegerType are compatible integer types.
 *
 * @tparam TargetIntegerType A target integer type.
 * @tparam CompatibleIntegerType A compatible integer type.
 */
template <typename TargetIntegerType, typename CompatibleIntegerType>
struct is_compatible_integer_type : is_compatible_integer_type_impl<TargetIntegerType, CompatibleIntegerType>
{
};

/**
 * @brief Type traits to check if T is a complete type.
 *
 * @tparam T A type to be checked if a complete type.
 * @tparam typename N/A
 */
template <typename T, typename = void>
struct is_complete_type : std::false_type
{
};

/**
 * @brief A partial specialization of is_complete_type if T is a complete type.
 *
 * @tparam T
 */
template <typename T>
struct is_complete_type<T, decltype(void(sizeof(T)))> : std::true_type
{
};

/**
 * @brief A type represent from_node function.
 *
 * @tparam T A type which provides from_node function.
 * @tparam Args Argument types passed to from_node function.
 */
template <typename T, typename... Args>
using from_node_function_t = decltype(T::from_node(std::declval<Args>()...));

/**
 * @brief Type traits to check if T is a compatible type for BasicNodeType in terms of from_node function.
 *
 * @tparam BasicNodeType A basic_node template instance type.
 * @tparam T A target type passed to from_node function.
 * @tparam typename N/A
 */
template <typename BasicNodeType, typename T, typename = void>
struct has_from_node : std::false_type
{
};

/**
 * @brief A partial specialization of has_from_node if T is not a basic_node template instance type.
 *
 * @tparam BasicNodeType A basic_node template instance type.
 * @tparam T A target type passed to from_node function.
 */
template <typename BasicNodeType, typename T>
struct has_from_node<BasicNodeType, T, enable_if_t<!is_basic_node<T>::value>>
{
    using converter = typename BasicNodeType::template value_converter_type<T, void>;

    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr bool value =
        is_detected_exact<void, from_node_function_t, converter, const BasicNodeType&, T&>::value;
};

/**
 * @brief A type which represent to_node function.
 *
 * @tparam T A type which provides to_node function.
 * @tparam Args Argument types passed to to_node function.
 */
template <typename T, typename... Args>
using to_node_funcion_t = decltype(T::to_node(std::declval<Args>()...));

/**
 * @brief Type traits to check if T is a compatible type for BasicNodeType in terms of to_node function.
 * @warning Do not pass basic_node type as BasicNodeType to avoid infinite type instantiation.
 *
 * @tparam BasicNodeType A basic_node template instance type.
 * @tparam T A target type passed to to_node function.
 * @tparam typename N/A
 */
template <typename BasicNodeType, typename T, typename = void>
struct has_to_node : std::false_type
{
};

/**
 * @brief A partial specialization of has_to_node if T is not a basic_node template instance type.
 *
 * @tparam BasicNodeType A basic_node template instance type.
 * @tparam T A target type passed to to_node function.
 */
template <typename BasicNodeType, typename T>
struct has_to_node<BasicNodeType, T, enable_if_t<!is_basic_node<T>::value>>
{
    using converter = typename BasicNodeType::template value_converter_type<T, void>;

    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr bool value = is_detected_exact<void, to_node_funcion_t, converter, BasicNodeType&, T>::value;
};

/**
 * @brief A type which represents T::char_type;
 *
 * @tparam T A target type to check if it has char_type;
 */
template <typename T>
using detect_char_type_helper_t = typename T::char_type;

/**
 * @brief Type traits to check if T has char_type as its member.
 *
 * @tparam T A target type.
 * @tparam typename N/A
 */
template <typename T, typename = void>
struct has_char_type : std::false_type
{
};

/**
 * @brief A partial specialization of has_char_type if T has char_type as its member.
 *
 * @tparam T A target type.
 */
template <typename T>
struct has_char_type<T, enable_if_t<is_detected<detect_char_type_helper_t, T>::value>> : std::true_type
{
};

/**
 * @brief A type which represents get_character function.
 *
 * @tparam T A target type.
 */
template <typename T>
using get_character_fn_t = decltype(std::declval<T>().get_character());

/**
 * @brief A type which respresents unget_character function.
 *
 * @tparam T A target type.
 */
template <typename T>
using unget_character_fn_t = decltype(std::declval<T>().unget_character());

/**
 * @brief Type traits to check if InputAdapterType has get_character member function.
 *
 * @tparam InputAdapterType An input adapter type to check if it has get_character function.
 * @tparam typename N/A
 */
template <typename InputAdapterType, typename = void>
struct has_get_character : std::false_type
{
};

/**
 * @brief A partial specialization of has_get_character if InputAdapterType has get_character member function.
 *
 * @tparam InputAdapterType A type of a target input adapter.
 */
template <typename InputAdapterType>
struct has_get_character<InputAdapterType, enable_if_t<is_detected<get_character_fn_t, InputAdapterType>::value>>
    : std::true_type
{
};

/**
 * @brief Type traits to check if InputAdapterType has unget_character member function.
 *
 * @tparam InputAdapterType A type of a target input adapter.
 * @tparam typename N/A
 */
template <typename InputAdapterType, typename = void>
struct has_unget_character : std::false_type
{
};

/**
 * @brief A partial specialization of has_unget_character if InputAdapterType has unget_character member function.
 *
 * @tparam InputAdapterType
 */
template <typename InputAdapterType>
struct has_unget_character<InputAdapterType, enable_if_t<is_detected<unget_character_fn_t, InputAdapterType>::value>>
    : std::true_type
{
};

/**
 * @brief Type traits to check if T is an input adapter type.
 *
 * @tparam T A target type.
 * @tparam typename N/A
 */
template <typename T, typename = void>
struct is_input_adapter : std::false_type
{
};

/**
 * @brief A partial specialization of is_input_adapter if T is an input adapter type.
 *
 * @tparam InputAdapterType
 */
template <typename InputAdapterType>
struct is_input_adapter<
    InputAdapterType, enable_if_t<conjunction<
                          has_char_type<InputAdapterType>, has_get_character<InputAdapterType>,
                          has_unget_character<InputAdapterType>>::value>> : std::true_type
{
};

/**
 * @brief Type traits implementation of is_compatible_type to check if CompatibleType is a compatible type for
 * BasicNodeType.
 *
 * @tparam BasicNodeType A basic_node template instance type.
 * @tparam CompatibleType A target type for compatibility check.
 * @tparam typename N/A
 */
template <typename BasicNodeType, typename CompatibleType, typename = void>
struct is_compatible_type_impl : std::false_type
{
};

/**
 * @brief A partial specialization of is_compatible_type_impl if CompatibleType is a complete type and is compatible for
 * BasicNodeType.
 *
 * @tparam BasicNodeType A basic_node template instance type.
 * @tparam CompatibleType A target type for compatibility check.
 */
template <typename BasicNodeType, typename CompatibleType>
struct is_compatible_type_impl<
    BasicNodeType, CompatibleType,
    enable_if_t<conjunction<is_complete_type<CompatibleType>, has_to_node<BasicNodeType, CompatibleType>>::value>>
    : std::true_type
{
};

/**
 * @brief Type traits to check if CompatibleType is a compatible type for BasicNodeType.
 *
 * @tparam BasicNodeType A basic_node template instance type.
 * @tparam CompatibleType A target type for compatibility check.
 */
template <typename BasicNodeType, typename CompatibleType>
struct is_compatible_type : is_compatible_type_impl<BasicNodeType, CompatibleType>
{
};

/**
 * @brief A utility struct to generate static constant instance.
 *
 * @tparam T A target type for the resulting static constant instance.
 */
template <typename T>
struct static_const
{
    static FK_YAML_INLINE_VAR constexpr T value {}; // NOLINT(readability-identifier-naming)
};

#ifndef FK_YAML_HAS_CXX_17
/**
 * @brief A instantiation of static_const::value instance.
 * @note This is required if inline variables are not available. C++11-14 do not provide such a feature yet.
 *
 * @tparam T A target type for the resulting static constant instance.
 */
template <typename T>
constexpr T static_const<T>::value;
#endif

} // namespace detail

FK_YAML_NAMESPACE_END

#endif /* FK_YAML_DETAIL_META_TYPE_TRAITS_HPP_ */
