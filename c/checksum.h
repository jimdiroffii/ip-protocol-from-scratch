/***
 * @file checksum.h
 * @brief A ones' complement checksum algorithm
 *
 * @author Jim Diroff II
 */
#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Computes a 16-bit ones' complement checksum.
 * @param data Pointer to the data buffer.
 * @param length Number of bytes in @p data checksum.
 * @return 16-bit ones' complement checksum.
 */
uint16_t compute_checksum(const void *data, size_t length);

#endif /* CHECKSUM_H */