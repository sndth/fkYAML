///  _______   __ __   __  _____   __  __  __
/// |   __| |_/  |  \_/  |/  _  \ /  \/  \|  |     fkYAML: A C++ header-only YAML library
/// |   __|  _  < \_   _/|  ___  |    _   |  |___  version 0.2.2
/// |__|  |_| \__|  |_|  |_|   |_|___||___|______| https://github.com/fktn-k/fkYAML
///
/// SPDX-FileCopyrightText: 2023 Kensuke Fukutani <fktn.dev@gmail.com>
/// SPDX-License-Identifier: MIT
///
/// @file

#ifndef FK_YAML_DETAIL_INPUT_DESERIALIZER_HPP_
#define FK_YAML_DETAIL_INPUT_DESERIALIZER_HPP_

#include <algorithm>
#include <cstdint>
#include <unordered_map>

#include <fkYAML/detail/macros/version_macros.hpp>
#include <fkYAML/detail/input/lexical_analyzer.hpp>
#include <fkYAML/detail/meta/input_adapter_traits.hpp>
#include <fkYAML/detail/meta/node_traits.hpp>
#include <fkYAML/detail/meta/stl_supplement.hpp>
#include <fkYAML/detail/types/lexical_token_t.hpp>
#include <fkYAML/detail/types/yaml_version_t.hpp>
#include <fkYAML/exception.hpp>

/// @brief namespace for fkYAML library.
FK_YAML_NAMESPACE_BEGIN

/// @brief namespace for internal implementations of fkYAML library.
namespace detail
{

/// @brief A class which provides the feature of deserializing YAML documents.
/// @tparam BasicNodeType A type of the container for deserialized YAML values.
template <typename BasicNodeType>
class basic_deserializer
{
    static_assert(is_basic_node<BasicNodeType>::value, "basic_deserializer only accepts basic_node<...>");

