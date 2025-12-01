#include "header.h"

extern "C" {
#	include <quickjs.h>
#	include <quickjs-libc.h>
}

#include <cassert>
#include <cstdio>

namespace qjs
{
	constexpr char script[] =
		R"(
			function Fib(n)
			{	if (n < 2)
					return n;
				return Fib(n-1) + Fib(n-2);
			}

			function Worker(msg, val)
			{	return callback(msg, val + 777);
			}
		)";
	JSRuntime *rt = nullptr;

	// Error logging utilities
	void log_exception(JSContext *ctx)
	{	JSValue exc = JS_GetException(ctx);
		if (JS_IsError(exc))
		{	JSValue stack = JS_GetPropertyStr(ctx, exc, "stack");
			if (const char *stack_str = JS_ToCString(ctx, stack))
			{	std::fprintf(stderr, "QuickJS error:\n%s\n", stack_str);
				JS_FreeCString(ctx, stack_str);
			}
			JS_FreeValue(ctx, stack);
		}
		else
		{	const char *msg = JS_ToCString(ctx, exc);
			std::fprintf(stderr, "QuickJS exception: %s\n", msg ? msg : "[unknown]");
			JS_FreeCString(ctx, msg);
		}
		JS_FreeValue(ctx, exc);
	}

	// Native callback
	JSValue callback(JSContext *ctx, JSValueConst /*this_val*/, int argc, JSValueConst *argv)
	{	TM("0: QJS callback");
		const char *message = (argc > 0) ? JS_ToCString(ctx, argv[0]) : nullptr;
		int32_t value = 0;
		if (argc > 1)
		{	JS_ToInt32(ctx, &value, argv[1]);
		}
		if (!message)
		{	assert(false);
			printf("QuickJS callback: unexpected string '<NUL>'\n");
		}

		int32_t res = -1;
		if (const auto len = message ? strlen(message) : 0; len > 0)
		{	res = message[((value - KEY) % len + len) % len];
		}

		JS_FreeCString(ctx, message);
		return JS_NewInt32(ctx, res);
	}

	// Step 1: init
	JSContext* init()
	{	TM("1: QJS Initialize");

		rt = JS_NewRuntime();
		if (!rt)
		{	assert(false);
			std::fprintf(stderr, "QuickJS: failed to create runtime\n");
			return nullptr;
		}

		JSContext *ctx = JS_NewContext(rt);
		if (!ctx)
		{	assert(false);
			std::fprintf(stderr, "QuickJS: failed to create context\n");
			JS_FreeRuntime(rt);
			rt = nullptr;
			return nullptr;
		}

		// Add standard library functions (e.g., console.log)
		js_std_add_helpers(ctx, 0, nullptr);

		JSValue global = JS_GetGlobalObject(ctx);
		JSValue cfunc = JS_NewCFunction(ctx, callback, "callback", 2);
		JS_SetPropertyStr(ctx, global, "callback", cfunc);

		JS_FreeValue(ctx, global);
		return ctx;
	}

	// Step 2: load script
	bool load_script(JSContext *ctx)
	{	TM("2: QJS Load script");
		JSValue eval_res = JS_Eval(ctx, script, strlen(script), "<input>", JS_EVAL_TYPE_GLOBAL);
		if (JS_IsException(eval_res))
		{	std::fprintf(stderr, "QuickJS: script load error\n");
			log_exception(ctx);
			JS_FreeValue(ctx, eval_res);
			return false;
		}

		JS_FreeValue(ctx, eval_res);
		return true;
	}

	int call_worker(JSContext *ctx, const char* msg, int val)
	{	JSValue global = JS_GetGlobalObject(ctx);
		JSValue func = JS_GetPropertyStr(ctx, global, "Worker");
		if (!JS_IsFunction(ctx, func))
		{	std::fprintf(stderr, "QuickJS: function Worker not found\n");
			JS_FreeValue(ctx, func);
			JS_FreeValue(ctx, global);
			return -1;
		}

		int32_t result = -1;
		JSValue argv[2] = { JS_NewString(ctx, msg), JS_NewInt32(ctx, val) };
		JSValue ret = JS_Call(ctx, func, global, 2, argv);
		if (JS_IsException(ret))
		{	assert(false);
			std::fprintf(stderr, "QuickJS: exception in Worker()\n");
			log_exception(ctx);
		}
		else if (JS_ToInt32(ctx, &result, ret) != 0)
		{	std::fprintf(stderr, "QuickJS: return value is not int\n");
		}

		JS_FreeValue(ctx, ret);
		JS_FreeValue(ctx, func);
		JS_FreeValue(ctx, global);

		return result;
	}

	int call_fibonacci(JSContext *ctx, int val)
	{	JSValue global = JS_GetGlobalObject(ctx);
		JSValue func = JS_GetPropertyStr(ctx, global, "Fib");
		if (!JS_IsFunction(ctx, func))
		{	std::fprintf(stderr, "QuickJS: function Fib not found\n");
			JS_FreeValue(ctx, func);
			JS_FreeValue(ctx, global);
			return -1;
		}

		int32_t result = -1;
		JSValue argv = JS_NewInt32(ctx, val);
		JSValue ret = JS_Call(ctx, func, global, 1, &argv);
		if (JS_IsException(ret))
		{	assert(false);
			std::fprintf(stderr, "QuickJS: exception in Fib()\n");
			log_exception(ctx);
		}
		else if (JS_ToInt32(ctx, &result, ret) != 0)
		{	std::fprintf(stderr, "QuickJS: return value is not int\n");
		}

		JS_FreeValue(ctx, ret);
		JS_FreeValue(ctx, func);
		JS_FreeValue(ctx, global);

		return result;
	}

	// Step 3: call worker
	bool call(JSContext *ctx)
	{
		{	TM("3.1: QJS First Call");
			if (MSG[0] != call_worker(ctx, MSG, 0))
			{	assert(false);
				return false;
			}
		}

		for(int n = 0; n < 100; ++n)
		{	TM("3.2: QJS Other Call");
			if (MSG[n % (strlen(MSG))] != call_worker(ctx, MSG, n))
			{	assert(false);
				return false;
			}
		}

		{	TM("3.3: QJS Fib Call");
			if (const auto f = call_fibonacci(ctx, FIB_N); FIB_R != f)
			{	assert(false);
				return false;
			}
		}

		return true;
	}

	// Step 4: cleanup
	void cleanup(JSContext *ctx)
	{	TM("4: QJS Cleanup");

		if (ctx)
		{	JS_FreeContext(ctx);
		}
		if (rt)
		{	JS_FreeRuntime(rt);
			rt = nullptr;
		}
	}

	// Test entry
	bool test()
	{	TM("*QJS test");

		bool result = false;
		if (auto ctx = init())
		{	if (load_script(ctx))
			{	result = call(ctx);
			}

			cleanup(ctx);
		}

		return result;
	}

	const auto _ = register_test("QJS", test);

} // namespace qjs
