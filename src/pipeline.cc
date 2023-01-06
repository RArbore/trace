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

#include <filesystem>
#include <fstream>

#include "context.h"

static constexpr std::string_view DEFAULT_SHADER_PATH = "build";

auto RenderContext::create_shaders() noexcept -> void {
    ASSERT(std::filesystem::exists(DEFAULT_SHADER_PATH) && std::filesystem::is_directory(DEFAULT_SHADER_PATH), "");
    for (const auto& entry : std::filesystem::directory_iterator(DEFAULT_SHADER_PATH)) {
	auto filename = entry.path().filename();
	if (filename.extension() == ".spv") {
	    std::ifstream fstream(entry.path(), std::ios::in | std::ios::binary);
	    const auto size = std::filesystem::file_size(entry.path());
	    std::string result(size, '\0');
	    fstream.read(result.data(), size);
	    shader_spirvs[filename.stem()] = result;
	}
    }
}

auto RenderContext::cleanup_shaders() noexcept -> void {

}
