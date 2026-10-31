//  _______   __ __   __  _____   __  __  __
// |   __| |_/  |  \_/  |/  _  \ /  \/  \|  |     fkYAML: A C++ header-only YAML library
// |   __|  _  < \_   _/|  ___  |    _   |  |___  version 0.5.0
// |__|  |_| \__|  |_|  |_|   |_|___||___|______| https://github.com/fktn-k/fkYAML
//
// SPDX-FileCopyrightText: 2023-2026 Kensuke Fukutani <fktn.dev@gmail.com>
// SPDX-License-Identifier: MIT

#ifndef FK_YAML_DOCUMENT_HPP
#define FK_YAML_DOCUMENT_HPP

#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <fkYAML/detail/macros/define_macros.hpp>
#include <fkYAML/node.hpp>
#include <fkYAML/detail/input/flat_tree_builder.hpp>

FK_YAML_NAMESPACE_BEGIN

/// @brief A read-only view of a node of a document.
/// @note It is valid as long as its document or a copy of the document exists.
class document_node {
public:
    /// A type for the views of strings.
    using string_view_type = detail::str_view;

    /// @brief A forward iterator over the entries of a sequence or the values of a mapping.
    class const_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = document_node;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = document_node;

        const_iterator() = default;

        const_iterator(const detail::flat_storage* p_storage, const detail::flat_node* p_node) noexcept
            : mp_storage(p_storage),
              mp_node(p_node) {
        }

        document_node operator*() const noexcept {
            return {mp_storage, mp_node};
        }

        const_iterator& operator++() noexcept {
            mp_node = mp_node->p_next;
            return *this;
        }

        const_iterator operator++(int) noexcept {
            const_iterator old = *this;
            ++(*this);
            return old;
        }

        bool operator==(const const_iterator& rhs) const noexcept {
            return mp_node == rhs.mp_node;
        }

        bool operator!=(const const_iterator& rhs) const noexcept {
            return mp_node != rhs.mp_node;
        }

