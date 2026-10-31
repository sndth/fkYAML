//  _______   __ __   __  _____   __  __  __
// |   __| |_/  |  \_/  |/  _  \ /  \/  \|  |     fkYAML: A C++ header-only YAML library
// |   __|  _  < \_   _/|  ___  |    _   |  |___  version 0.5.0
// |__|  |_| \__|  |_|  |_|   |_|___||___|______| https://github.com/fktn-k/fkYAML
//
// SPDX-FileCopyrightText: 2023-2026 Kensuke Fukutani <fktn.dev@gmail.com>
// SPDX-License-Identifier: MIT

#ifndef FK_YAML_DETAIL_INPUT_FLAT_TREE_BUILDER_HPP
#define FK_YAML_DETAIL_INPUT_FLAT_TREE_BUILDER_HPP

#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <new>
#include <string>
#include <utility>
#include <vector>

#include <fkYAML/detail/macros/define_macros.hpp>
#include <fkYAML/detail/conversions/scalar_conv.hpp>
#include <fkYAML/detail/document_metainfo.hpp>
#include <fkYAML/detail/input/block_scalar_header.hpp>
#include <fkYAML/detail/input/scalar_parser.hpp>
#include <fkYAML/detail/input/scalar_scanner.hpp>
#include <fkYAML/detail/input/tag_t.hpp>
#include <fkYAML/detail/meta/node_traits.hpp>
#include <fkYAML/detail/str_view.hpp>
#include <fkYAML/detail/types/lexical_token_t.hpp>
#include <fkYAML/exception.hpp>
#include <fkYAML/node_type.hpp>

FK_YAML_DETAIL_NAMESPACE_BEGIN

/// @brief A node of the tree behind a document.
/// @note Nodes live in the blocks of a flat_storage object and never move, so they are linked with pointers. Scalars
/// refer to their contents and are converted only when asked to. Copying a node copies its value, not its links.
struct flat_node {
    /// @brief The kinds of nodes.
    /// @note It is compared with the lowest bits of a node, which a scoped enumeration would make a chore of.
    enum kind_t : uint8_t { // NOLINT(cppcoreguidelines-use-enum-class)
        NULL_SCALAR,        //!< An empty node.
        PLAIN_SCALAR,       //!< A scalar whose type depends on its contents.
        STRING_SCALAR,      //!< A scalar which is a string unless it is tagged otherwise.
        SEQUENCE,           //!< A sequence.
        MAPPING,            //!< A mapping.
        ANCHOR,             //!< An anchor, whose value is p_target.
        ALIAS,              //!< An alias, whose value is p_target.
    };

    flat_node() noexcept
        : p_str(nullptr) {
    }

    /// @brief Constructs an empty node of the given type.
    /// @param type Either node_type::SEQUENCE or node_type::MAPPING.
    explicit flat_node(node_type type) noexcept
        : p_str(nullptr),
          bits(type == node_type::SEQUENCE ? SEQUENCE : MAPPING) {
    }

    flat_node(const flat_node& rhs) noexcept
        : p_str(nullptr) {
        assign(rhs);
    }

    flat_node(flat_node&& rhs) noexcept
        : p_str(nullptr) {
        assign(rhs);
    }

    // Assigning a node to itself is harmless, as a value is nothing more than a few members to copy.
    // NOLINTNEXTLINE(bugprone-unhandled-self-assignment,cert-oop54-cpp)
    flat_node& operator=(const flat_node& rhs) noexcept {
        assign(rhs);
        return *this;
    }

    flat_node& operator=(flat_node&& rhs) noexcept {
        assign(rhs);
        return *this;
    }

    ~flat_node() = default;

    /// @brief Constructs an empty sequence.
    static flat_node sequence() noexcept {
        return flat_node(node_type::SEQUENCE);
    }

    /// @brief Constructs an empty mapping.
    static flat_node mapping() noexcept {
        return flat_node(node_type::MAPPING);
    }

    /// @brief Swaps the values of two nodes.
    void swap(flat_node& rhs) noexcept {
        const flat_node tmp(rhs);
        rhs.assign(*this);
        assign(tmp);
    }

    /// @brief Gets the kind of this node.
    kind_t kind() const noexcept {
        return static_cast<kind_t>(bits & 0xFFu);
    }

