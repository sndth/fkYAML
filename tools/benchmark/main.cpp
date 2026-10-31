//  _______   __ __   __  _____   __  __  __
// |   __| |_/  |  \_/  |/  _  \ /  \/  \|  |     fkYAML: A C++ header-only YAML library (supporting code)
// |   __|  _  < \_   _/|  ___  |    _   |  |___  version 0.5.0
// |__|  |_| \__|  |_|  |_|   |_|___||___|______| https://github.com/fktn-k/fkYAML
//
// SPDX-FileCopyrightText: 2023-2026 Kensuke Fukutani <fktn.dev@gmail.com>
// SPDX-License-Identifier: MIT

#ifdef _MSC_VER
// suppress the C4996 warning against the usage of fopen().
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cassert>
#include <cstring>
#include <string>

#include <benchmark/benchmark.h>

#include <fkYAML/node.hpp>
#include <yaml-cpp/yaml.h>

#include <libfyaml.h>

#include <ryml.hpp>
#include <ryml_std.hpp>
#include <c4/yml/parse.hpp>

static std::string test_src {};

void prepare_test_source(char* filename) {
    FILE* fp = std::fopen(filename, "rb");
    char tmp_buf[256] {};
    std::size_t buf_size = sizeof(tmp_buf) / sizeof(char);
    std::size_t read_size = 0;
    while ((read_size = std::fread(&tmp_buf[0], sizeof(char), buf_size, fp)) > 0) {
        test_src.append(tmp_buf, tmp_buf + read_size);
    }
    std::fclose(fp);
    fp = nullptr;
}

int main(int argc, char** argv) {
    prepare_test_source(argv[1]);

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();

    return 0;
}

//
// Benchmarking functions.
//

// fkYAML
void bm_fkyaml_parse(benchmark::State& st) {
    for (auto _ : st) {
        fkyaml::node n = fkyaml::node::deserialize(test_src);
    }

    st.SetItemsProcessed(st.iterations());
    st.SetBytesProcessed(st.iterations() * test_src.size());
}

static std::size_t read_all_values(const fkyaml::node& n) {
    std::size_t sum = 0;
    if (n.is_mapping()) {
        for (const auto& entry : n.as_map()) {
            sum += read_all_values(entry.first) + read_all_values(entry.second);
        }
    }
    else if (n.is_sequence()) {
        for (const auto& entry : n.as_seq()) {
            sum += read_all_values(entry);
        }
    }
    else if (n.is_string()) {
        sum += n.as_str().size();
    }
    else if (n.is_integer()) {
        sum += static_cast<std::size_t>(n.as_int());
    }
    else if (n.is_float_number()) {
        sum += static_cast<std::size_t>(n.as_float());
    }
    return sum;
}

// fkYAML (every value is read after parsing, since rapidyaml converts values on access)
void bm_fkyaml_parse_and_read(benchmark::State& st) {
    for (auto _ : st) {
        fkyaml::node n = fkyaml::node::deserialize(test_src);
        benchmark::DoNotOptimize(read_all_values(n));
    }

    st.SetItemsProcessed(st.iterations());
    st.SetBytesProcessed(st.iterations() * test_src.size());
}

// fkYAML (document)
void bm_fkyaml_document_parse(benchmark::State& st) {
    for (auto _ : st) {
        fkyaml::document doc = fkyaml::document::parse(test_src);
        benchmark::DoNotOptimize(doc.root().is_mapping());
    }

    st.SetItemsProcessed(st.iterations());
    st.SetBytesProcessed(st.iterations() * test_src.size());
}

static std::size_t read_all_values(const fkyaml::document_node& n) {
    std::size_t sum = 0;
    if (n.is_sequence() || n.is_mapping()) {
        for (const fkyaml::document_node child : n) {
            if (n.is_mapping()) {
                sum += read_all_values(child.key());
            }
            sum += read_all_values(child);
        }
        return sum;
    }

    switch (n.get_type()) {
    case fkyaml::node_type::STRING:
        return n.as_str().size();
    case fkyaml::node_type::INTEGER:
        return static_cast<std::size_t>(n.as<std::int64_t>());
    case fkyaml::node_type::FLOAT:
        return static_cast<std::size_t>(n.as<double>());
    default:
        return 0;
    }
}

// fkYAML (document, with every value read and converted after parsing)
void bm_fkyaml_document_parse_and_read(benchmark::State& st) {
    for (auto _ : st) {
        fkyaml::document doc = fkyaml::document::parse(test_src);
        benchmark::DoNotOptimize(read_all_values(doc.root()));
    }

    st.SetItemsProcessed(st.iterations());
    st.SetBytesProcessed(st.iterations() * test_src.size());
}

// yaml-cpp
void bm_yamlcpp_parse(benchmark::State& st) {
    for (auto _ : st) {
        YAML::Node n = YAML::Load(test_src);
    }

    st.SetItemsProcessed(st.iterations());
    st.SetBytesProcessed(st.iterations() * test_src.size());
}

