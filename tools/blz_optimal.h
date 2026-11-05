#ifndef YU_BLZ_OPTIMAL_H
#define YU_BLZ_OPTIMAL_H
#include <blz.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

static uint8_t *YuBlz_Optimal(const uint8_t *input, int input_length, uint32_t *output_length, int overlap)
{
    if (!output_length || input_length < 0 || input_length > 0x200000 || (!input && input_length))
        return NULL;
    *output_length = 0;
    uint32_t n = (uint32_t)input_length;
    uint8_t *reverse = malloc(n ? n : 1);
    uint8_t *matches = calloc(n ? n : 1, 1);
    uint16_t *distances = calloc(n ? n : 1, sizeof(*distances));
    int32_t *previous = malloc((n ? n : 1) * sizeof(*previous));
    int32_t *heads = malloc(65536 * sizeof(*heads));
    uint32_t *costs = malloc(((size_t)n + 1) * 8 * sizeof(*costs));
    uint8_t *choices = calloc(((size_t)n + 1) * 8, 1);
    uint8_t *result = NULL;
    if (!reverse || !matches || !distances || !previous || !heads || !costs || !choices)
        goto done;
    for (uint32_t i = 0; i < n; ++i)
        reverse[i] = input[n - i - 1];
    memset(heads, 0xFF, 65536 * sizeof(*heads));
    memset(costs, 0xFF, ((size_t)n + 1) * 8 * sizeof(*costs));
    costs[0] = 0;
    for (uint32_t i = 0; i + 2 < n; ++i)
    {
        uint32_t key = ((reverse[i] * 257u + reverse[i + 1]) * 257u + reverse[i + 2]) & 65535u;
        previous[i] = heads[key];
        heads[key] = (int32_t)i;
        uint32_t goal = n - i < 18 ? n - i : 18;
        for (int32_t prior = previous[i]; prior >= 0 && i - (uint32_t)prior <= 4098; prior = previous[prior])
        {
            uint32_t distance = i - (uint32_t)prior;
            if (distance < 3 || reverse[prior] != reverse[i] || reverse[prior + 1] != reverse[i + 1] || reverse[prior + 2] != reverse[i + 2])
                continue;
            uint32_t limit = !overlap && distance < goal ? distance : goal;
            uint32_t length = 3;
            while (length < limit && reverse[prior + length] == reverse[i + length])
                ++length;
            if (length > matches[i])
            {
                matches[i] = (uint8_t)length;
                distances[i] = (uint16_t)distance;
                if (length == goal)
                    break;
            }
        }
    }
    uint32_t best_score = n, best_position = 0, best_state = 0;
    for (uint32_t i = 0; i <= n; ++i)
    {
        for (uint32_t state = 0; state < 8; ++state)
        {
            uint32_t cost = costs[(size_t)i * 8 + state];
            if (cost == UINT32_MAX)
                continue;
            uint32_t score = n - i + cost;
            if (score < best_score)
            {
                best_score = score;
                best_position = i;
                best_state = state;
            }
            if (i == n)
                continue;
            uint32_t next = (state + 1) & 7;
            uint32_t literal = cost + 1 + (state == 0);
            size_t position = (size_t)(i + 1) * 8 + next;
            if (literal < costs[position])
            {
                costs[position] = literal;
                choices[position] = 1;
            }
            uint32_t encoded = cost + 2 + (state == 0);
            for (uint32_t length = 3; length <= matches[i]; ++length)
            {
                position = (size_t)(i + length) * 8 + next;
                if (encoded < costs[position])
                {
                    costs[position] = encoded;
                    choices[position] = (uint8_t)length;
                }
            }
        }
    }
    uint32_t total = ((best_score + 3) & ~3u) + 8;
    if (!best_position || total >= n)
    {
        total = ((n + 3) & ~3u) + 4;
        result = calloc(total ? total : 1, 1);
        if (result && n)
            memcpy(result, input, n);
        if (result)
            *output_length = total;
        goto done;
    }
    uint32_t compressed_length = costs[(size_t)best_position * 8 + best_state];
    uint32_t prefix = n - best_position;
    uint8_t *tokens = matches;
    memset(tokens, 0, n);
    uint32_t cursor = best_position, state = best_state;
    while (cursor)
    {
        uint32_t length = choices[(size_t)cursor * 8 + state];
        if (!length || length > cursor)
            goto done;
        cursor -= length;
        tokens[cursor] = (uint8_t)length;
        state = (state + 7) & 7;
    }
    result = malloc(total);
    if (!result)
        goto done;
    memcpy(result, input, prefix);
    uint8_t *encoded = result + prefix;
    cursor = 0;
    uint32_t produced = 0;
    while (cursor < best_position)
    {
        uint32_t flag = produced++;
        encoded[flag] = 0;
        for (uint32_t bit = 0; bit < 8 && cursor < best_position; ++bit)
        {
            uint32_t length = tokens[cursor];
            if (length == 1)
                encoded[produced++] = reverse[cursor];
            else
            {
                encoded[flag] |= (uint8_t)(0x80u >> bit);
                uint32_t reference = ((length - 3) << 12) | (distances[cursor] - 3);
                encoded[produced++] = (uint8_t)(reference >> 8);
                encoded[produced++] = (uint8_t)reference;
            }
            cursor += length;
        }
    }
    if (produced != compressed_length)
    {
        free(result);
        result = NULL;
        goto done;
    }
    for (uint32_t i = 0; i < compressed_length / 2; ++i)
    {
        uint8_t value = encoded[i];
        encoded[i] = encoded[compressed_length - i - 1];
        encoded[compressed_length - i - 1] = value;
    }
    uint32_t end = prefix + compressed_length;
    uint32_t header_length = total - end;
    memset(result + end, 0xFF, header_length - 8);
    uint32_t section_length = compressed_length + header_length;
    result[total - 8] = (uint8_t)section_length;
    result[total - 7] = (uint8_t)(section_length >> 8);
    result[total - 6] = (uint8_t)(section_length >> 16);
    result[total - 5] = (uint8_t)header_length;
    uint32_t increment = n - total;
    for (uint32_t i = 0; i < 4; ++i)
        result[total - 4 + i] = (uint8_t)(increment >> (i * 8));
    *output_length = total;
done:
    free(reverse);
    free(matches);
    free(distances);
    free(previous);
    free(heads);
    free(costs);
    free(choices);
    return result;
}