    /// @brief Gets the index of the properties of this node, which is 0 if there are none.
    uint32_t props_index() const noexcept {
        return bits >> 8u;
    }

    /// @brief Gets the node which holds the value of this node.
    const flat_node& resolve() const noexcept {
        return kind() < ANCHOR ? *this : *p_target;
    }

    /// @brief Gets the node which holds the value of this node.
    flat_node& resolve() noexcept {
        return kind() < ANCHOR ? *this : *p_target;
    }

    bool is_sequence() const noexcept {
        return resolve().kind() == SEQUENCE;
    }

    bool is_mapping() const noexcept {
        return resolve().kind() == MAPPING;
    }

    bool is_scalar() const noexcept {
        return resolve().kind() < SEQUENCE;
    }

    bool is_alias() const noexcept {
        return kind() == ALIAS;
    }

    /// @brief Gets the contents of this scalar.
    str_view text() const noexcept {
        return {p_str, size};
    }

    union {
        const char* p_str;   //!< The contents of a scalar.
        flat_node* p_first;  //!< The first entry of a collection.
        flat_node* p_target; //!< The value of an anchor or an alias.
    };
    /// The last entry of a collection.
    flat_node* p_last {nullptr};
    /// The next entry of the parent collection.
    flat_node* p_next {nullptr};
    /// The key of this mapping value.
    flat_node* p_key {nullptr};
    /// The kind in the lowest 8 bits and the index of the properties above them.
    uint32_t bits {NULL_SCALAR};
    /// The length of a scalar or the number of the entries of a collection.
    uint32_t size {0};

private:
    void assign(const flat_node& rhs) noexcept {
        p_first = rhs.p_first;
        p_last = rhs.p_last;
        bits = rhs.bits;
        size = rhs.size;
    }
};

/// @brief A partial specialization of is_basic_node_impl for flat_node, which can stand in for basic_node in the
/// parsing code.
template <>
struct is_basic_node_impl<flat_node> : std::true_type {};

/// @brief The properties of a flat_node object.
struct flat_props {
    /// The anchor name of an anchor or an alias.
    str_view anchor;
    /// The tag name.
    str_view tag;
    /// The type of the tag.
    tag_t tag_type {tag_t::NONE};
    /// The index of the document metainfo of a root node.
    uint32_t document {0};
};

/// @brief The storage of the trees built from one input.
class flat_storage {
public:
    using metainfo_type = document_metainfo<flat_node>;

    /// @brief Takes a copy of the input, which all the nodes refer to.
    /// @param input The input buffer.
    explicit flat_storage(str_view input)
        : source(input.begin(), input.end()),
          // Most of the inputs take less than a node per 12 bytes, and the blocks only cost what is used of them.
          m_next_block_nodes((input.size() / 12) + 32) {
    }

    flat_storage(const flat_storage&) = delete;
    flat_storage& operator=(const flat_storage&) = delete;
    flat_storage(flat_storage&&) = delete;
    flat_storage& operator=(flat_storage&&) = delete;
    ~flat_storage() = default;

    /// @brief Gets a new node with the given value.
    /// @param value The value of the node.
    /// @return The new node.
    flat_node* new_node(const flat_node& value) {
        if FK_YAML_UNLIKELY (mp_free_node == mp_nodes_end) {
            add_node_block();
        }
        return new (mp_free_node++) flat_node(value);
    }

    /// @brief Gets a new empty node.
    /// @return The new node.
    flat_node* new_node() {
        if FK_YAML_UNLIKELY (mp_free_node == mp_nodes_end) {
            add_node_block();
        }
        return new (mp_free_node++) flat_node();
    }

    /// @brief Keeps a copy of the given characters.
    /// @param chars The characters to keep.
    /// @return The copy of the given characters.
    str_view keep_chars(str_view chars) {
        const std::size_t size = chars.size();
        if FK_YAML_UNLIKELY (size > m_free_chars) {
            const std::size_t block_size = size > 4096 ? size : 4096;
            m_blocks.emplace_back(new char[block_size]);
            mp_free_char = m_blocks.back().get();
            m_free_chars = block_size;
        }

        char* p_chars = mp_free_char;
        if (size > 0) {
            std::memcpy(p_chars, chars.data(), size);
        }
        mp_free_char += size;
        m_free_chars -= size;
        return {p_chars, size};
    }

