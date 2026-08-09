#pragma once
#include <cstdint>     
#include <string>      
#include <vector>       
#include <utility>     
#include <fstream>     
#include <filesystem>   
#include <stdexcept>   
#include <ios>  
#include <type_traits>       
#include <format>

/// @brief Header used to store information for storing and retrieving matrices
struct MatrixHeader{
    uint64_t num_rows; 
    uint64_t num_cols;
    uint64_t bytes_per_elt;

    bool is_signed;
    bool is_float;
    bool operator==(const MatrixHeader&) const = default;

    std::string to_string() const {
        std::string desc = "MatrixHeader{num_rows=" +
            std::to_string(num_rows)+ 
            ", num_cols=" + 
            std::to_string(num_cols)+
            ", bytes_per_elt=" + 
            std::to_string(bytes_per_elt) +
            ", is_float=" + 
            std::format("{}", is_float ) +
            ", is_signed=" + 
            std::format("{}", is_signed)+
            "}";

        return desc;
    }
};

template <typename T>
void header_type_check(MatrixHeader header){

    if (!std::is_arithmetic_v<T>){
        throw std::runtime_error("Matrix loader does not support non-arithmetic types");
    }

    if (header.bytes_per_elt != sizeof(T)){
        throw std::runtime_error("Header bytes_per_elt=" +
            std::to_string(header.bytes_per_elt) + 
            " and requested type bytes_per_elt=" +
            std::to_string(sizeof(T)) + 
            " do not match");
    }

    if (header.is_float != std::is_floating_point_v<T>){
        throw std::runtime_error("Header is_float=" +
            std::format("{}", header.is_float) +
            " and requested type is_float=" +
            std::format("{}", std::is_floating_point_v<T>) + 
            " do not match");
    } 

    if (header.is_signed != std::is_signed_v<T>){
        throw std::runtime_error("Header is_signed=" +
            std::format("{}", header.is_signed) +
            " and requested type is_signed=" +
            std::format("{}", std::is_signed_v<T>) + 
            " do not match");
    }
}

template <typename T>
std::pair<std::vector<T>, MatrixHeader> load_matrix_and_header(const std::string& result_path) {
    //check against the header and throw an error if something goes wrong
    std::ifstream in(result_path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open " + result_path);

    MatrixHeader header;
    header_type_check<T>(header);
    in.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (in.gcount() != static_cast<std::streamsize>(sizeof(header))) throw std::runtime_error("missing header");



    uint64_t expected_bytes = header.num_rows * header.num_cols * header.bytes_per_elt;
    std::streampos here = in.tellg(); in.seekg(0, std::ios::end);
    std::streampos end = in.tellg(); in.seekg(here);

    uint64_t file_bytes = static_cast<uint64_t>(end - here);
    if (file_bytes != expected_bytes){
        throw std::runtime_error("Header expected bytes=" +
        std::to_string(expected_bytes)+
        " and recieved bytes="+
        std::to_string(file_bytes) + 
        " do not match");
    }

    std::vector<T> results(expected_bytes / sizeof(T));
    in.read(reinterpret_cast<char*>(results.data()), static_cast<std::streamsize>(expected_bytes));
    
    uint64_t read_bytes = static_cast<uint64_t>(in.gcount());
    if (read_bytes != expected_bytes){
        throw std::runtime_error("Header expected bytes=" +
        std::to_string(expected_bytes)+
        " and read_bytes="+
        std::to_string(read_bytes) + 
        " do not match");
    }

    return {std::move(results), header};
}

template <typename T>
inline void write_matrix_and_header(const std::string& write_path, MatrixHeader header, const std::vector<T>& results) {
    //check against the header and throw an error if something goes wrong
    header_type_check<T>(header);

    if (std::filesystem::exists(write_path)){
        throw std::runtime_error("write path already exists");
    }

    uint64_t expected_num_elts = header.num_rows * header.num_cols;

    if (expected_num_elts != static_cast<uint64_t>(results.size())){
        throw std::runtime_error("Expected num elts="+
        std::to_string(expected_num_elts)+
        "and recieved num elts"+
        std::to_string(results.size())+
        " do not match");
    }

    std::ofstream out(write_path, std::ios::binary);
    if (!out) throw std::runtime_error("Can not open the path: " + write_path);

    out.write(reinterpret_cast<const char*>(&header), sizeof(header));
    out.write(reinterpret_cast<const char*>(results.data()),
              static_cast<std::streamsize>(results.size() * sizeof(T)));
    if (!out) throw std::runtime_error("Failed while writing to path: : " + write_path);
}