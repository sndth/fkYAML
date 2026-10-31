//  _______   __ __   __  _____   __  __  __
// |   __| |_/  |  \_/  |/  _  \ /  \/  \|  |     fkYAML: A C++ header-only YAML library
// |   __|  _  < \_   _/|  ___  |    _   |  |___  version 0.5.0
// |__|  |_| \__|  |_|  |_|   |_|___||___|______| https://github.com/fktn-k/fkYAML
//
// SPDX-FileCopyrightText: 2023-2026 Kensuke Fukutani <fktn.dev@gmail.com>
// SPDX-License-Identifier: MIT

#ifndef FK_YAML_DETAIL_INPUT_NODE_TREE_BUILDER_HPP
#define FK_YAML_DETAIL_INPUT_NODE_TREE_BUILDER_HPP

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <fkYAML/detail/macros/define_macros.hpp>
#include <fkYAML/detail/document_metainfo.hpp>
#include <fkYAML/detail/input/block_scalar_header.hpp>
#include <fkYAML/detail/input/scalar_parser.hpp>
#include <fkYAML/detail/input/tag_t.hpp>
#include <fkYAML/detail/meta/node_traits.hpp>
#include <fkYAML/detail/str_view.hpp>
#include <fkYAML/detail/types/lexical_token_t.hpp>

FK_YAML_DETAIL_NAMESPACE_BEGIN

/// @brief The operations the deserializer needs from a tree of basic_node objects.
/// @tparam BasicNodeType A type of the container for deserialized YAML values.
template <typename BasicNodeType>
class node_tree_builder {
    static_assert(is_basic_node<BasicNodeType>::value, "node_tree_builder only accepts basic_node<...>");

public:
    /// A type for the nodes of the tree.
    using node_type = BasicNodeType;
    /// A type for the metainfo of a document.
    using metainfo_type = document_metainfo<node_type>;

    /// @brief What identifies an anchor among the ones of a document.
    struct anchor_id {
        anchor_id(std::string anchor_name, uint32_t anchor_offset)
            : name(std::move(anchor_name)),
              offset(anchor_offset) {
        }

        std::string name;
        uint32_t offset;
    };

    /// @brief Makes the given node aware of the document it belongs to.
    /// @param node The target node.
    /// @param p_meta The metainfo of the document.
    void set_metainfo(node_type& node, const std::shared_ptr<metainfo_type>& p_meta) const noexcept {
        node.extras().p_meta = p_meta;
    }

    /// @brief Appends an entry to a sequence.
    /// @param seq The target sequence.
    /// @param entry The new entry.
    /// @return The new entry in the sequence.
    node_type* append(node_type& seq, node_type&& entry) const {
        auto& entries = seq.as_seq();
        entries.emplace_back(std::move(entry));
        return &entries.back();
    }

    /// @brief Inserts a key with a null value into a mapping unless the mapping has the key already.
    /// @param map The target mapping.
    /// @param key The new key.
    /// @return The value of the key, and whether the key is a new one.
    std::pair<node_type*, bool> insert(node_type& map, node_type&& key) const {
        auto result = map.as_map().emplace(std::move(key), node_type());
        return {&(result.first->second), result.second};
    }

    /// @brief Creates a flow scalar node.
    /// @param line Current line.
    /// @param indent Current indentation.
    /// @param lex_type Lexical token type for the scalar.
    /// @param tag_type Tag type for the scalar.
    /// @param token Scalar contents.
    /// @param is_verbatim Whether the contents are known to be the same as the token.
    /// @return The scalar node.
    node_type make_flow_scalar(
        uint32_t line, uint32_t indent, lexical_token_t lex_type, tag_t tag_type, str_view token,
        bool is_verbatim) const {
        return scalar_parser<node_type>(line, indent).parse_flow(lex_type, tag_type, token, is_verbatim);
    }

    /// @brief Creates a block scalar node.
    /// @param line Current line.
    /// @param indent Current indentation.
    /// @param lex_type Lexical token type for the scalar.
    /// @param tag_type Tag type for the scalar.
    /// @param token Scalar contents.
    /// @param header Block scalar header information.
    /// @return The scalar node.
    node_type make_block_scalar(
        uint32_t line, uint32_t indent, lexical_token_t lex_type, tag_t tag_type, str_view token,
        const block_scalar_header& header) const {
        return scalar_parser<node_type>(line, indent).parse_block(lex_type, tag_type, token, header);
    }