// libfyaml
void bm_libfyaml_parse(benchmark::State& st) {
    const char* p_test_src = test_src.c_str();
    std::size_t test_src_size = std::distance(test_src.begin(), test_src.end());

    for (auto _ : st) {
        fy_document* p_fyd = fy_document_build_from_string(nullptr, p_test_src, test_src_size);
        (void)p_fyd;
    }

    st.SetItemsProcessed(st.iterations());
    st.SetBytesProcessed(st.iterations() * test_src.size());
}

// rapidyaml (in place)
void bm_rapidyaml_parse_inplace(benchmark::State& st) {
    // Initialize from the source so the view below spans the real contents. Constructing the buffer
    // filled with '\0' instead would make trimr('\0') strip all of it, leaving an empty view and
    // making the benchmark parse nothing.
    std::string in_place_buff(test_src);
    c4::substr c4_test_src = c4::to_substr(in_place_buff).trimr('\0');

    for (auto _ : st) {
        // ryml::parse_in_place() modifies the contents of `in_place_buff` during parsing.
        // Without the following copy, the second (and subsequent) parsing would fail.
        // The timer is paused around the copy so that only the parsing is measured.
        st.PauseTiming();
        assert(in_place_buff.size() == test_src.size());
        std::memcpy(&in_place_buff[0], &test_src[0], in_place_buff.size());
        st.ResumeTiming();

        ryml::Tree tree = ryml::parse_in_place(c4_test_src);
    }

    st.SetItemsProcessed(st.iterations());
    st.SetBytesProcessed(st.iterations() * test_src.size());
}

// rapidyaml (arena)
void bm_rapidyaml_parse_arena(benchmark::State& st) {
    c4::csubstr src = c4::to_csubstr(test_src).trimr('\0');

    for (auto _ : st) {
        ryml::Tree tree = ryml::parse_in_arena(src);
    }

    st.SetItemsProcessed(st.iterations());
    st.SetBytesProcessed(st.iterations() * test_src.size());
}

// rapidyaml (arena, with the tree reserved up front, which is how it is meant to be used for speed)
void bm_rapidyaml_parse_arena_reserved(benchmark::State& st) {
    c4::csubstr src = c4::to_csubstr(test_src).trimr('\0');
    const std::size_t num_nodes = ryml::parse_in_arena(src).size();

    for (auto _ : st) {
        ryml::Tree tree;
        tree.reserve(num_nodes + 16);
        tree.reserve_arena(test_src.size() * 2);
        ryml::parse_in_arena(src, &tree);
    }

    st.SetItemsProcessed(st.iterations());
    st.SetBytesProcessed(st.iterations() * test_src.size());
}

static std::size_t read_all_values(ryml::ConstNodeRef n) {
    std::size_t sum = 0;
    if (n.has_key()) {
        sum += n.key().len;
    }
    if (n.has_val()) {
        const c4::csubstr val = n.val();
        long long integer = 0;
        double float_val = 0;
        if (val.is_integer() && c4::atox(val, &integer)) {
            sum += static_cast<std::size_t>(integer);
        }
        else if (val.is_number() && c4::atod(val, &float_val)) {
            sum += static_cast<std::size_t>(float_val);
        }
        else {
            sum += val.len;
        }
    }
    for (ryml::ConstNodeRef child : n.children()) {
        sum += read_all_values(child);
    }
    return sum;
}

// rapidyaml (arena, with the tree reserved up front and every value read after parsing)
void bm_rapidyaml_parse_and_read(benchmark::State& st) {
    c4::csubstr src = c4::to_csubstr(test_src).trimr('\0');
    const std::size_t num_nodes = ryml::parse_in_arena(src).size();

    for (auto _ : st) {
        ryml::Tree tree;
        tree.reserve(num_nodes + 16);
        tree.reserve_arena(test_src.size() * 2);
        ryml::parse_in_arena(src, &tree);
        benchmark::DoNotOptimize(read_all_values(tree.rootref()));
    }

    st.SetItemsProcessed(st.iterations());
    st.SetBytesProcessed(st.iterations() * test_src.size());
}

// Register benchmarking functions.
BENCHMARK(bm_fkyaml_parse);
BENCHMARK(bm_fkyaml_parse_and_read);
BENCHMARK(bm_fkyaml_document_parse);
BENCHMARK(bm_fkyaml_document_parse_and_read);
BENCHMARK(bm_yamlcpp_parse);

BENCHMARK(bm_libfyaml_parse);

BENCHMARK(bm_rapidyaml_parse_inplace);
BENCHMARK(bm_rapidyaml_parse_arena);
BENCHMARK(bm_rapidyaml_parse_arena_reserved);
BENCHMARK(bm_rapidyaml_parse_and_read);
