//  _______   __ __   __  _____   __  __  __
// |   __| |_/  |  \_/  |/  _  \ /  \/  \|  |     fkYAML: A C++ header-only YAML library
// |   __|  _  < \_   _/|  ___  |    _   |  |___  version 0.5.0
// |__|  |_| \__|  |_|  |_|   |_|___||___|______| https://github.com/fktn-k/fkYAML
//
// SPDX-FileCopyrightText: 2023-2026 Kensuke Fukutani <fktn.dev@gmail.com>
// SPDX-License-Identifier: MIT

#ifndef FK_YAML_DETAIL_NODE_PROPERTY_HPP
#define FK_YAML_DETAIL_NODE_PROPERTY_HPP

#include <memory>
#include <string>

#include <fkYAML/detail/macros/define_macros.hpp>
#include <fkYAML/detail/document_metainfo.hpp>

FK_YAML_DETAIL_NAMESPACE_BEGIN

struct node_property {
    /// The tag name property.
    std::string tag {}; // NOLINT(readability-redundant-member-init) necessary for older compilers
    /// The anchor name property.
    std::string anchor {}; // NOLINT(readability-redundant-member-init) necessary for older compilers
};

/// @brief What only few nodes need, kept out of them so that all the others stay small and cheap to move.
/// @tparam BasicNodeType A type of the container for YAML values.
template <typename BasicNodeType>
struct node_extras {
    /// The shared set of YAML directives and anchors of the document of a node.
    std::shared_ptr<document_metainfo<BasicNodeType>> p_meta {};
    /// The anchor and tag names of a node.
    node_property prop {};
};

FK_YAML_DETAIL_NAMESPACE_END

#endif /* FK_YAML_DETAIL_NODE_PROPERTY_HPP */
