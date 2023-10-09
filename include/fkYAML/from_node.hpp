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

#ifndef FK_YAML_FROM_NODE_HPP_
#define FK_YAML_FROM_NODE_HPP_

#include <limits>
#include <utility>

#include "fkYAML/version_macros.hpp"
#include "fkYAML/exception.hpp"
#include "fkYAML/node_t.hpp"
#include "fkYAML/type_traits.hpp"

/**
 * @namespace fkyaml
 * @brief namespace for fkYAML library.
 */
FK_YAML_NAMESPACE_BEGIN

/**
 * @namespace detail
 * @brief namespace for internal implementations of fkYAML library.
 */
namespace detail
{

// Avoid ambiguity between fkyaml::exception and std::exception
using fkyaml::exception;

///////////////////
//   from_node   //
///////////////////

/**
 * @brief from_node function for BasicNodeType::sequence_type objects.
 *
 * @tparam BasicNodeType A basic_node template instance type.
 * @param n A basic_node object.
 * @param s A sequence node value object.
 */
template <typename BasicNodeType, enable_if_t<is_basic_node<BasicNodeType>::value, int> = 0>
inline void from_node(const BasicNodeType& n, typename BasicNodeType::sequence_type& s)
{
    if (!n.is_sequence())
    {
        throw exception("The target node value type is not sequence type.");
    }
    s = n.to_sequence();
}

/**
 * @brief from_node function for BasicNodeType::mapping_type objects.
 *
 * @tparam BasicNodeType A basic_node template instance type.
 * @param n A basic_node object.
 * @param m A mapping node value object.
 */
template <typename BasicNodeType, enable_if_t<is_basic_node<BasicNodeType>::value, int> = 0>
inline void from_node(const BasicNodeType& n, typename BasicNodeType::mapping_type& m)
{
    if (!n.is_mapping())
    {
        throw exception("The target node value type is not mapping type.");
    }

    for (auto pair : n.to_mapping())
    {
        m.emplace(pair.first, pair.second);
    }
}

/**
 * @brief from_node function for null node values.
 *
 * @tparam BasicNodeType A basic_node template instance type.
 * @param n A basic_node object.
 * @param null A null node value object.
 */
template <typename BasicNodeType, enable_if_t<is_basic_node<BasicNodeType>::value, int> = 0>
inline void from_node(const BasicNodeType& n, std::nullptr_t& null)
{
    // to ensure the target node value type is null.
    if (!n.is_null())
    {
        throw exception("The target node value type is not null type.");
    }
    null = nullptr;
}

/**
 * @brief from_node function for BasicNodeType::boolean_type objects.
 *
 * @tparam BasicNodeType A basic_node template instance type.
 * @param n A basic_node object.
 * @param b A boolean node value object.
 */
template <typename BasicNodeType, enable_if_t<is_basic_node<BasicNodeType>::value, int> = 0>
inline void from_node(const BasicNodeType& n, typename BasicNodeType::boolean_type& b)
{
    if (!n.is_boolean())
    {
        throw exception("The target node value type is not boolean type.");
    }
    b = n.to_boolean();
}

/**
 * @brief from_node function for BasicNodeType::integer_type objects.
 *
 * @tparam BasicNodeType A basic_node template instance type.
 * @param n A basic_node object.
 * @param i An integer node value object.
 */
template <typename BasicNodeType, enable_if_t<is_basic_node<BasicNodeType>::value, int> = 0>
inline void from_node(const BasicNodeType& n, typename BasicNodeType::integer_type& i)
{
    if (!n.is_integer())
    {
        throw exception("The target node value type is not integer type.");
    }
    i = n.to_integer();
}

/**
 * @brief from_node function for other integer objects. (i.e., not BasicNodeType::integer_type)
 *
 * @tparam BasicNodeType A basic_node template instance type.
 * @tparam IntegerType An integer value type.
 * @param n A basic_node object.
 * @param i An integer node value object.
 */
template <
    typename BasicNodeType, typename IntegerType,
    enable_if_t<
        is_non_bool_integral<IntegerType>::value &&
            !std::is_same<IntegerType, typename BasicNodeType::integer_type>::value,
        int> = 0>
inline void from_node(const BasicNodeType& n, IntegerType& i)
{
    if (!n.is_integer())
    {
        throw exception("The target node value type is not integer type.");
    }

    // under/overflow check.
    typename BasicNodeType::integer_type tmp_int = n.to_integer();
    if (tmp_int < static_cast<typename BasicNodeType::integer_type>(std::numeric_limits<IntegerType>::min()))
    {
        throw exception("Integer value underflow detected.");
    }
    if (static_cast<typename BasicNodeType::integer_type>(std::numeric_limits<IntegerType>::max()) < tmp_int)
    {
        throw exception("Integer value overflow detected.");
    }

    i = static_cast<IntegerType>(tmp_int);
}

/**
 * @brief from_node function for BasicNodeType::float_number_type objects.
 *
 * @tparam BasicNodeType A basic_node template instance type.
 * @param n A basic_node object.
 * @param f A float number node value object.
 */
template <typename BasicNodeType, enable_if_t<is_basic_node<BasicNodeType>::value, int> = 0>
inline void from_node(const BasicNodeType& n, typename BasicNodeType::float_number_type& f)
{
    if (!n.is_float_number())
    {
        throw exception("The target node value type is not float number type.");
    }
    f = n.to_float_number();
}

/**
 * @brief from_node function for other float number objects. (i.e., not BasicNodeType::float_number_type)
 *
 * @tparam BasicNodeType A basic_node template instance type.
 * @tparam FloatType A float number value type.
 * @param n A basic_node object.
 * @param f A float number node value object.
 */
template <
    typename BasicNodeType, typename FloatType,
    enable_if_t<
        std::is_floating_point<FloatType>::value &&
            !std::is_same<FloatType, typename BasicNodeType::float_number_type>::value,
        int> = 0>
inline void from_node(const BasicNodeType& n, FloatType& f)
{
    if (!n.is_float_number())
    {
        throw exception("The target node value type is not float number type.");
    }

    typename BasicNodeType::float_number_type tmp_float = n.to_float_number();
    if (tmp_float < std::numeric_limits<FloatType>::min())
    {
        throw exception("Floating point value underflow detected.");
    }
    if (std::numeric_limits<FloatType>::max() < tmp_float)
    {
        throw exception("Floating point value overflow detected.");
    }

    f = tmp_float;
}

/**
 * @brief from_node function for BasicNodeType::string_type objects.
 *
 * @tparam BasicNodeType A basic_node template instance type.
 * @param n A basic_node object.
 * @param s A string node value object.
 */
template <typename BasicNodeType, enable_if_t<is_basic_node<BasicNodeType>::value, int> = 0>
inline void from_node(const BasicNodeType& n, typename BasicNodeType::string_type& s)
{
    if (!n.is_string())
    {
        throw exception("The target node value type is not string type.");
    }
    s = n.to_string();
}

/**
 * @brief A function object to call from_node functions.
 * @note User-defined specialization is available by providing implementation **OUTSIDE** fkyaml namespace.
 */
struct from_node_fn
{
    /**
     * @brief Call from_node function suitable for the given T type.
     *
     * @tparam BasicNodeType A basic_node template instance type.
     * @tparam T A target value type assigned from the basic_node object.
     * @param n A basic_node object.
     * @param val A target object assigned from the basic_node object.
     * @return decltype(from_node(n, std::forward<T>(val))) void by default. User can set it to some other type.
     */
    template <typename BasicNodeType, typename T>
    auto operator()(const BasicNodeType& n, T&& val) const noexcept(noexcept(from_node(n, std::forward<T>(val))))
        -> decltype(from_node(n, std::forward<T>(val)))
    {
        return from_node(n, std::forward<T>(val));
    }
};

} // namespace detail

#ifndef FK_YAML_HAS_CXX_17
// anonymous namespace to hold `from_node` functor.
// see http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4381.html for why it's needed.
namespace // NOLINT(cert-dcl59-cpp,fuchsia-header-anon-namespaces,google-build-namespaces)
{
#endif

/**
 * @brief A blobal object to represent ADL friendly from_node functor.
 */
// NOLINTNEXTLINE(misc-definitions-in-headers)
FK_YAML_INLINE_VAR constexpr const auto& from_node = detail::static_const<detail::from_node_fn>::value;

#ifndef FK_YAML_HAS_CXX_17
} // namespace
#endif

FK_YAML_NAMESPACE_END

#endif /* FK_YAML_FROM_NODE_HPP_ */