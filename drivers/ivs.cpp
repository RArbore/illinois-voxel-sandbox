#include <graphics/GraphicsContext.h>

int main(int argc, char *argv[]) {
    auto context = create_graphics_context();

    while (!should_exit(context)) {
	render_frame(context, nullptr);
    }
}
