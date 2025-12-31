#pragma once
#include <cstdint>
#include <string>
#include <vector>

using std::vector;
using std::string;

//Note: I did not write this class, that was bobby

uint32_t eval_cards(const vector<string>& cards);

uint32_t evaluate_raw(uint8_t* ranks, uint8_t* suits, uint8_t n);
