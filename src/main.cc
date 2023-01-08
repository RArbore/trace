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
#include <chrono>

#include "context.h"

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) noexcept -> int {
    RenderContext context {};
    context.init();
    auto system_time = std::chrono::system_clock::now();
    double rolling_average_dt = 0.0;
    while (context.active) {
	const auto current_time = std::chrono::system_clock::now();
	const std::chrono::duration<double> dt = current_time - system_time;
	system_time = current_time;
	rolling_average_dt += dt.count();
	context.render(dt.count());
	if (context.current_frame % 10000 == 0) {
	    printf("FPS: %f\n", 10000.0 / rolling_average_dt);
	    rolling_average_dt = 0.0;
	}
    }
    context.cleanup();
    return 0;
}
