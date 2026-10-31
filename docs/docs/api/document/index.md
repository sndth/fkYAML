<small>Defined in header [`<fkYAML/node.hpp>`](https://github.com/fktn-k/fkYAML/blob/develop/include/fkYAML/document.hpp)</small>

# <small>fkyaml::</small>document

```cpp
class document;
class document_node;
```

A read-only YAML document which parses several times faster than a tree of [`fkyaml::node`](../basic_node/node.md) objects.  
It comes from the same parsing code, so it accepts and rejects exactly the same inputs. The nodes live in one flat storage, strings are views into a copy of the input, and scalars are converted when they are asked for.

Use it when a YAML input only has to be read. Use [`fkyaml::node`](../basic_node/node.md) to modify or serialize the contents. A `document` converts to one with `to_node()`.

## Differences from `fkyaml::node`

* Nodes cannot be modified.
* Keys of a mapping stay in the order of the input.
* A key lookup compares the keys one by one, and an index lookup walks the entries from the beginning. Prefer iterating over a large collection to looking its entries up one at a time.
* Strings are returned as `document_node::string_view_type` views, which are valid as long as the document or a copy of it exists. `as<std::string>()` makes a copy.
* Keys are looked up by their text, so `doc["1"]` finds the key `1`.

## `document` member functions

| Name                                  | Description                                                                                       |
|---------------------------------------|---------------------------------------------------------------------------------------------------|
| `static parse(input)`                 | Parses the first document of an input of anything [`deserialize`](../basic_node/deserialize.md) accepts. |
| `static parse(begin, end)`            | Parses the first document of the input between two iterators.                                    |
| `static parse_docs(input)`            | Parses all the documents of an input into a `std::vector<document>`.                              |
| `root()`                              | Gets the root `document_node`.                                                                    |
| `operator[]`                          | Gets the value of a key of the root mapping or an entry of the root sequence.                     |
| `to_node<BasicNodeType = node>()`     | Converts the document to a `basic_node`, including its directives, anchors and aliases.           |

Copying a `document` is cheap since the copies share their storage.

## `document_node` member functions

| Name                                                                 | Description                                                                  |
|----------------------------------------------------------------------|------------------------------------------------------------------------------|
| `get_type()`                                                         | Gets the [`node_type`](../node_type.md) of the value.                        |
| `is_sequence()`, `is_mapping()`, `is_scalar()`, `is_null()`, `is_boolean()`, `is_integer()`, `is_float_number()`, `is_string()` | Check the type of the value. |
| `is_alias()`, `is_anchor()`, `has_anchor_name()`, `get_anchor_name()`, `has_tag_name()`, `get_tag_name()` | Give access to the node properties. |
| `size()`, `empty()`                                                  | Give the number of entries of a collection or the length of a string.        |
| `contains(key)`                                                      | Checks whether a mapping has a key.                                          |
| `operator[]`                                                         | Gets the value of a key of a mapping or an entry of a sequence.              |
| `begin()`, `end()`                                                   | Iterate over the entries of a sequence or the values of a mapping.           |
| `key()`                                                              | Gets the key of a mapping value, which is how to get keys while iterating.   |
| `as_str()`                                                           | Gets a view of the contents of a scalar without any conversion.              |
| `as<T>()`                                                            | Converts the value to any type which [`get_value`](../basic_node/get_value.md) can convert to. |
| `to_node<BasicNodeType = node>()`                                    | Converts this node and everything below it to a `basic_node`. Aliases of anchors outside of it become copies of their values. |

## Example

```cpp
#include <iostream>
#include <fkYAML/node.hpp>

int main() {
    const fkyaml::document doc = fkyaml::document::parse("name: fkYAML\n"
                                                         "versions: [0.5, 0.6]\n"
                                                         "stable: true\n");

    std::cout << doc["name"].as_str() << std::endl;
    std::cout << std::boolalpha << doc["stable"].as<bool>() << std::endl;

    for (const fkyaml::document_node version : doc["versions"]) {
        std::cout << version.as<double>() << std::endl;
    }

    for (const fkyaml::document_node value : doc.root()) {
        std::cout << value.key().as_str() << std::endl;
    }

    return 0;
}
```

```bash
fkYAML
true
0.5
0.6
name
versions
stable
```
