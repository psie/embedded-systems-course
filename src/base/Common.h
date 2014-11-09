#ifndef __BASE_COMMON_H__
#define __BASE_COMMON_H__

#include <stdint.h>

#define UNUSED __attribute__((unused))
#define PURE __attribute__((pure))

#define htons(x) ((x) >> 8 | (x) << 8)

template <typename T>
void bclr(T &word, uint8_t pos) { word &= ~(1 << pos); }

template <typename T>
void bset(T &word, uint8_t pos) { word |= (1 << pos); }

template <typename T>
T bget(T word, uint8_t pos) { return (word & (1 << pos)); }

template <typename T0, typename T1>
void bput(T0 &word, uint8_t pos, T1 value)
{
  if (value) bset(word, pos); else bclr(word, pos);
}

template <typename T0, typename T1>
T1 map(T0 x, T0 in_min, T0 in_max, T1 out_min, T1 out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#endif