    /// @brief Gets the properties of the given node, which must have some.
    const flat_props& get_props(const flat_node& node) const noexcept {
        return props[node.props_index()];
    }

    /// @brief Gets the properties of the given node, creating them on first use.
    flat_props& ensure_props(flat_node& node) {
        if (node.props_index() == 0) {
            const std::size_t index = props.size();
            if FK_YAML_UNLIKELY (index > 0xFFFFFFu) {
                throw fkyaml::exception("Too many nodes with properties in a document.");
            }
            props.emplace_back();
            node.bits |= static_cast<uint32_t>(index) << 8u;
        }
        return props[node.props_index()];
    }

    /// @brief Gets the type of the value of the given node.
    /// @note The rules are the ones the scalar_parser class goes by, including how values which do not fit their
    /// types are recovered as strings.
    /// @param node The target node.
    /// @return The type of the value of the node.
    node_type type_of(const flat_node& node) const {
        const flat_node& value = node.resolve();
        switch (value.kind()) {
        case flat_node::SEQUENCE:
            return node_type::SEQUENCE;
        case flat_node::MAPPING:
            return node_type::MAPPING;
        case flat_node::NULL_SCALAR:
            return node_type::NULL_OBJECT;
        default:
            break;
        }

        switch (props[value.props_index()].tag_type) {
        case tag_t::NULL_VALUE:
            return node_type::NULL_OBJECT;
        case tag_t::BOOLEAN:
            return node_type::BOOLEAN;
        case tag_t::INTEGER:
            return node_type::INTEGER;
        case tag_t::FLOATING_NUMBER:
            return node_type::FLOAT;
        case tag_t::STRING:
        case tag_t::NON_SPECIFIC:
            return node_type::STRING;
        default:
            break;
        }

        if (value.kind() == flat_node::STRING_SCALAR) {
            return node_type::STRING;
        }

        const str_view text = value.text();
        const node_type type = scalar_scanner::scan(text.begin(), text.end());
        if (type == node_type::INTEGER) {
            std::int64_t integer = 0;
            std::uint64_t unsigned_integer = 0;
            const bool fits =
                atoi(text.begin(), text.end(), integer) || atoi(text.begin(), text.end(), unsigned_integer);
            return fits ? type : node_type::STRING;
        }
        if (type == node_type::FLOAT) {
            double float_val = 0;
            return atof(text.begin(), text.end(), float_val) ? type : node_type::STRING;
        }
        return type;
    }

    /// @brief Checks whether two nodes have the same value, the way basic_node objects are compared.
    /// @param lhs A node to be compared.
    /// @param rhs A node to be compared.
    /// @return true if the two nodes have the same value, false otherwise.
    bool equals(const flat_node& lhs, const flat_node& rhs) const {
        const flat_node& l = lhs.resolve();
        const flat_node& r = rhs.resolve();

        // By far the most common case, which keeps looking for duplicate keys cheap.
        if (l.bits == flat_node::STRING_SCALAR && r.bits == flat_node::STRING_SCALAR) {
            return l.text() == r.text();
        }

        const node_type type = type_of(l);
        if (type != type_of(r)) {
            return false;
        }

        switch (type) {
        case node_type::SEQUENCE:
            return equal_sequences(l, r);
        case node_type::MAPPING:
            return equal_mappings(l, r);
        case node_type::NULL_OBJECT:
            return true;
        case node_type::BOOLEAN: {
            bool l_bool = false;
            bool r_bool = false;
            return atob(l.text().begin(), l.text().end(), l_bool) && atob(r.text().begin(), r.text().end(), r_bool) &&
                   l_bool == r_bool;
        }
        case node_type::INTEGER: {
            std::int64_t l_int = 0;
            std::int64_t r_int = 0;
            const bool are_signed =
                atoi(l.text().begin(), l.text().end(), l_int) && atoi(r.text().begin(), r.text().end(), r_int);
            return are_signed ? l_int == r_int : l.text() == r.text();
        }
        case node_type::FLOAT: {
            double l_float = 0;
            double r_float = 0;
            return atof(l.text().begin(), l.text().end(), l_float) && atof(r.text().begin(), r.text().end(), r_float) &&
                   l_float == r_float;
        }
        default:
            return l.text() == r.text();
        }
    }

    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    /// The copy of the input.
    std::string source;
    /// The properties of the nodes, where the first element stands for no properties.
    std::vector<flat_props> props {flat_props {}};
    /// The metainfo of the documents.
    std::vector<std::shared_ptr<metainfo_type>> metainfos;
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)