    private:
        const detail::flat_storage* mp_storage {nullptr};
        const detail::flat_node* mp_node {nullptr};
    };

    document_node() = default;

    /// @brief Constructs a view of a node.
    /// @param p_storage The storage of the node.
    /// @param p_node The node.
    document_node(const detail::flat_storage* p_storage, const detail::flat_node* p_node) noexcept
        : mp_storage(p_storage),
          mp_node(p_node) {
    }

    /// @brief Gets the type of the value of this node.
    node_type get_type() const {
        return mp_storage->type_of(*mp_node);
    }

    bool is_sequence() const noexcept {
        return mp_node->is_sequence();
    }

    bool is_mapping() const noexcept {
        return mp_node->is_mapping();
    }

    bool is_scalar() const noexcept {
        return mp_node->is_scalar();
    }

    bool is_null() const {
        return get_type() == node_type::NULL_OBJECT;
    }

    bool is_boolean() const {
        return get_type() == node_type::BOOLEAN;
    }

    bool is_integer() const {
        return get_type() == node_type::INTEGER;
    }

    bool is_float_number() const {
        return get_type() == node_type::FLOAT;
    }

    bool is_string() const {
        return get_type() == node_type::STRING;
    }

    bool is_alias() const noexcept {
        return mp_node->is_alias();
    }

    bool is_anchor() const noexcept {
        return mp_node->kind() == detail::flat_node::ANCHOR;
    }

    bool has_anchor_name() const noexcept {
        return mp_node->kind() >= detail::flat_node::ANCHOR;
    }

    /// @brief Gets the anchor name of this anchor or alias.
    /// @return The anchor name, which is empty if this node has none.
    string_view_type get_anchor_name() const noexcept {
        return mp_storage->get_props(*mp_node).anchor;
    }

    bool has_tag_name() const noexcept {
        return !get_tag_name().empty();
    }

    /// @brief Gets the tag name of this node.
    /// @return The tag name, which is empty if this node has none.
    string_view_type get_tag_name() const noexcept {
        return mp_storage->get_props(mp_node->resolve()).tag;
    }

    /// @brief Gets the number of the entries of a collection or the length of a string.
    /// @return The size of this node.
    std::size_t size() const {
        const detail::flat_node& value = mp_node->resolve();
        if FK_YAML_UNLIKELY (value.kind() == detail::flat_node::NULL_SCALAR || !(is_collection() || is_string())) {
            throw type_error("The target node is not of a type which has a size.", get_type());
        }
        return value.size;
    }

    /// @brief Checks whether a collection or a string is empty.
    bool empty() const {
        return size() == 0;
    }

    /// @brief Checks whether this mapping has the given key.
    /// @param key The string form of the key.
    /// @return true if this node is a mapping with the given key, false otherwise.
    bool contains(string_view_type key) const noexcept {
        return find(key) != nullptr;
    }

    /// @brief Gets the value of the given key of this mapping.
    /// @note The keys are compared one by one, as they are in the input.
    /// @param key The string form of the key.
    /// @return The value of the key.
    document_node operator[](string_view_type key) const {
        if FK_YAML_UNLIKELY (!mp_node->is_mapping()) {
            throw type_error("A key can only be looked up in a mapping.", get_type());
        }
        const detail::flat_node* p_value = find(key);
        if FK_YAML_UNLIKELY (p_value == nullptr) {
            throw exception("The given key is not found in the mapping.");
        }
        return {mp_storage, p_value};
    }

    /// @brief Gets the value of the given key of this mapping.
    /// @param key The string form of the key.
    /// @return The value of the key.
    document_node operator[](const char* key) const {
        return (*this)[string_view_type(key)];
    }

    /// @brief Gets the value of the given key of this mapping.
    /// @param key The string form of the key.
    /// @return The value of the key.
    document_node operator[](const std::string& key) const {
        return (*this)[string_view_type(key)];
    }

    /// @brief Gets the entry at the given index of this sequence.
    /// @note The entries are linked, which makes this linear in the index. Prefer iterating over all of them.
    /// @tparam IndexType An integer type.
    /// @param index The index of the entry.
    /// @return The entry at the index.
    template <typename IndexType, detail::enable_if_t<detail::is_non_bool_integral<IndexType>::value, int> = 0>
    document_node operator[](IndexType index) const {
        if FK_YAML_UNLIKELY (!mp_node->is_sequence()) {
            throw type_error("An index can only be used with a sequence.", get_type());
        }
        const detail::flat_node& seq = mp_node->resolve();
        const auto position = static_cast<std::size_t>(index);
        if FK_YAML_UNLIKELY (index < IndexType(0) || position >= seq.size) {
            throw exception("The given index is out of the range of the sequence.");
        }

        const detail::flat_node* p_entry = seq.p_first;
        for (std::size_t i = 0; i < position; ++i) {
            p_entry = p_entry->p_next;
        }
        return {mp_storage, p_entry};
    }

    /// @brief Gets an iterator to the first entry of this collection.
    const_iterator begin() const noexcept {
        const detail::flat_node& value = mp_node->resolve();
        return {mp_storage, is_collection() ? value.p_first : nullptr};
    }

    /// @brief Gets an iterator past the last entry of this collection.
    const_iterator end() const noexcept {
        return {mp_storage, nullptr};
    }

    /// @brief Gets the key of this mapping value.
    /// @return The key, which only makes sense to ask for while iterating over a mapping.
    document_node key() const {
        if FK_YAML_UNLIKELY (mp_node->p_key == nullptr) {
            throw exception("The target node is not a mapping value.");
        }
        return {mp_storage, mp_node->p_key};
    }

    /// @brief Gets the contents of this scalar as they are after unescaping, without any conversion.
    /// @return The view of the contents, which lives as long as the document does.
    string_view_type as_str() const {
        if FK_YAML_UNLIKELY (!mp_node->is_scalar()) {
            throw type_error("The target node is not a scalar.", get_type());
        }
        return mp_node->resolve().text();
    }

    /// @brief Converts the value of this node to the given type.
    /// @note Whatever can be converted from a basic_node object can be converted from a document_node object.
    /// @tparam T The target type.
    /// @return The converted value.
    template <typename T>
    T as() const {
        T value {};
        if (!convert(value)) {
            value = to_node().template get_value<T>();
        }
        return value;
    }

    /// @brief Converts this node and everything below it to a basic_node object.
    /// @note Aliases of anchors outside of this node become copies of their values, and the directives of the document
    /// are left behind. Use document::to_node() to keep them.
    /// @tparam BasicNodeType The target node type.
    /// @return The converted basic_node object.
    template <typename BasicNodeType = fkyaml::node>
    BasicNodeType to_node() const {
        using builder_type = detail::node_tree_builder<BasicNodeType>;
        builder_type builder {};
        auto p_meta = std::make_shared<typename builder_type::metainfo_type>();
        BasicNodeType root = convert_node(builder, *mp_node, p_meta, false);
        builder.finish_document(root, p_meta);
        return root;
    }

