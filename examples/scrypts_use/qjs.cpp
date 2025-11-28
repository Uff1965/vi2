#include "header.h"

extern "C" {
#include <quickjs.h>
#include <quickjs-libc.h>
}

#include <cassert>
#include <cstdio>

namespace qjs
{
	constexpr int VAL = 42;
	JSRuntime *rt = nullptr;

	// Error logging utilities
	void log_exception(JSContext *ctx)
	{	JSValue exc = JS_GetException(ctx);
		if (!JS_IsError(exc))
		{	const char *msg = JS_ToCString(ctx, exc);
			std::fprintf(stderr, "QuickJS exception: %s\n", msg ? msg : "[unknown]");
			JS_FreeCString(ctx, msg);
		}
		else
		{	JSValue stack = JS_GetPropertyStr(ctx, exc, "stack");
			if (const char *stack_str = JS_ToCString(ctx, stack))
			{	std::fprintf(stderr, "QuickJS error:\n%s\n", stack_str);
				JS_FreeCString(ctx, stack_str);
			}
			JS_FreeValue(ctx, stack);
		}
		JS_FreeValue(ctx, exc);
	}

	// Native callback
	JSValue cpp_callback(JSContext *ctx, JSValueConst /*this_val*/, int argc, JSValueConst *argv)
	{	TM("0: QJS callback");
		const char *message = (argc > 0) ? JS_ToCString(ctx, argv[0]) : nullptr;
		if (!message || strcmp(message, "Hello, World!") != 0)
		{	assert(false);
			printf("QuickJS callback: unexpected string '%s'\n", message? message: "<Nul>");
		}
		JS_FreeCString(ctx, message);
		return JS_NewInt32(ctx, VAL);
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
		JSValue cfunc = JS_NewCFunction(ctx, cpp_callback, "cpp_callback", 1);
		JS_SetPropertyStr(ctx, global, "cpp_callback", cfunc);

		JS_FreeValue(ctx, global);
		return ctx;
	}

	// Step 2: load script
	bool load_script(JSContext *ctx)
	{	TM("2: QJS Load script");

		static constexpr char script[] = R"(function js_worker() {return cpp_callback("Hello, World!");})";

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

	// ---------------------------------------------------------
	// Step 3: call worker
	// ---------------------------------------------------------
	bool call_worker(JSContext *ctx)
	{	JSValue global = JS_GetGlobalObject(ctx);
		JSValue func = JS_GetPropertyStr(ctx, global, "js_worker");
		if (!JS_IsFunction(ctx, func))
		{	std::fprintf(stderr, "QuickJS: function js_worker not found\n");
			JS_FreeValue(ctx, func);
			JS_FreeValue(ctx, global);
			return false;
		}

		JSValue ret = JS_Call(ctx, func, global, 0, nullptr);
		if (JS_IsException(ret))
		{	assert(false);
			std::fprintf(stderr, "QuickJS: exception in js_worker()\n");
			log_exception(ctx);
		}
		else
		{	int32_t val;
			if (JS_ToInt32(ctx, &val, ret) == 0)
			{	assert(val == VAL);
			}
			else
			{	std::fprintf(stderr, "QuickJS: return value is not int\n");
			}
		}

		JS_FreeValue(ctx, ret);
		JS_FreeValue(ctx, func);
		JS_FreeValue(ctx, global);

		return true;
	}

	bool call(JSContext *ctx)
	{
		{	TM("3.1: QJS First Call");
			if(!call_worker(ctx))
			{	return false;
			}
		}

		for(int n = 0; n < 100; ++n)
		{	TM("3.2: QJS Other Call");
			if(!call_worker(ctx))
			{	return false;
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
	{	TM("qjs test");

		bool result = false;
		if (auto ctx = init())
		{	if (load_script(ctx))
			{	result = call(ctx);
			}

			cleanup(ctx);
		}

		return result;
	}

	const auto _ = register_test("QuickJS", test);

} // namespace qjs