private:
    void add_node_block() {
        const std::size_t num_nodes = m_next_block_nodes;
        m_blocks.emplace_back(new char[num_nodes * sizeof(flat_node)]);
        mp_free_node = reinterpret_cast<flat_node*>(m_blocks.back().get());
        mp_nodes_end = mp_free_node + num_nodes;
        m_next_block_nodes = num_nodes * 2;
    }

    bool equal_sequences(const flat_node& lhs, const flat_node& rhs) const {
        if (lhs.size != rhs.size) {
            return false;
        }
        const flat_node* p_rhs = rhs.p_first;
        for (const flat_node* p_lhs = lhs.p_first; p_lhs != nullptr; p_lhs = p_lhs->p_next) {
            if (!equals(*p_lhs, *p_rhs)) {
                return false;
            }
            p_rhs = p_rhs->p_next;
        }
        return true;
    }

    bool equal_mappings(const flat_node& lhs, const flat_node& rhs) const {
        if (lhs.size != rhs.size) {
            return false;
        }
        for (const flat_node* p_lhs = lhs.p_first; p_lhs != nullptr; p_lhs = p_lhs->p_next) {
            const flat_node* p_rhs = rhs.p_first;
            while (p_rhs != nullptr && !equals(*p_lhs->p_key, *p_rhs->p_key)) {
                p_rhs = p_rhs->p_next;
            }
            if (p_rhs == nullptr || !equals(*p_lhs, *p_rhs)) {
                return false;
            }
        }
        return true;
    }

    /// The blocks all the nodes and the kept characters live in.
    std::vector<std::unique_ptr<char[]>> m_blocks;
    /// The next free node in the current block.
    flat_node* mp_free_node {nullptr};
    /// The end of the current block of nodes.
    flat_node* mp_nodes_end {nullptr};
    /// The number of nodes in the next block.
    std::size_t m_next_block_nodes {0};
    /// The next free character in the current block.
    char* mp_free_char {nullptr};
    /// The number of free characters in the current block.
    std::size_t m_free_chars {0};
};

/// @brief The operations the deserializer needs from a tree of flat_node objects.
/// @tparam BasicNodeType The type of nodes which validates scalars against their tags.
template <typename BasicNodeType>
class flat_tree_builder {
public:
    /// A type for the nodes of the tree.
    using node_type = flat_node;
    /// A type for the metainfo of a document.
    using metainfo_type = flat_storage::metainfo_type;
    /// What identifies an anchor among the ones of a document.
    using anchor_id = const flat_node*;

    flat_tree_builder() = default;

    /// @brief Constructs a flat_tree_builder object.
    /// @param p_storage The storage of the tree.
    explicit flat_tree_builder(flat_storage* p_storage) noexcept
        : mp_storage(p_storage) {
    }

    /// @brief Does nothing, since the nodes of a tree share their storage anyway.
    void set_metainfo(flat_node& /*unused*/, const std::shared_ptr<metainfo_type>& /*unused*/) const noexcept {
    }

    /// @brief Appends an entry to a sequence.
    /// @param seq The target sequence.
    /// @param entry The new entry.
    /// @return The new entry in the sequence.
    flat_node* append(flat_node& seq, const flat_node& entry) {
        flat_node& target = seq.resolve();
        if FK_YAML_UNLIKELY (target.kind() != flat_node::SEQUENCE) {
            throw_type_error("The target node is not of a sequence type.", target);
        }
        return link(target, mp_storage->new_node(entry));
    }

