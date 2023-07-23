#include <graphics/GraphicsContext.h>

int main(int argc, char *argv[]) {
    auto context = createGraphicsContext();

    while (!shouldExit(context)) {
	renderFrame(context);
    }

    destroyGraphicsContext(context);
}