    /** A type for sequence node value containers. */
    using sequence_type = typename BasicNodeType::sequence_type;
    /** A type for mapping node value containers. */
    using mapping_type = typename BasicNodeType::mapping_type;
    /** A type for boolean node values. */
    using boolean_type = typename BasicNodeType::boolean_type;
    /** A type for integer node values. */
    using integer_type = typename BasicNodeType::integer_type;
    /** A type for float number node values. */
    using float_number_type = typename BasicNodeType::float_number_type;
    /** A type for string node values. */
    using string_type = typename BasicNodeType::string_type;

public:
    /// @brief Construct a new basic_deserializer object.
    basic_deserializer() = default;

public:
    /// @brief Deserialize a YAML-formatted source string into a YAML node.
    /// @param source A YAML-formatted source string.
    /// @return BasicNodeType A root YAML node deserialized from the source string.
    template <typename InputAdapterType, enable_if_t<is_input_adapter<InputAdapterType>::value, int> = 0>
    BasicNodeType deserialize(InputAdapterType&& input_adapter)
    {
        lexical_analyzer<BasicNodeType, InputAdapterType> lexer(std::forward<InputAdapterType>(input_adapter));

        BasicNodeType root = BasicNodeType::mapping();
        m_current_node = &root;

        lexical_token_t type = lexer.get_next_token();
        std::size_t cur_indent = lexer.get_last_token_begin_pos();
        std::size_t cur_line = lexer.get_lines_processed();

        while (type != lexical_token_t::END_OF_BUFFER)
        {
            switch (type)
            {
            case lexical_token_t::KEY_SEPARATOR: {
                bool is_stack_empty = m_node_stack.empty();
                if (is_stack_empty)
                {
                    throw fkyaml::parse_error("A key separator found without key.", cur_line, cur_indent);
                }
                break;
            }
            case lexical_token_t::VALUE_SEPARATOR:
                break;
            case lexical_token_t::ANCHOR_PREFIX: {
                m_anchor_name = lexer.get_string();
                m_needs_anchor_impl = true;
                break;
            }
            case lexical_token_t::ALIAS_PREFIX: {
                const string_type& alias_name = lexer.get_string();
                auto itr = m_anchor_table.find(alias_name);
                if (itr == m_anchor_table.end())
                {
                    throw fkyaml::parse_error(
                        "The given anchor name must appear prior to the alias node.", cur_line, cur_indent);
                }
                assign_node_value(BasicNodeType::alias_of(m_anchor_table.at(alias_name)));
                break;
            }
            case lexical_token_t::COMMENT_PREFIX:
                break;
            case lexical_token_t::YAML_VER_DIRECTIVE: {
                FK_YAML_ASSERT(m_current_node == &root);
                update_yaml_version_from(lexer.get_yaml_version());
                set_yaml_version(*m_current_node);
                break;
            }
            case lexical_token_t::TAG_DIRECTIVE:
                // TODO: implement tag directive deserialization.
            case lexical_token_t::INVALID_DIRECTIVE:
                // TODO: should output a warning log. Currently just ignore this case.
                break;
            case lexical_token_t::SEQUENCE_BLOCK_PREFIX:
                if (m_current_node->is_sequence())
                {
                    bool is_empty = m_current_node->empty();
                    if (is_empty)
                    {
                        m_indent_stack.push_back(cur_indent);
                        break;
                    }

                    // move back to the previous sequence if necessary.
                    while (!m_current_node->is_sequence() || cur_indent != m_indent_stack.back())
                    {
                        m_current_node = m_node_stack.back();
                        m_node_stack.pop_back();
                        m_indent_stack.pop_back();
                    }
                    break;
                }

                // if the current node is a mapping.
                if (m_node_stack.empty())
                {
                    throw fkyaml::parse_error("Invalid sequence block prefix(- ) found.", cur_line, cur_indent);
                }

                // move back to the previous sequence if necessary.
                while (!m_current_node->is_sequence() || cur_indent != m_indent_stack.back())
                {
                    m_current_node = m_node_stack.back();
                    m_node_stack.pop_back();
                    m_indent_stack.pop_back();
                }

                // for mappings in a sequence.
                m_current_node->template get_value_ref<sequence_type&>().emplace_back(BasicNodeType::mapping());
                m_node_stack.push_back(m_current_node);
                m_current_node = &(m_current_node->template get_value_ref<sequence_type&>().back());
                set_yaml_version(*m_current_node);
                break;
            case lexical_token_t::SEQUENCE_FLOW_BEGIN:
                *m_current_node = BasicNodeType::sequence();
                set_yaml_version(*m_current_node);
                break;
            case lexical_token_t::SEQUENCE_FLOW_END:
                m_current_node = m_node_stack.back();
                m_node_stack.pop_back();
                break;
            case lexical_token_t::MAPPING_BLOCK_PREFIX:
                type = lexer.get_next_token();
                if (type == lexical_token_t::SEQUENCE_BLOCK_PREFIX)
                {
                    *m_current_node = BasicNodeType::sequence();
                    set_yaml_version(*m_current_node);
                    cur_indent = lexer.get_last_token_begin_pos();
                    cur_line = lexer.get_lines_processed();
                    continue;
                }

                *m_current_node = BasicNodeType::mapping();
                set_yaml_version(*m_current_node);
                cur_indent = lexer.get_last_token_begin_pos();
                cur_line = lexer.get_lines_processed();
                continue;
            case lexical_token_t::MAPPING_FLOW_BEGIN:
                *m_current_node = BasicNodeType::mapping();
                set_yaml_version(*m_current_node);
                break;
            case lexical_token_t::MAPPING_FLOW_END:
                if (!m_current_node->is_mapping())
                {
                    throw fkyaml::parse_error("Invalid mapping flow ending found.", cur_line, cur_indent);
                }
                m_current_node = m_node_stack.back();
                m_node_stack.pop_back();
                break;
            case lexical_token_t::NULL_VALUE: {
                if (m_current_node->is_mapping())
                {
                    add_new_key(lexer.get_string(), cur_indent, cur_line);
                    break;
                }

                if (m_current_node->is_sequence())
                {
                    string_type str = lexer.get_string();
                    type = lexer.get_next_token();

                    // check if the current target token is a key or not.
                    if (type == lexical_token_t::KEY_SEPARATOR || type == lexical_token_t::MAPPING_BLOCK_PREFIX)
                    {
                        add_new_key(str, cur_indent, cur_line);
                        cur_indent = lexer.get_last_token_begin_pos();
                        cur_line = lexer.get_lines_processed();
                        continue;
                    }

                    assign_node_value(BasicNodeType());
                    cur_indent = lexer.get_last_token_begin_pos();
                    cur_line = lexer.get_lines_processed();
                    continue;
                }

                assign_node_value(BasicNodeType());
                break;
            }
            case lexical_token_t::BOOLEAN_VALUE: {
                if (m_current_node->is_mapping())
                {
                    add_new_key(lexer.get_string(), cur_indent, cur_line);
                    break;
                }

                if (m_current_node->is_sequence())
                {
                    boolean_type boolean = lexer.get_boolean();
                    string_type str = lexer.get_string();
                    type = lexer.get_next_token();

                    // check if the current target token is a key or not.
                    if (type == lexical_token_t::KEY_SEPARATOR || type == lexical_token_t::MAPPING_BLOCK_PREFIX)
                    {
                        add_new_key(str, cur_indent, cur_line);
                        cur_indent = lexer.get_last_token_begin_pos();
                        cur_line = lexer.get_lines_processed();
                        continue;
                    }

                    assign_node_value(BasicNodeType(boolean));
                    cur_indent = lexer.get_last_token_begin_pos();
                    cur_line = lexer.get_lines_processed();
                    continue;
                }

                assign_node_value(BasicNodeType(lexer.get_boolean()));
                break;
            }
            case lexical_token_t::INTEGER_VALUE: {
                if (m_current_node->is_mapping())
                {
                    add_new_key(lexer.get_string(), cur_indent, cur_line);
                    break;
                }

                if (m_current_node->is_sequence())
                {
                    integer_type integer = lexer.get_integer();
                    string_type str = lexer.get_string();
                    type = lexer.get_next_token();

                    // check if the current target token is a key or not.
                    if (type == lexical_token_t::KEY_SEPARATOR || type == lexical_token_t::MAPPING_BLOCK_PREFIX)
                    {
                        add_new_key(str, cur_indent, cur_line);
                        cur_indent = lexer.get_last_token_begin_pos();
                        cur_line = lexer.get_lines_processed();
                        continue;
                    }

                    assign_node_value(BasicNodeType(integer));
                    cur_indent = lexer.get_last_token_begin_pos();
                    cur_line = lexer.get_lines_processed();
                    continue;
                }

                assign_node_value(BasicNodeType(lexer.get_integer()));
                break;
            }
            case lexical_token_t::FLOAT_NUMBER_VALUE: {
                if (m_current_node->is_mapping())
                {
                    add_new_key(lexer.get_string(), cur_indent, cur_line);
                    break;
                }

                if (m_current_node->is_sequence())
                {
                    float_number_type float_val = lexer.get_float_number();
                    string_type str = lexer.get_string();
                    type = lexer.get_next_token();

                    // check if the current target token is a key or not.
                    if (type == lexical_token_t::KEY_SEPARATOR || type == lexical_token_t::MAPPING_BLOCK_PREFIX)
                    {
                        add_new_key(str, cur_indent, cur_line);
                        cur_indent = lexer.get_last_token_begin_pos();
                        cur_line = lexer.get_lines_processed();
                        continue;
                    }

                    assign_node_value(BasicNodeType(float_val));
                    cur_indent = lexer.get_last_token_begin_pos();
                    cur_line = lexer.get_lines_processed();
                    continue;
                }

                assign_node_value(BasicNodeType(lexer.get_float_number()));
                break;
            }
            case lexical_token_t::STRING_VALUE: {
                if (m_current_node->is_mapping())
                {
                    add_new_key(lexer.get_string(), cur_indent, cur_line);
                    break;
                }

                string_type str = lexer.get_string();
                if (m_current_node->is_sequence())
                {
                    type = lexer.get_next_token();

                    // check if the current target token is a key or not.
                    if (type == lexical_token_t::KEY_SEPARATOR || type == lexical_token_t::MAPPING_BLOCK_PREFIX)
                    {
                        add_new_key(str, cur_indent, cur_line);
                        cur_indent = lexer.get_last_token_begin_pos();
                        cur_line = lexer.get_lines_processed();
                        continue;
                    }

                    assign_node_value(BasicNodeType(str));
                    cur_indent = lexer.get_last_token_begin_pos();
                    cur_line = lexer.get_lines_processed();
                    continue;
                }

                assign_node_value(BasicNodeType(str));
                break;
            }
            case lexical_token_t::END_OF_DIRECTIVES:
                break;
            case lexical_token_t::END_OF_DOCUMENT:
                // TODO: This token should be handled to support multiple documents.
                break;
            default:                                                                                 // LCOV_EXCL_LINE
                throw fkyaml::parse_error("Unsupported lexical token found.", cur_line, cur_indent); // LCOV_EXCL_LINE
            }

            type = lexer.get_next_token();
            cur_indent = lexer.get_last_token_begin_pos();
            cur_line = lexer.get_lines_processed();
        }

        m_current_node = nullptr;
        m_needs_anchor_impl = false;
        m_anchor_table.clear();
        m_node_stack.clear();
        m_indent_stack.clear();

        return root;
    }

private:
    /// @brief Add new key string to the current YAML node.
    /// @param key a key string to be added to the current YAML node.
    void add_new_key(const string_type& key, const std::size_t indent, const std::size_t line)
    {
        if (!m_indent_stack.empty() && indent < m_indent_stack.back())
        {
            auto target_itr = std::find(m_indent_stack.rbegin(), m_indent_stack.rend(), indent);
            bool is_indent_valid = (target_itr != m_indent_stack.rend());
            if (!is_indent_valid)
            {
                throw fkyaml::parse_error("Detected invalid indentaion.", line, indent);
            }

            auto pop_num = std::distance(m_indent_stack.rbegin(), target_itr);
            for (auto i = 0; i < pop_num; i++)
            {
                // move back to the previous container node.
                m_current_node = m_node_stack.back();
                m_node_stack.pop_back();
                m_indent_stack.pop_back();
            }
        }

        if (m_current_node->is_sequence())
        {
            m_current_node->template get_value_ref<sequence_type&>().emplace_back(BasicNodeType::mapping());
            m_node_stack.push_back(m_current_node);
            m_current_node = &(m_current_node->operator[](m_current_node->size() - 1));
        }

        mapping_type& map = m_current_node->template get_value_ref<mapping_type&>();
        bool is_empty = map.empty();
        if (is_empty)
        {
            m_indent_stack.push_back(indent);
        }
        else
        {
            // check key duplication in the current mapping if not empty.
            auto itr = map.find(key);
            if (itr != map.end())
            {
                throw fkyaml::parse_error("Detected duplication in mapping keys.", line, indent);
            }
        }

        map.emplace(key, BasicNodeType());
        m_node_stack.push_back(m_current_node);
        m_current_node = &(m_current_node->operator[](key));
    }