    /// @brief Inserts a key with a null value into a mapping unless the mapping has the key already.
    /// @param map The target mapping.
    /// @param key The new key.
    /// @return The value of the key, and whether the key is a new one.
    // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
    std::pair<flat_node*, bool> insert(flat_node& map, flat_node&& key) {
        flat_node& target = map.resolve();
        if FK_YAML_UNLIKELY (target.kind() != flat_node::MAPPING) {
            throw_type_error("The target node is not of a mapping type.", target);
        }

        // Scanning a plain key once spares every comparison with it from doing so.
        if (key.bits == flat_node::PLAIN_SCALAR) {
            const str_view text = key.text();
            if (scalar_scanner::scan(text.begin(), text.end()) == fkyaml::node_type::STRING) {
                key.bits = flat_node::STRING_SCALAR;
            }
        }

        // Small mappings are not indexed, so their keys have no use for a hash.
        const std::uint64_t hash = target.size > max_scanned_entries ? hash_of(key) : 0;
        if (flat_node* p_existing = find(target, key, hash)) {
            return {p_existing, false};
        }

        flat_node* p_key = mp_storage->new_node(key);
        flat_node* p_value = link(target, mp_storage->new_node());
        p_value->p_key = p_key;

        if (target.size == max_scanned_entries + 1) {
            for (flat_node* p_entry = target.p_first; p_entry != nullptr; p_entry = p_entry->p_next) {
                add_to_index(target, *p_entry, hash_of(*p_entry->p_key));
            }
        }
        else if (target.size > max_scanned_entries) {
            add_to_index(target, *p_value, hash);
        }

        return {p_value, true};
    }

    /// @brief Creates a flow scalar node.
    /// @param line Current line.
    /// @param indent Current indentation.
    /// @param lex_type Lexical token type for the scalar.
    /// @param tag_type Tag type for the scalar.
    /// @param token Scalar contents.
    /// @param is_verbatim Whether the contents are known to be the same as the token.
    /// @return The scalar node.
    flat_node make_flow_scalar(
        uint32_t line, uint32_t indent, lexical_token_t lex_type, tag_t tag_type, str_view token, bool is_verbatim) {
        const bool is_plain = lex_type == lexical_token_t::PLAIN_SCALAR;
        if FK_YAML_UNLIKELY (tag_type != tag_t::NONE) {
            validate_flow_scalar(line, indent, lex_type, tag_type, token, is_verbatim);
        }

        if (is_verbatim) {
            return make_typed_scalar(is_plain ? flat_node::PLAIN_SCALAR : flat_node::STRING_SCALAR, tag_type, token);
        }

        m_contents_parser.reset(line, indent);
        const str_view contents = m_contents_parser.parse_flow_contents(lex_type, token);
        return make_typed_scalar(
            is_plain ? flat_node::PLAIN_SCALAR : flat_node::STRING_SCALAR,
            tag_type,
            m_contents_parser.uses_owned_buffer() ? mp_storage->keep_chars(contents) : contents);
    }

    /// @brief Creates a block scalar node.
    /// @param line Current line.
    /// @param indent Current indentation.
    /// @param lex_type Lexical token type for the scalar.
    /// @param tag_type Tag type for the scalar.
    /// @param token Scalar contents.
    /// @param header Block scalar header information.
    /// @return The scalar node.
    flat_node make_block_scalar(
        uint32_t line, uint32_t indent, lexical_token_t lex_type, tag_t tag_type, str_view token,
        const block_scalar_header& header) {
        if FK_YAML_UNLIKELY (tag_type != tag_t::NONE) {
            validate_block_scalar(line, indent, lex_type, tag_type, token, header);
        }

        m_contents_parser.reset(line, indent);
        const str_view contents = m_contents_parser.parse_block_contents(lex_type, token, header);
        return make_typed_scalar(
            flat_node::STRING_SCALAR,
            tag_type,
            m_contents_parser.uses_owned_buffer() ? mp_storage->keep_chars(contents) : contents);
    }

    /// @brief Turns the given node into an empty string.
    /// @param node The target node.
    void make_empty_string(flat_node& node) const noexcept {
        node = make_scalar(flat_node::STRING_SCALAR, {});
    }

    /// @brief Turns the given node into an alias of the latest anchor with the given name, if there is any.
    /// @param node The target node.
    /// @param name The anchor name.
    /// @return true if the anchor is found, false otherwise.
    bool make_alias(flat_node& node, str_view name, const std::shared_ptr<metainfo_type>& /*unused*/) {
        const auto itr = m_anchors.find(name);
        if FK_YAML_UNLIKELY (itr == m_anchors.end()) {
            return false;
        }

        node.bits = flat_node::ALIAS;
        node.p_target = itr->second;
        mp_storage->ensure_props(node).anchor = name;
        return true;
    }

    /// @brief Gets what identifies the anchor of the given alias.
    /// @param alias The target alias node.
    /// @return The anchor identity.
    anchor_id get_anchor_id(const flat_node& alias) const noexcept {
        return alias.p_target;
    }

