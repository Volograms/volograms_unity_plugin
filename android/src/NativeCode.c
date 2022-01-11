#include <string.h> // include memcpy()

#include "vol_av.h"
#include "vol_geom.h"

#if __cplusplus
extern "C"
{
#endif

float add(float x, float y)
{
	return x + y;
}

typedef void(*FuncCallBack)(const char* message, int color, int size);

void default_print(const char* message, int color, int size)
{
    return;
}

static FuncCallBack callbackInstance = default_print;

void register_debug_callback(FuncCallBack cb) {
    callbackInstance = cb;
	callbackInstance("TEST", 0, 4);
}

#if __cplusplus
}
#endif