#pragma once
#include <cstdint>
#include <array>

uint32_t evaluate_raw(uint8_t* ranks, uint8_t* suits, uint8_t n);
uint32_t evaluate(std::array<uint8_t, 7>& cards);

inline uint8_t card_rank(uint8_t c) noexcept { return (uint8_t)(c / 4); }
inline uint8_t card_suit(uint8_t c) noexcept { return (uint8_t)(c % 4); }
