#include "header.h"
#include <vi_timing/vi_timing.h>

extern "C" {
	// Include QuickJS header
#   include <quickjs.h>
}

VI_TM(FILE_PATH);

namespace
{
	// ---------------------------------------------------------
	// 1. C++ Function to be called from JavaScript
	// ---------------------------------------------------------
	// QuickJS C functions match this signature:
	static JSValue cpp_callback(JSContext * ctx, JSValueConst this_val, int argc, JSValueConst * argv)
	{
		// Parse arguments: check if first arg is a string
		const char *message = JS_ToCString(ctx, argv[0]);

		// Free the C string returned by JS_ToCString
		JS_FreeCString(ctx, message);

		// Return an integer (42) back to JavaScript
		return JS_NewInt32(ctx, 42);
	}

	// ---------------------------------------------------------
	// 2. Main Application
	// ---------------------------------------------------------
	void test_qjs()
	{
		// Create a Runtime and a Context
		JSRuntime *rt = JS_NewRuntime();
		JSContext *ctx = JS_NewContext(rt);

		// Get the global object (like 'window' or 'global')
		JSValue global_obj = JS_GetGlobalObject(ctx);

		// Register the C++ function as a global property in JS
		// "cpp_callback" is the name used inside JS
		JSValue c_func = JS_NewCFunction(ctx, cpp_callback, "cpp_callback", 1);
		JS_SetPropertyStr(ctx, global_obj, "cpp_callback", c_func);

		// Define JS script inline
		const char *js_code = R"(
		function js_worker() {
			console.log('[JS] Inside js_worker function');
			
			console.log('[JS] Calling C++ callback...');
			let result = cpp_callback('Hello from JavaScript!');
			
			console.log('[JS] Callback returned: ' + result);
			console.log('[JS] Finishing js_worker');
		}
	)";

		// Evaluate the script to define the function
		JSValue eval_res = JS_Eval(ctx, js_code, strlen(js_code), "<input>", JS_EVAL_TYPE_GLOBAL);

		if (JS_IsException(eval_res))
		{
			// (Error handling omitted for brevity)
		}
		JS_FreeValue(ctx, eval_res);

		// --- THE FULL CYCLE HAPPENS HERE ---

		// Retrieve 'js_worker' from global scope
		JSValue js_func = JS_GetPropertyStr(ctx, global_obj, "js_worker");

		// Check if it's a function
		if (JS_IsFunction(ctx, js_func))
		{
			// Call the function: global as 'this', 0 args
			JSValue ret = JS_Call(ctx, js_func, global_obj, 0, NULL);

			if (JS_IsException(ret))
			{
			}
			else
			{
			}
			JS_FreeValue(ctx, ret);
		}
		else
		{
		}

		// Cleanup
		JS_FreeValue(ctx, js_func);
		JS_FreeValue(ctx, global_obj);
		JS_FreeContext(ctx);
		JS_FreeRuntime(rt);
	}

	const auto _ = register_test(test_qjs);
}