    /// @brief Turns the given node into an empty string.
    /// @param node The target node.
    void make_empty_string(node_type& node) const {
        node = node_type(typename node_type::string_type());
    }

    /// @brief Turns the given node into an alias of the latest anchor with the given name, if there is any.
    /// @param node The target node.
    /// @param name The anchor name.
    /// @param p_meta The metainfo of the document.
    /// @return true if the anchor is found, false otherwise.
    bool make_alias(node_type& node, str_view name, const std::shared_ptr<metainfo_type>& p_meta) const {
        std::string anchor_name(name.begin(), name.end());
        const auto anchor_counts = static_cast<uint32_t>(p_meta->anchor_table.count(anchor_name));
        if FK_YAML_UNLIKELY (anchor_counts == 0) {
            return false;
        }

        node.m_attrs.set(detail::node_attr_bits::alias_bit);
        node.prop().anchor = std::move(anchor_name);
        node.m_attrs.set_anchor_offset(anchor_counts - 1);
        node.extras().p_meta = p_meta;
        return true;
    }

    /// @brief Gets what identifies the anchor of the given alias.
    /// @param alias The target alias node.
    /// @return The anchor identity.
    anchor_id get_anchor_id(const node_type& alias) const {
        return {alias.get_anchor_name(), alias.m_attrs.get_anchor_offset()};
    }

    /// @brief Checks whether the given node is the anchor with the given identity.
    /// @param node The target node.
    /// @param id The anchor identity.
    /// @return true if the node is the anchor, false otherwise.
    bool is_anchor_of(const node_type& node, const anchor_id& id) const {
        return node.is_anchor() && node.m_attrs.get_anchor_offset() == id.offset && node.get_anchor_name() == id.name;
    }

    /// @brief Sets an anchor name to the given node.
    /// @param node The target node.
    /// @param name The anchor name.
    /// @param p_meta The metainfo of the document.
    void set_anchor(node_type& node, str_view name, const std::shared_ptr<metainfo_type>& p_meta) const {
        node.extras().p_meta = p_meta;
        node.add_anchor_name(std::string(name.begin(), name.end()));
    }

    /// @brief Sets a tag name to the given node.
    /// @param node The target node.
    /// @param name The tag name.
    void set_tag(node_type& node, str_view name, tag_t /*unused*/) const {
        node.add_tag_name(std::string(name.begin(), name.end()));
    }

    /// @brief Finishes a document.
    /// @param root The root node of the document.
    /// @param p_meta The metainfo of the document.
    void finish_document(node_type& root, const std::shared_ptr<metainfo_type>& p_meta) const {
        // The root keeps the document alive and is where the serializer finds the anchors.
        root.extras().p_meta = p_meta;
        share_metainfo_with_collections(root, p_meta);
    }

private:
    /// @brief Shares the document metainfo with all the collections of a document which has anchors.
    /// @note The serializer finds anchors through the metainfo of the node it is given, which may be any of them.
    /// @param root The root node of the document.
    /// @param p_meta The metainfo of the document.
    void share_metainfo_with_collections(const node_type& root, const std::shared_ptr<metainfo_type>& p_meta) const {
        if (p_meta->anchor_table.empty()) {
            return;
        }

        std::vector<const node_type*> stack {&root};
        while (!stack.empty()) {
            const node_type* p_node = stack.back();
            stack.pop_back();
            if (p_node->is_alias()) {
                continue;
            }
            if (p_node->is_sequence()) {
                p_node->extras().p_meta = p_meta;
                for (const auto& entry : p_node->as_seq()) {
                    stack.push_back(&entry);
                }
            }
            else if (p_node->is_mapping()) {
                p_node->extras().p_meta = p_meta;
                for (const auto& entry : p_node->as_map()) {
                    stack.push_back(&entry.first);
                    stack.push_back(&entry.second);
                }
            }
        }
    }
};

FK_YAML_DETAIL_NAMESPACE_END

#endif /* FK_YAML_DETAIL_INPUT_NODE_TREE_BUILDER_HPP */
