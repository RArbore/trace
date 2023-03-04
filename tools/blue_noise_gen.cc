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

#include <iostream>
#include <string.h>

#include "Tracy.hpp"

#define NUM_THREADS 16

static int32_t texture_width, num_textures;
static uint8_t *scratch_memory;

auto usage() noexcept -> void {
    ZoneScoped;
    std::cout << "Usage: blue_noise_gen <texture width> <number of textures>\n";
}

auto hash32(uint32_t x) noexcept -> uint32_t {
    ZoneScoped;
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

auto blue_noise_gen_texture(uint8_t *texture_mem, int32_t texture_num) noexcept {
    ZoneScoped;
    memset(texture_mem, 0, texture_width * texture_width);

    for (int32_t i = 0; i < texture_width * texture_width; i += 4) {
	uint32_t hash = hash32((uint32_t) i);
	texture_mem[i] = (uint8_t) (hash >> 24) & 0xFF;
	texture_mem[i + 1] = (uint8_t) (hash >> 16) & 0xFF;
	texture_mem[i + 2] = (uint8_t) (hash >> 8) & 0xFF;
	texture_mem[i + 3] = (uint8_t) hash & 0xFF;
    }

    char texture_name[128];
    sprintf(texture_name, "assets/blue_noise_texture_%dx%d_num_%d.bin", texture_width, texture_width, texture_num);
    FILE *output_file = fopen(texture_name, "w");
    if (!output_file) {
	std::cerr << "PANIC: Couldn't open output file " << texture_name << ".\n";
	exit(1);
    }
    fwrite(texture_mem, 1, texture_width * texture_width, output_file);
    fclose(output_file);
}

auto blue_noise_gen_worker(void *thread_id_ptr) -> void * {
    int32_t thread_id = *((int32_t *) thread_id_ptr);
    for (int32_t i = thread_id; i < num_textures; i += NUM_THREADS) {
	blue_noise_gen_texture(scratch_memory + texture_width * texture_width * thread_id, i);
    }
    return NULL;
}

auto main(int32_t argc, char **argv) noexcept -> int32_t {
    ZoneScoped;
    FrameMark;
    if (argc != 3) {
	usage();
	exit(1);
    }
    int32_t num_chars;
    if (sscanf(argv[1], "%d%n", &texture_width, &num_chars) != 1 || argv[1][num_chars] != '\0') {
	usage();
	exit(1);
    }
    if (sscanf(argv[2], "%d%n", &num_textures, &num_chars) != 1 || argv[2][num_chars] != '\0') {
	usage();
	exit(1);
    }
    if (texture_width < 16) {
	std::cout << "Generated texture width must be at least 16.\n";
	usage();
	exit(1);
    }
    if (num_textures < 1) {
	std::cout << "Number of textures must be at least 1.\n";
	usage();
	exit(1);
    }

    scratch_memory = (uint8_t *) malloc(texture_width * texture_width * NUM_THREADS);
    if (!scratch_memory) {
	std::cerr << "PANIC: Couldn't allocate " << texture_width * texture_width * NUM_THREADS << " bytes of scratch memory.\n";
	exit(1);
    }

    pthread_t threads[NUM_THREADS];
    int32_t thread_ids[NUM_THREADS];
    for (int32_t i = 0; i < NUM_THREADS; ++i) {
	thread_ids[i] = i;
	pthread_create(&threads[i], NULL, blue_noise_gen_worker, (void *) &thread_ids[i]);
    }
    for (int32_t i = 0; i < NUM_THREADS; ++i) {
	pthread_join(threads[i], NULL);;
    }
}