private:
    friend class document;

    bool is_collection() const noexcept {
        return mp_node->is_sequence() || mp_node->is_mapping();
    }

    const detail::flat_node* find(string_view_type key) const noexcept {
        const detail::flat_node& map = mp_node->resolve();
        if (map.kind() != detail::flat_node::MAPPING) {
            return nullptr;
        }
        for (const detail::flat_node* p_value = map.p_first; p_value != nullptr; p_value = p_value->p_next) {
            const detail::flat_node& entry_key = p_value->p_key->resolve();
            if (entry_key.kind() < detail::flat_node::SEQUENCE && entry_key.text() == key) {
                return p_value;
            }
        }
        return nullptr;
    }

    bool convert(bool& value) const {
        const string_view_type text = mp_node->resolve().text();
        return mp_node->is_scalar() && is_boolean() && detail::atob(text.begin(), text.end(), value);
    }

    bool convert(std::string& value) const {
        if (!mp_node->is_scalar() || !is_string()) {
            return false;
        }
        const string_view_type text = mp_node->resolve().text();
        value.assign(text.begin(), text.end());
        return true;
    }

    template <typename T, detail::enable_if_t<detail::is_non_bool_integral<T>::value, int> = 0>
    bool convert(T& value) const {
        const string_view_type text = mp_node->resolve().text();
        return mp_node->is_scalar() && is_integer() && detail::atoi(text.begin(), text.end(), value);
    }

    template <typename T, detail::enable_if_t<std::is_floating_point<T>::value, int> = 0>
    bool convert(T& value) const {
        const string_view_type text = mp_node->resolve().text();
        return mp_node->is_scalar() && is_float_number() && detail::atof(text.begin(), text.end(), value);
    }

    template <
        typename T, detail::enable_if_t<!std::is_arithmetic<T>::value && !std::is_same<T, std::string>::value, int> = 0>
    bool convert(T& /*unused*/) const noexcept {
        return false;
    }

    template <typename BuilderType, typename MetaType>
    typename BuilderType::node_type convert_node(
        BuilderType& builder, const detail::flat_node& node, const std::shared_ptr<MetaType>& p_meta,
        bool has_directives) const {
        using node_t = typename BuilderType::node_type;

        const detail::flat_props& props = mp_storage->get_props(node);
        if (node.is_alias()) {
            node_t alias;
            if (builder.make_alias(alias, props.anchor, p_meta)) {
                return alias;
            }
            // The anchor is outside of the part of the document which is being converted.
            return convert_node(builder, node.resolve(), p_meta, has_directives);
        }

        const detail::flat_node& value = node.resolve();
        const detail::flat_props& value_props = mp_storage->get_props(value);

        node_t result;
        switch (value.kind()) {
        case detail::flat_node::SEQUENCE:
            result = node_t::sequence();
            break;
        case detail::flat_node::MAPPING:
            result = node_t::mapping();
            break;
        case detail::flat_node::NULL_SCALAR:
            break;
        default: {
            const auto lex_type = value.kind() == detail::flat_node::PLAIN_SCALAR
                                      ? detail::lexical_token_t::PLAIN_SCALAR
                                      : detail::lexical_token_t::DOUBLE_QUOTED_SCALAR;
            result =
                detail::scalar_parser<node_t>(0, 0).create_from_contents(lex_type, value_props.tag_type, value.text());
            break;
        }
        }

        if (has_directives) {
            builder.set_metainfo(result, p_meta);
        }

        // The same order as in parsing, so that anchors which share their names resolve to the same nodes.
        if (node.kind() == detail::flat_node::ANCHOR) {
            builder.set_anchor(result, props.anchor, p_meta);
        }
        if (!value_props.tag.empty()) {
            builder.set_tag(result, value_props.tag, value_props.tag_type);
        }

        if (value.kind() == detail::flat_node::SEQUENCE) {
            for (const detail::flat_node* p_entry = value.p_first; p_entry != nullptr; p_entry = p_entry->p_next) {
                builder.append(result, convert_node(builder, *p_entry, p_meta, has_directives));
            }
        }
        else if (value.kind() == detail::flat_node::MAPPING) {
            for (const detail::flat_node* p_entry = value.p_first; p_entry != nullptr; p_entry = p_entry->p_next) {
                node_t key = convert_node(builder, *p_entry->p_key, p_meta, has_directives);
                node_t* p_value = builder.insert(result, std::move(key)).first;
                *p_value = convert_node(builder, *p_entry, p_meta, has_directives);
            }
        }

        return result;
    }

    const detail::flat_storage* mp_storage {nullptr};
    const detail::flat_node* mp_node {nullptr};
};

