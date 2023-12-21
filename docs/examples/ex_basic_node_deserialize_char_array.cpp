#include <cstdint>
#include <iomanip>
#include <iostream>
#include <fkYAML/node.hpp>

int main()
{
    // deserialize a YAML string.
    char input[] = R"(
    foo: true
    bar: 123
    baz: 3.14
    )";
    fkyaml::node n = fkyaml::node::deserialize(input);

    // check the deserialization result.
    std::cout << n["foo"].get_value<bool>() << std::endl;
    std::cout << n["bar"].get_value<std::int64_t>() << std::endl;
    std::cout << std::setprecision(3) << n["baz"].get_value<double>() << std::endl;

    return 0;
}
