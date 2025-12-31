#include "TexasHoldEm/HandEval.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <array>

using namespace std;


#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>
#include <cctype>

static uint8_t parse_rank(char rc) {
    rc = (char)std::toupper((unsigned char)rc);
    switch (rc) {
        case '2': return 0;
        case '3': return 1;
        case '4': return 2;
        case '5': return 3;
        case '6': return 4;
        case '7': return 5;
        case '8': return 6;
        case '9': return 7;
        case 'T': return 8;
        case 'J': return 9;
        case 'Q': return 10;
        case 'K': return 11;
        case 'A': return 12;
        default: throw invalid_argument("Bad rank character");
    }
}

static uint8_t parse_suit(char sc) {
    switch (sc) {
        case 'c': return 0;
        case 'd': return 1;
        case 'h': return 2;
        case 's': return 3;
        default: throw invalid_argument("Bad suit character");
    }
}

uint32_t eval_cards(const vector<std::string>& cards) {
    const size_t n = cards.size();
    if (n != 7) {
        throw invalid_argument("To evaluate a board you must have 7 cards");
    }

    uint8_t ranks[7];
    uint8_t suits[7];

    for (size_t i = 0; i < n; i++) {
        const string& c = cards[i];
        if (c.size() != 2) {
            throw invalid_argument("Card must be 2 chars, a suit and a rank");
        }
        ranks[i] = parse_rank(c[0]);
        suits[i] = parse_suit(c[1]);
    }

    return evaluate_raw(ranks, suits, (uint8_t)n);
}


//goated code written by bobby
// Precompute straight masks
static const uint16_t straight_masks[10] = {
    0b1111100000000,  // A-K-Q-J-T
    0b0111110000000,  // K-Q-J-T-9
    0b0011111000000,  // Q-J-T-9-8
    0b0001111100000,  // J-T-9-8-7
    0b0000111110000,  // T-9-8-7-6
    0b0000011111000,  // 9-8-7-6-5
    0b0000001111100,  // 8-7-6-5-4
    0b0000000111110,  // 7-6-5-4-3
    0b0000000011111,  // 6-5-4-3-2
    0b1000000001111   // A-5-4-3-2 (wheel)
};

uint32_t evaluate_raw(uint8_t* ranks, uint8_t* suits, uint8_t n)
{
    uint8_t counts_r[13];
    uint8_t counts_s[4];
    uint8_t count_counts[5];
    uint16_t overall = 0;
    uint16_t persuit[4];
    uint16_t mxfl = 0;
    uint8_t i, r, s;
    bool is_flush = false;
    bool straight_flush = false;
    uint8_t strfl_mx = 0;
    uint8_t mxfl_cnt = 0;
    
    // Initialize arrays
    memset(counts_r, 0, sizeof(counts_r));
    memset(counts_s, 0, sizeof(counts_s));
    memset(persuit, 0, sizeof(persuit));
    memset(count_counts, 0, sizeof(count_counts));
    
    uint8_t rank, suit;
    uint16_t rank2;
    
    count_counts[0] = n;
    
    // Count ranks and suits
    for(i = 0; i < n; i ++)
    {
        rank = ranks[i];
        suit = suits[i];
        
        count_counts[counts_r[rank]] -= 1;
        counts_r[rank] += 1;
        count_counts[counts_r[rank]] += 1;
        
        counts_s[suit] += 1;
        rank2 = 1 << rank;
        overall |= rank2;
        persuit[suit] |= rank2;
    }
    
    // Check for flush and straight flush
    for(s = 0; s < 4; s ++)
    {
        if(counts_s[s] >= 5)
        {
            is_flush = true;
            mxfl = max(mxfl, persuit[s]);
            mxfl_cnt = counts_s[s];
            for(r = 0; r < 10; r ++)
            {
                if((persuit[s] & straight_masks[r]) == straight_masks[r])
                {
                    strfl_mx = max(strfl_mx, (uint8_t)(10 - r));
                    straight_flush = true;
                    break;
                }
            }
        }
    }
    
    if(straight_flush)
        return (9 << 20) | (strfl_mx << 16);
    
    // Quads
    int8_t mx1 = -1;
    int8_t mx2 = -1;
    int8_t mx3 = -1;
    int8_t mx4 = -1;
    if(count_counts[4] > 0)
    {
        for(r = 0; r < 13; r ++)
        {
            if(counts_r[r] == 4)
            {
                mx2 = max(mx2, mx1);
                mx1 = r;
            }
            else if(counts_r[r] > 0)
                mx2 = r;
        }
        return (8 << 20) | (mx1 << 16) | (mx2 << 12);
    }
    
    // Full house
    if(count_counts[3] > 0 && count_counts[2]+count_counts[3] > 1)
    {
        for(r = 0; r < 13; r ++)
        {
            if(counts_r[r] == 2)
                mx2 = r;
            else if(counts_r[r] == 3)
            {
                mx2 = max(mx2, mx1);
                mx1 = r;
            }
        }
        return (7 << 20) | (mx1 << 16) | (mx2 << 12);
    }
    
    // Flush
    if(is_flush)
    {
        for(i = 0; i < mxfl_cnt-5; i ++)
            mxfl &= (mxfl - 1);
        return (6 << 20) | mxfl;
    }
    
    // Straight
    for(r = 0; r < 10; r ++)
        if((overall & straight_masks[r]) == straight_masks[r])
            return (5 << 20) | ((10 - r) << 16);
    
    // Trips
    if(count_counts[3] > 0)
    {
        for(r = 0; r < 13; r ++)
        {
            if(counts_r[r] == 1 || counts_r[r] == 2)
            {
                mx3 = mx2;
                mx2 = r;
            }
            else if(counts_r[r] == 3)
            {
                mx2 = max(mx2, mx1);
                mx1 = r;
            }
        }
        return (4 << 20) | (mx1 << 16) | (mx2 << 12) | (mx3 << 8);
    }
    
    // Two pair
    if(count_counts[2] > 1)
    {
        for(r = 0; r < 13; r ++)
        {
            if(counts_r[r] == 1)
                mx3 = r;
            else if(counts_r[r] == 2)
            {
                mx3 = max(mx3, mx2);
                mx2 = mx1;
                mx1 = r;
            }
        }
        return (3 << 20) | (mx1 << 16) | (mx2 << 12) | (mx3 << 8);
    }
    
    // One pair
    if(count_counts[2] > 0)
    {
        for(r = 0; r < 13; r ++)
        {
            if(counts_r[r] == 1)
            {
                mx4 = mx3;
                mx3 = mx2;
                mx2 = r;
            }
            else if(counts_r[r] == 2)
            {
                mx2 = max(mx2, mx1);
                mx1 = r;
            }
        }
        return (2 << 20) | (mx1 << 16) | (mx2 << 12) | (mx3 << 8)| (mx4 << 4);
    }
    
    // High card
    for(i = 0; i < n-5; i ++)
        overall &= (overall - 1);
    
    return (1 << 20) | overall;
}
    