    /// @brief Checks whether the given node is the anchor with the given identity.
    /// @param node The target node.
    /// @param id The anchor identity.
    /// @return true if the node is the anchor, false otherwise.
    bool is_anchor_of(const flat_node& node, anchor_id id) const noexcept {
        return node.kind() == flat_node::ANCHOR && node.p_target == id;
    }

    /// @brief Sets an anchor name to the given node.
    /// @note The value moves to a node of its own, because the given one may well be a temporary object.
    /// @param node The target node.
    /// @param name The anchor name.
    void set_anchor(flat_node& node, str_view name, const std::shared_ptr<metainfo_type>& /*unused*/) {
        if (node.kind() != flat_node::ANCHOR) {
            flat_node* p_value = mp_storage->new_node(node);
            node = flat_node();
            node.bits = flat_node::ANCHOR;
            node.p_target = p_value;
        }

        mp_storage->ensure_props(node).anchor = name;
        m_anchors[name] = node.p_target;
    }

    /// @brief Sets a tag name to the given node.
    /// @note The tag goes along with the value, so that the aliases of an anchor see it as well. It says nothing about
    /// the type of a scalar any more since only a tag known before the scalar is parsed decides that.
    /// @param node The target node.
    /// @param name The tag name.
    void set_tag(flat_node& node, str_view name, tag_t /*unused*/) {
        mp_storage->ensure_props(node.resolve()).tag = name;
    }

    /// @brief Finishes a document.
    /// @param root The root node of the document.
    /// @param p_meta The metainfo of the document.
    void finish_document(flat_node& root, const std::shared_ptr<metainfo_type>& p_meta) {
        mp_storage->ensure_props(root).document = static_cast<uint32_t>(mp_storage->metainfos.size());
        mp_storage->metainfos.emplace_back(p_meta);
        m_anchors.clear();
        m_index.clear();
        m_num_indexed = 0;
    }

private:
    /// @brief An entry of the index of the keys of large mappings.
    struct index_entry {
        std::uint64_t hash;
        const flat_node* p_map;
        flat_node* p_value;
    };

    /// @brief Throws a type error, out of the way of the functions which detect it.
    [[noreturn]] FK_YAML_NOINLINE void throw_type_error(const char* msg, const flat_node& node) const {
        throw fkyaml::type_error(msg, mp_storage->type_of(node));
    }

    /// @brief Checks a tagged flow scalar, since a value which contradicts its tag is an error.
    /// @note Conversions on demand would come too late for that.
    FK_YAML_NOINLINE void validate_flow_scalar(
        uint32_t line, uint32_t indent, lexical_token_t lex_type, tag_t tag_type, str_view token,
        bool is_verbatim) const {
        scalar_parser<BasicNodeType>(line, indent).parse_flow(lex_type, tag_type, token, is_verbatim);
    }

    /// @brief Checks a tagged block scalar, since a value which contradicts its tag is an error.
    FK_YAML_NOINLINE void validate_block_scalar(
        uint32_t line, uint32_t indent, lexical_token_t lex_type, tag_t tag_type, str_view token,
        const block_scalar_header& header) const {
        scalar_parser<BasicNodeType>(line, indent).parse_block(lex_type, tag_type, token, header);
    }

    /// The largest number of entries of a mapping whose keys are compared one by one.
    static constexpr uint32_t max_scanned_entries = 8;

    static flat_node make_scalar(flat_node::kind_t kind, str_view contents) noexcept {
        flat_node node;
        node.bits = kind;
        node.p_str = contents.data();
        node.size = static_cast<uint32_t>(contents.size());
        return node;
    }

    flat_node make_typed_scalar(flat_node::kind_t kind, tag_t tag_type, str_view contents) {
        flat_node node = make_scalar(kind, contents);
        if FK_YAML_UNLIKELY (tag_type != tag_t::NONE) {
            mp_storage->ensure_props(node).tag_type = tag_type;
        }
        return node;
    }

    static flat_node* link(flat_node& collection, flat_node* p_entry) noexcept {
        if (collection.p_last != nullptr) {
            collection.p_last->p_next = p_entry;
        }
        else {
            collection.p_first = p_entry;
        }
        collection.p_last = p_entry;
        ++collection.size;
        return p_entry;
    }