    /// @brief Assign node value to the current node.
    /// @param node_value A rvalue BasicNodeType object to be assigned to the current node.
    void assign_node_value(BasicNodeType&& node_value) noexcept
    {
        if (m_current_node->is_sequence())
        {
            m_current_node->template get_value_ref<sequence_type&>().emplace_back(std::move(node_value));
            set_yaml_version(m_current_node->template get_value_ref<sequence_type&>().back());
            if (m_needs_anchor_impl)
            {
                m_current_node->template get_value_ref<sequence_type&>().back().add_anchor_name(m_anchor_name);
                m_anchor_table[m_anchor_name] = m_current_node->template get_value_ref<sequence_type&>().back();
                m_needs_anchor_impl = false;
                m_anchor_name.clear();
            }
            return;
        }

        // a scalar node
        *m_current_node = std::move(node_value);
        set_yaml_version(*m_current_node);
        if (m_needs_anchor_impl)
        {
            m_current_node->add_anchor_name(m_anchor_name);
            m_anchor_table[m_anchor_name] = *m_current_node;
            m_needs_anchor_impl = false;
            m_anchor_name.clear();
        }
        m_current_node = m_node_stack.back();
        m_node_stack.pop_back();
    }

    /// @brief Set the yaml_version_t object to the given node.
    /// @param node A BasicNodeType object to be set the yaml_version_t object.
    void set_yaml_version(BasicNodeType& node) noexcept
    {
        node.set_yaml_version(m_yaml_version);
    }

    /// @brief Update the target YAML version with an input string.
    /// @param version_str A YAML version string.
    void update_yaml_version_from(const string_type& version_str) noexcept
    {
        if (version_str == "1.1")
        {
            m_yaml_version = yaml_version_t::VER_1_1;
            return;
        }
        m_yaml_version = yaml_version_t::VER_1_2;
    }

private:
    /// The currently focused YAML node.
    BasicNodeType* m_current_node {nullptr};
    /// The stack of YAML nodes.
    std::vector<BasicNodeType*> m_node_stack {};
    /// The stack of indentation widths.
    std::vector<std::size_t> m_indent_stack {};
    /// The YAML version specification type.
    yaml_version_t m_yaml_version {yaml_version_t::VER_1_2};
    /// A flag to determine the need for YAML anchor node implementation.
    bool m_needs_anchor_impl {false};
    /// The last YAML anchor name.
    string_type m_anchor_name {};
    /// The table of YAML anchor nodes.
    std::unordered_map<std::string, BasicNodeType> m_anchor_table {};
};

} // namespace detail

FK_YAML_NAMESPACE_END

#endif /* FK_YAML_DETAIL_INPUT_DESERIALIZER_HPP_ */