/// @brief A YAML document which is much faster to parse than a tree of basic_node objects.
/// @note It is read-only, its strings are views and its scalars are converted when asked to. Use to_node() for a
/// basic_node object, or fkyaml::node::deserialize() for one in the first place, to modify it or to serialize it.
class document {
public:
    /// @brief Parses the first YAML document of the given input.
    /// @note The input may be anything fkyaml::node::deserialize() accepts. Its contents are copied.
    /// @tparam InputType The type of the input.
    /// @param input The input.
    /// @return The parsed document.
    template <typename InputType>
    static document parse(InputType&& input) {
        auto adapter = detail::input_adapter(std::forward<InputType>(input));
        auto p_storage = std::make_shared<detail::flat_storage>(adapter.get_buffer_view());
        deserializer_type deserializer {detail::flat_tree_builder(p_storage.get())};
        const detail::flat_node root = deserializer.deserialize(view_adapter {p_storage->source});
        return {p_storage, p_storage->new_node(root)};
    }

    /// @brief Parses the first YAML document of the input between the given iterators.
    /// @tparam ItrType The type of the iterators.
    /// @param begin The beginning of the input.
    /// @param end The end of the input.
    /// @return The parsed document.
    template <typename ItrType>
    static document parse(ItrType begin, ItrType end) {
        auto adapter = detail::input_adapter(begin, end);
        auto p_storage = std::make_shared<detail::flat_storage>(adapter.get_buffer_view());
        deserializer_type deserializer {detail::flat_tree_builder(p_storage.get())};
        const detail::flat_node root = deserializer.deserialize(view_adapter {p_storage->source});
        return {p_storage, p_storage->new_node(root)};
    }

