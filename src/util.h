/*
 * This file is part of trace.
 * trace is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 * trace is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with trace. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef UTIL_H
#define UTIL_H

#include <iostream>

#include <vulkan/vulkan.h>

inline auto assert_impl(VkResult result, const char *msg, const char* file, uint32_t line) noexcept -> void {
    if (result != VK_SUCCESS) {
	fprintf(stderr, "PANIC: %s\nOccurred in %s at line %u.\n", msg, file, line);
	exit(-1);
    }
}

inline auto assert_impl(bool result, const char *msg, const char* file, uint32_t line) noexcept -> void {
    if (!result) {
	fprintf(stderr, "PANIC: %s\nOccurred in %s at line %u.\n", msg, file, line);
	exit(-1);
    }
}

inline auto assert_impl(int32_t result, const char *msg, const char* file, uint32_t line) noexcept -> void {
    if (result == -1) {
	fprintf(stderr, "PANIC: %s\nOccurred in %s at line %u.\n", msg, file, line);
	exit(-1);
    }
}

inline auto assert_impl(uint32_t result, const char *msg, const char* file, uint32_t line) noexcept -> void {
    if (result == 0xFFFFFFFF) {
	fprintf(stderr, "PANIC: %s\nOccurred in %s at line %u.\n", msg, file, line);
	exit(-1);
    }
}

#define ASSERT(res, msg)			\
    assert_impl(res, msg, __FILE__, __LINE__);

#endif
