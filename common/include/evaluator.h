#pragma once
#include <cstdint>
#include <array>
#include <string>

uint32_t evaluate_raw(uint8_t* ranks, uint8_t* suits, uint8_t n);
uint32_t evaluate(std::array<uint8_t, 7>& cards);

inline uint8_t card_rank(uint8_t c) noexcept { return (uint8_t)(c / 4); }
inline uint8_t card_suit(uint8_t c) noexcept { return (uint8_t)(c % 4); }
inline uint8_t make_card(uint8_t rank, uint8_t suit) noexcept {return (uint8_t)(rank * 4 + suit); }

inline std::string card_string(uint8_t c) {
    static constexpr char kRanks[] = "23456789TJQKA";
    static constexpr char kSuits[] = "cdhs";
    return std::string{kRanks[card_rank(c)], kSuits[card_suit(c)]};
}