static int YuBlz_Verify(const uint8_t *encoded, uint32_t size, const uint8_t *raw, uint32_t raw_size)
{
    if (size < 8 || size >= raw_size)
        return 0;
    uint32_t section = encoded[size - 8] | (uint32_t)encoded[size - 7] << 8 | (uint32_t)encoded[size - 6] << 16;
    uint32_t header = encoded[size - 5];
    uint32_t increment = encoded[size - 4] | (uint32_t)encoded[size - 3] << 8 | (uint32_t)encoded[size - 2] << 16 | (uint32_t)encoded[size - 1] << 24;
    if (header < 8 || header > 11 || section < header || section > size || increment != raw_size - size)
        return 0;
    uint8_t *decoded = malloc(raw_size);
    if (!decoded)
        return 0;
    memcpy(decoded, encoded, size);
    uint32_t prefix = size - section, source = size - header, destination = raw_size;
    int valid = 1;
    while (source > prefix && destination > prefix && valid)
    {
        uint8_t flags = decoded[--source];
        for (uint32_t bit = 0; bit < 8 && destination > prefix; ++bit)
        {
            uint32_t length = 1, distance = 0;
            if (flags & (0x80u >> bit))
            {
                if (source - prefix < 2)
                {
                    valid = 0;
                    break;
                }
                uint32_t reference = (uint32_t)decoded[source - 1] << 8 | decoded[source - 2];
                source -= 2;
                length = (reference >> 12) + 3;
                distance = (reference & 0xFFF) + 3;
            }
            else if (source == prefix)
            {
                valid = 0;
                break;
            }
            if (length > destination - prefix || (distance && distance > raw_size - destination))
            {
                valid = 0;
                break;
            }
            if (!distance)
            {
                uint8_t value = decoded[--source];
                if (--destination < source)
                {
                    valid = 0;
                    break;
                }
                decoded[destination] = value;
            }
            else
                for (uint32_t i = 0; i < length; ++i)
                {
                    if (--destination < source)
                    {
                        valid = 0;
                        break;
                    }
                    decoded[destination] = decoded[destination + distance];
                }
        }
    }
    valid = valid && source == prefix && destination == prefix && memcmp(decoded, raw, raw_size) == 0;
    free(decoded);
    return valid;
}

static uint8_t *YuBlz_Code(uint8_t *raw, int size, uint32_t *new_size, int best)
{
    uint8_t *original = BLZ_Code(raw, size, new_size, best);
    const char *enabled = getenv("MAKEROM_BLZ_OPTIMAL");
    if (!original || !enabled || strcmp(enabled, "1") != 0)
        return original;
    uint32_t optimal_size = 0;
    uint8_t *optimal = YuBlz_Optimal(raw, size, &optimal_size, 1);
    if (optimal && optimal_size < *new_size && YuBlz_Verify(optimal, optimal_size, raw, (uint32_t)size))
    {
        free(original);
        *new_size = optimal_size;
        return optimal;
    }
    free(optimal);
    return original;
}

#endif