    /// @brief Gets a hash which is the same for the keys that compare equal.
    std::uint64_t hash_of(const flat_node& key) const {
        const flat_node& value = key.resolve();
        if (value.bits == flat_node::STRING_SCALAR) {
            return hash_of_text(value.text());
        }

        switch (mp_storage->type_of(value)) {
        case fkyaml::node_type::STRING:
            return hash_of_text(value.text());
        case fkyaml::node_type::INTEGER: {
            std::int64_t integer = 0;
            return atoi(value.text().begin(), value.text().end(), integer) ? static_cast<std::uint64_t>(integer)
                                                                           : hash_of_text(value.text());
        }
        case fkyaml::node_type::SEQUENCE:
        case fkyaml::node_type::MAPPING:
            return value.size;
        default:
            return 0;
        }
    }

    // FNV-1a, which relies on the multiplication to wrap around.
    FK_YAML_NO_SANITIZE("unsigned-integer-overflow")
    static std::uint64_t hash_of_text(str_view text) noexcept {
        std::uint64_t hash = 14695981039346656037ull;
        for (const char c : text) {
            hash = (hash ^ static_cast<uint8_t>(c)) * 1099511628211ull;
        }
        return hash;
    }

    static std::uint64_t mix(const flat_node& map, std::uint64_t hash) noexcept {
        const std::uint64_t mixed = hash ^ reinterpret_cast<std::uintptr_t>(&map);
        return mixed ^ (mixed >> 29u);
    }

    /// @brief Finds the value of the given key.
    /// @param map The target mapping.
    /// @param key The key to look for.
    /// @param hash The hash of the key.
    /// @return The value of the key if the mapping has it, nullptr otherwise.
    flat_node* find(const flat_node& map, const flat_node& key, std::uint64_t hash) const {
        if (map.size <= max_scanned_entries) {
            for (flat_node* p_entry = map.p_first; p_entry != nullptr; p_entry = p_entry->p_next) {
                if (mp_storage->equals(*p_entry->p_key, key)) {
                    return p_entry;
                }
            }
            return nullptr;
        }

        const std::uint64_t mixed = mix(map, hash);
        const std::size_t mask = m_index.size() - 1;
        for (std::size_t pos = mixed & mask; m_index[pos].p_value != nullptr; pos = (pos + 1) & mask) {
            const index_entry& entry = m_index[pos];
            if (entry.hash == mixed && entry.p_map == &map && mp_storage->equals(*entry.p_value->p_key, key)) {
                return entry.p_value;
            }
        }
        return nullptr;
    }

    void add_to_index(const flat_node& map, flat_node& value, std::uint64_t hash) {
        if (m_index.empty()) {
            // Sized for the whole input right away, because growing it step by step is most of what it costs.
            std::size_t size = 64;
            while (size < mp_storage->source.size() / 32) {
                size *= 2;
            }
            m_index.assign(size, index_entry {0, nullptr, nullptr});
        }
        else if ((m_num_indexed + 1) * 4 > m_index.size() * 3) {
            std::vector<index_entry> old_index(
                m_index.size() < 64 ? 64 : m_index.size() * 2, index_entry {0, nullptr, nullptr});
            old_index.swap(m_index);
            for (const index_entry& entry : old_index) {
                if (entry.p_value != nullptr) {
                    place(entry);
                }
            }
        }

        place({mix(map, hash), &map, &value});
        ++m_num_indexed;
    }

    void place(const index_entry& entry) noexcept {
        const std::size_t mask = m_index.size() - 1;
        std::size_t pos = entry.hash & mask;
        while (m_index[pos].p_value != nullptr) {
            pos = (pos + 1) & mask;
        }
        m_index[pos] = entry;
    }

    /// The storage of the tree.
    flat_storage* mp_storage {nullptr};
    /// The parser of scalar contents, which keeps its buffer between scalars.
    scalar_contents_parser m_contents_parser {0, 0};
    /// The latest anchors of the current document by name.
    std::map<str_view, flat_node*> m_anchors;
    /// The index of the keys of large mappings, as an open addressing hash table of a power of 2 size.
    std::vector<index_entry> m_index;
    /// The number of entries in the index.
    std::size_t m_num_indexed {0};
};

FK_YAML_DETAIL_NAMESPACE_END

#endif /* FK_YAML_DETAIL_INPUT_FLAT_TREE_BUILDER_HPP */
