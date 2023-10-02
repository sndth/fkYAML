#ifndef FK_YAML_NODE_TYPE_HPP_
#define FK_YAML_NODE_TYPE_HPP_

#include <cstdint>

#include "fkYAML/VersioningMacros.hpp"

/**
 * @namespace fkyaml
 * @brief namespace for fkYAML library.
 */
FK_YAML_NAMESPACE_BEGIN

/**
 * @enum NodeType
 * @brief Definition of node value types.
 */
enum class NodeType : std::uint32_t
{
    SEQUENCE,     //!< sequence value type
    MAPPING,      //!< mapping value type
    NULL_OBJECT,  //!< null value type
    BOOLEAN,      //!< boolean value type
    INTEGER,      //!< integer value type
    FLOAT_NUMBER, //!< float number value type
    STRING,       //!< string value type
};

FK_YAML_NAMESPACE_END

#endif /* FK_YAML_NODE_TYPE_HPP_ */