    /// @brief Parses all the YAML documents of the given input.
    /// @tparam InputType The type of the input.
    /// @param input The input.
    /// @return The parsed documents.
    template <typename InputType>
    static std::vector<document> parse_docs(InputType&& input) {
        auto adapter = detail::input_adapter(std::forward<InputType>(input));
        auto p_storage = std::make_shared<detail::flat_storage>(adapter.get_buffer_view());
        deserializer_type deserializer {detail::flat_tree_builder(p_storage.get())};

        std::vector<document> docs {};
        for (const detail::flat_node& root : deserializer.deserialize_docs(view_adapter {p_storage->source})) {
            docs.emplace_back(document(p_storage, p_storage->new_node(root)));
        }
        return docs;
    }

    /// @brief Gets the root node.
    document_node root() const noexcept {
        return {mp_storage.get(), mp_root};
    }

    /// @brief Gets the value of the given key of the root mapping.
    /// @param key The string form of the key.
    /// @return The value of the key.
    document_node operator[](document_node::string_view_type key) const {
        return root()[key];
    }

    /// @brief Gets the value of the given key of the root mapping.
    /// @param key The string form of the key.
    /// @return The value of the key.
    document_node operator[](const char* key) const {
        return root()[key];
    }

    /// @brief Gets the value of the given key of the root mapping.
    /// @param key The string form of the key.
    /// @return The value of the key.
    document_node operator[](const std::string& key) const {
        return root()[key];
    }

    /// @brief Gets the entry at the given index of the root sequence.
    /// @tparam IndexType An integer type.
    /// @param index The index of the entry.
    /// @return The entry at the index.
    template <typename IndexType, detail::enable_if_t<detail::is_non_bool_integral<IndexType>::value, int> = 0>
    document_node operator[](IndexType index) const {
        return root()[index];
    }

    /// @brief Converts this document to a basic_node object, including its directives.
    /// @tparam BasicNodeType The target node type.
    /// @return The converted basic_node object.
    template <typename BasicNodeType = fkyaml::node>
    BasicNodeType to_node() const {
        using builder_type = detail::node_tree_builder<BasicNodeType>;
        using meta_type = typename builder_type::metainfo_type;

        const auto& p_source_meta = mp_storage->metainfos[mp_storage->get_props(*mp_root).document];
        auto p_meta = std::make_shared<meta_type>();
        p_meta->version = p_source_meta->version;
        p_meta->is_version_specified = p_source_meta->is_version_specified;
        p_meta->primary_handle_prefix = p_source_meta->primary_handle_prefix;
        p_meta->secondary_handle_prefix = p_source_meta->secondary_handle_prefix;
        p_meta->named_handle_map = p_source_meta->named_handle_map;

        builder_type builder {};
        const document_node root_node = root();
        const bool has_directives = p_meta->is_version_specified || !p_meta->primary_handle_prefix.empty() ||
                                    !p_meta->secondary_handle_prefix.empty() || !p_meta->named_handle_map.empty();
        BasicNodeType root_copy = root_node.convert_node(builder, *mp_root, p_meta, has_directives);
        builder.finish_document(root_copy, p_meta);
        return root_copy;
    }

private:
    using deserializer_type = detail::basic_deserializer<detail::flat_node, detail::flat_tree_builder>;

    /// @brief An input adapter for an input which is ready to be parsed.
    struct view_adapter {
        detail::str_view get_buffer_view() const noexcept {
            return buffer;
        }

        detail::str_view buffer;
    };

    document(std::shared_ptr<const detail::flat_storage> p_storage, const detail::flat_node* p_root) noexcept
        : mp_storage(std::move(p_storage)),
          mp_root(p_root) {
    }

    /// The storage of the tree, which may be shared with the other documents of the same input.
    std::shared_ptr<const detail::flat_storage> mp_storage;
    /// The root node.
    const detail::flat_node* mp_root;
};

FK_YAML_NAMESPACE_END

#endif /* FK_YAML_DOCUMENT_HPP */
