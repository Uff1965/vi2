// Build: link with vi_timing and Lua (e.g., -lvi_timing -llua)
// Usage in Lua: local tm = require("vi_timing")

#include <vi_timing/vi_timing.h>

#include <lua/lua.hpp>

#include <cstring> // for strlen

namespace
{
	// Helpers: convert between Lua and C handles
	VI_TM_HREG lua_check_reg(lua_State *L, int idx)
	{
		void *p = lua_touserdata(L, idx);
		if (!p) luaL_error(L, "expected registry handle (lightuserdata) at index %d", idx);
		return reinterpret_cast<VI_TM_HREG>(p);
	}

	VI_TM_HMEAS lua_check_meas(lua_State *L, int idx)
	{
		void *p = lua_touserdata(L, idx);
		if (!p) luaL_error(L, "expected measurement handle (lightuserdata) at index %d", idx);
		return reinterpret_cast<VI_TM_HMEAS>(p);
	}
	 
	void lua_push_reg(lua_State *L, VI_TM_HREG h)
	{
		lua_pushlightuserdata(L, reinterpret_cast<void *>(h));
	}

	void lua_push_meas(lua_State *L, VI_TM_HMEAS h)
	{
		lua_pushlightuserdata(L, reinterpret_cast<void *>(h));
	}

	// vi_tmReportCb adapter: call a Lua function (if provided) or accumulate to a buffer
	struct LuaReportCtx
	{
		lua_State *L;
		int fn_ref; // LUA_REFNIL if none
	};

	VI_TM_RESULT VI_SYS_CALL lua_report_cb(const char *str, void *ctx)
	{
		auto *c = static_cast<LuaReportCtx *>(ctx);
		if (!c) return 0;
		if (c->fn_ref == LUA_REFNIL)
		{
			// If no function, print to stdout via Lua's print equivalent (leave as no-op here)
			// You can integrate with Lua IO if needed; returning length for compatibility.
			return (VI_TM_RESULT)std::strlen(str);
		}
		// Call Lua function: fn(str)
		lua_rawgeti(c->L, LUA_REGISTRYINDEX, c->fn_ref);
		lua_pushstring(c->L, str);
		if (lua_pcall(c->L, 1, 0, 0) != LUA_OK)
		{
			// On error, report non-zero to stop enumeration/report
			return -1;
		}
		return (VI_TM_RESULT)std::strlen(str);
	}

	// ------------------- Wrapped functions -------------------

	// vi_tmGetTicks() -> integer
	int l_vi_tmGetTicks(lua_State *L)
	{
		VI_TM_TICK t = vi_tmGetTicks();
		lua_pushinteger(L, (lua_Integer)t);
		return 1;
	}

	// vi_tmGlobalInit(flags, title?, footer?) -> result(int)
	int l_vi_tmGlobalInit(lua_State *L)
	{
		VI_TM_FLAGS flags = (VI_TM_FLAGS)luaL_optinteger(L, 1, vi_tmReportDefault);
		const char *title = luaL_optstring(L, 2, "Timing report:\n");
		const char *footer = luaL_optstring(L, 3, nullptr);
		VI_TM_RESULT r = vi_tmGlobalInit(flags, title, footer);
		lua_pushinteger(L, r);
		return 1;
	}

	// vi_tmRegistryCreate() -> registry(lightuserdata)
	int l_vi_tmRegistryCreate(lua_State *L)
	{
		VI_TM_HREG h = vi_tmRegistryCreate();
		if (!h) return 0; // push nil
		lua_push_reg(L, h);
		return 1;
	}

	// vi_tmRegistryReset(reg) -> ()
	int l_vi_tmRegistryReset(lua_State *L)
	{
		VI_TM_HREG j = lua_check_reg(L, 1);
		vi_tmRegistryReset(j);
		return 0;
	}

	// vi_tmRegistryClose(reg) -> ()
	int l_vi_tmRegistryClose(lua_State *L)
	{
		VI_TM_HREG j = lua_check_reg(L, 1);
		vi_tmRegistryClose(j);
		return 0;
	}

	// vi_tmRegistryGetMeas(reg, name) -> meas(lightuserdata)|nil
	int l_vi_tmRegistryGetMeas(lua_State *L)
	{
		VI_TM_HREG j = lua_check_reg(L, 1);
		const char *name = luaL_checkstring(L, 2);
		VI_TM_HMEAS m = vi_tmRegistryGetMeas(j, name);
		if (!m) return 0;
		lua_push_meas(L, m);
		return 1;
	}

	// vi_tmRegistryEnumerateMeas(reg, fn(meas)->int) -> result(int)
	VI_TM_RESULT VI_TM_CALL lua_enum_cb(VI_TM_HMEAS meas, void *ctx)
	{
		lua_State *L = static_cast<lua_State *>(ctx);
		// Stack: push function (at registry index stored in upvalue) — we pass fn via upvalue on C closure
		// But here ctx is lua_State; we instead capture the function in a global upvalue slot before call.
		// Simpler approach: store function in registry key "vi_tm_enum_fn".
		lua_getfield(L, LUA_REGISTRYINDEX, "vi_tm_enum_fn");
		if (!lua_isfunction(L, -1))
		{
			lua_pop(L, 1);
			return -1;
		}
		lua_pushlightuserdata(L, (void *)meas);
		if (lua_pcall(L, 1, 1, 0) != LUA_OK)
		{
			lua_pop(L, 1); // error message
			return -1;
		}
		VI_TM_RESULT rc = (VI_TM_RESULT)luaL_optinteger(L, -1, 0);
		lua_pop(L, 1);
		return rc;
	}

	int l_vi_tmRegistryEnumerateMeas(lua_State *L)
	{
		VI_TM_HREG j = lua_check_reg(L, 1);
		luaL_checktype(L, 2, LUA_TFUNCTION);
		// Store callback in registry
		lua_pushvalue(L, 2);
		lua_setfield(L, LUA_REGISTRYINDEX, "vi_tm_enum_fn");
		VI_TM_RESULT rc = vi_tmRegistryEnumerateMeas(j, lua_enum_cb, (void *)L);
		// Remove callback
		lua_pushnil(L);
		lua_setfield(L, LUA_REGISTRYINDEX, "vi_tm_enum_fn");
		lua_pushinteger(L, rc);
		return 1;
	}

	// vi_tmMeasurementAdd(meas, dur, cnt?) -> ()
	int l_vi_tmMeasurementAdd(lua_State *L)
	{
		VI_TM_HMEAS m = lua_check_meas(L, 1);
		VI_TM_TDIFF dur = (VI_TM_TDIFF)luaL_checkinteger(L, 2);
		VI_TM_SIZE cnt = (VI_TM_SIZE)luaL_optinteger(L, 3, 1);
		vi_tmMeasurementAdd(m, dur, cnt);
		return 0;
	}

	// vi_tmMeasurementMerge(meas, stats_table) -> ()
	// stats_table fields: calls, cnt, sum, flt_calls, flt_cnt, flt_avg, flt_ss, min, max (as available)
	int l_vi_tmMeasurementMerge(lua_State *L)
	{
		VI_TM_HMEAS m = lua_check_meas(L, 1);
		luaL_checktype(L, 2, LUA_TTABLE);
		vi_tmStats_t s{};
		// Minimal fields
		lua_getfield(L, 2, "calls"); if (lua_isinteger(L, -1)) s.calls_ = (VI_TM_SIZE)lua_tointeger(L, -1); lua_pop(L, 1);
#if VI_TM_STAT_USE_RAW
		lua_getfield(L, 2, "cnt");   if (lua_isinteger(L, -1)) s.cnt_ = (VI_TM_SIZE)lua_tointeger(L, -1); lua_pop(L, 1);
		lua_getfield(L, 2, "sum");   if (lua_isinteger(L, -1)) s.sum_ = (VI_TM_TDIFF)lua_tointeger(L, -1); lua_pop(L, 1);
#endif
#if VI_TM_STAT_USE_RMSE
		lua_getfield(L, 2, "flt_calls"); if (lua_isinteger(L, -1)) s.flt_calls_ = (VI_TM_SIZE)lua_tointeger(L, -1); lua_pop(L, 1);
		lua_getfield(L, 2, "flt_cnt");   if (lua_isnumber(L, -1))  s.flt_cnt_ = (VI_TM_FP)lua_tonumber(L, -1); lua_pop(L, 1);
		lua_getfield(L, 2, "flt_avg");   if (lua_isnumber(L, -1))  s.flt_avg_ = (VI_TM_FP)lua_tonumber(L, -1); lua_pop(L, 1);
		lua_getfield(L, 2, "flt_ss");    if (lua_isnumber(L, -1))  s.flt_ss_ = (VI_TM_FP)lua_tonumber(L, -1); lua_pop(L, 1);
#endif
#if VI_TM_STAT_USE_MINMAX
		lua_getfield(L, 2, "min");       if (lua_isnumber(L, -1))  s.min_ = (VI_TM_FP)lua_tonumber(L, -1); lua_pop(L, 1);
		lua_getfield(L, 2, "max");       if (lua_isnumber(L, -1))  s.max_ = (VI_TM_FP)lua_tonumber(L, -1); lua_pop(L, 1);
#endif
		vi_tmMeasurementMerge(m, &s);
		return 0;
	}

	// vi_tmMeasurementGet(meas) -> name(string)|nil, stats(table)|nil
	int l_vi_tmMeasurementGet(lua_State *L)
	{
		VI_TM_HMEAS m = lua_check_meas(L, 1);
		const char *name = nullptr;
		vi_tmStats_t s{};
		vi_tmMeasurementGet(m, &name, &s);
		if (name) lua_pushstring(L, name); else lua_pushnil(L);
		// stats table
		lua_createtable(L, 0, 10);
		lua_pushinteger(L, (lua_Integer)s.calls_); lua_setfield(L, -2, "calls");
#if VI_TM_STAT_USE_RAW
		lua_pushinteger(L, (lua_Integer)s.cnt_);   lua_setfield(L, -2, "cnt");
		lua_pushinteger(L, (lua_Integer)s.sum_);   lua_setfield(L, -2, "sum");
#endif
#if VI_TM_STAT_USE_RMSE
		lua_pushinteger(L, (lua_Integer)s.flt_calls_); lua_setfield(L, -2, "flt_calls");
		lua_pushnumber(L, (lua_Number)s.flt_cnt_);     lua_setfield(L, -2, "flt_cnt");
		lua_pushnumber(L, (lua_Number)s.flt_avg_);     lua_setfield(L, -2, "flt_avg");
		lua_pushnumber(L, (lua_Number)s.flt_ss_);      lua_setfield(L, -2, "flt_ss");
#endif
#if VI_TM_STAT_USE_MINMAX
		lua_pushnumber(L, (lua_Number)s.min_);         lua_setfield(L, -2, "min");
		lua_pushnumber(L, (lua_Number)s.max_);         lua_setfield(L, -2, "max");
#endif
		return 2;
	}

	// vi_tmMeasurementReset(meas) -> ()
	int l_vi_tmMeasurementReset(lua_State *L)
	{
		VI_TM_HMEAS m = lua_check_meas(L, 1);
		vi_tmMeasurementReset(m);
		return 0;
	}

	// vi_tmStatsAdd(stats_table, dur, cnt?) -> ()
	int l_vi_tmStatsAdd(lua_State *L)
	{
		luaL_checktype(L, 1, LUA_TTABLE);
		VI_TM_TDIFF dur = (VI_TM_TDIFF)luaL_checkinteger(L, 2);
		VI_TM_SIZE cnt = (VI_TM_SIZE)luaL_optinteger(L, 3, 1);
		vi_tmStats_t s{};
		// read current
		lua_getfield(L, 1, "calls"); if (lua_isinteger(L, -1)) s.calls_ = (VI_TM_SIZE)lua_tointeger(L, -1); lua_pop(L, 1);
#if VI_TM_STAT_USE_RAW
		lua_getfield(L, 1, "cnt");   if (lua_isinteger(L, -1)) s.cnt_ = (VI_TM_SIZE)lua_tointeger(L, -1); lua_pop(L, 1);
		lua_getfield(L, 1, "sum");   if (lua_isinteger(L, -1)) s.sum_ = (VI_TM_TDIFF)lua_tointeger(L, -1); lua_pop(L, 1);
#endif
#if VI_TM_STAT_USE_RMSE
		lua_getfield(L, 1, "flt_calls"); if (lua_isinteger(L, -1)) s.flt_calls_ = (VI_TM_SIZE)lua_tointeger(L, -1); lua_pop(L, 1);
		lua_getfield(L, 1, "flt_cnt");   if (lua_isnumber(L, -1))  s.flt_cnt_ = (VI_TM_FP)lua_tonumber(L, -1); lua_pop(L, 1);
		lua_getfield(L, 1, "flt_avg");   if (lua_isnumber(L, -1))  s.flt_avg_ = (VI_TM_FP)lua_tonumber(L, -1); lua_pop(L, 1);
		lua_getfield(L, 1, "flt_ss");    if (lua_isnumber(L, -1))  s.flt_ss_ = (VI_TM_FP)lua_tonumber(L, -1); lua_pop(L, 1);
#endif
#if VI_TM_STAT_USE_MINMAX
		lua_getfield(L, 1, "min");       if (lua_isnumber(L, -1))  s.min_ = (VI_TM_FP)lua_tonumber(L, -1); lua_pop(L, 1);
		lua_getfield(L, 1, "max");       if (lua_isnumber(L, -1))  s.max_ = (VI_TM_FP)lua_tonumber(L, -1); lua_pop(L, 1);
#endif
		vi_tmStatsAdd(&s, dur, cnt);
		// write back
		lua_pushinteger(L, (lua_Integer)s.calls_); lua_setfield(L, 1, "calls");
#if VI_TM_STAT_USE_RAW
		lua_pushinteger(L, (lua_Integer)s.cnt_);   lua_setfield(L, 1, "cnt");
		lua_pushinteger(L, (lua_Integer)s.sum_);   lua_setfield(L, 1, "sum");
#endif
#if VI_TM_STAT_USE_RMSE
		lua_pushinteger(L, (lua_Integer)s.flt_calls_); lua_setfield(L, 1, "flt_calls");
		lua_pushnumber(L, (lua_Number)s.flt_cnt_);     lua_setfield(L, 1, "flt_cnt");
		lua_pushnumber(L, (lua_Number)s.flt_avg_);     lua_setfield(L, 1, "flt_avg");
		lua_pushnumber(L, (lua_Number)s.flt_ss_);      lua_setfield(L, 1, "flt_ss");
#endif
#if VI_TM_STAT_USE_MINMAX
		lua_pushnumber(L, (lua_Number)s.min_);         lua_setfield(L, 1, "min");
		lua_pushnumber(L, (lua_Number)s.max_);         lua_setfield(L, 1, "max");
#endif
		return 0;
	}

	// vi_tmStatsMerge(dst_table, src_table) -> ()
	int l_vi_tmStatsMerge(lua_State *L)
	{
		luaL_checktype(L, 1, LUA_TTABLE);
		luaL_checktype(L, 2, LUA_TTABLE);
		vi_tmStats_t dst{}, src{};
		// read both (reuse reader from above for brevity)
		auto read_stats = [](lua_State *Ls, int idx, vi_tmStats_t &out)
			{
				lua_pushvalue(Ls, idx);
				lua_getfield(Ls, -1, "calls"); if (lua_isinteger(Ls, -1)) out.calls_ = (VI_TM_SIZE)lua_tointeger(Ls, -1); lua_pop(Ls, 1);
#if VI_TM_STAT_USE_RAW
				lua_getfield(Ls, -1, "cnt");   if (lua_isinteger(Ls, -1)) out.cnt_ = (VI_TM_SIZE)lua_tointeger(Ls, -1); lua_pop(Ls, 1);
				lua_getfield(Ls, -1, "sum");   if (lua_isinteger(Ls, -1)) out.sum_ = (VI_TM_TDIFF)lua_tointeger(Ls, -1); lua_pop(Ls, 1);
#endif
#if VI_TM_STAT_USE_RMSE
				lua_getfield(Ls, -1, "flt_calls"); if (lua_isinteger(Ls, -1)) out.flt_calls_ = (VI_TM_SIZE)lua_tointeger(Ls, -1); lua_pop(Ls, 1);
				lua_getfield(Ls, -1, "flt_cnt");   if (lua_isnumber(Ls, -1))  out.flt_cnt_ = (VI_TM_FP)lua_tonumber(Ls, -1); lua_pop(Ls, 1);
				lua_getfield(Ls, -1, "flt_avg");   if (lua_isnumber(Ls, -1))  out.flt_avg_ = (VI_TM_FP)lua_tonumber(Ls, -1); lua_pop(Ls, 1);
				lua_getfield(Ls, -1, "flt_ss");    if (lua_isnumber(Ls, -1))  out.flt_ss_ = (VI_TM_FP)lua_tonumber(Ls, -1); lua_pop(Ls, 1);
#endif
#if VI_TM_STAT_USE_MINMAX
				lua_getfield(Ls, -1, "min");       if (lua_isnumber(Ls, -1))  out.min_ = (VI_TM_FP)lua_tonumber(Ls, -1); lua_pop(Ls, 1);
				lua_getfield(Ls, -1, "max");       if (lua_isnumber(Ls, -1))  out.max_ = (VI_TM_FP)lua_tonumber(Ls, -1); lua_pop(Ls, 1);
#endif
				lua_pop(Ls, 1);
			};
		read_stats(L, 1, dst);
		read_stats(L, 2, src);

		vi_tmStatsMerge(&dst, &src);

		// write back dst into table 1
		lua_pushinteger(L, (lua_Integer)dst.calls_); lua_setfield(L, 1, "calls");
#if VI_TM_STAT_USE_RAW
		lua_pushinteger(L, (lua_Integer)dst.cnt_);   lua_setfield(L, 1, "cnt");
		lua_pushinteger(L, (lua_Integer)dst.sum_);   lua_setfield(L, 1, "sum");
#endif
#if VI_TM_STAT_USE_RMSE
		lua_pushinteger(L, (lua_Integer)dst.flt_calls_); lua_setfield(L, 1, "flt_calls");
		lua_pushnumber(L, (lua_Number)dst.flt_cnt_);     lua_setfield(L, 1, "flt_cnt");
		lua_pushnumber(L, (lua_Number)dst.flt_avg_);     lua_setfield(L, 1, "flt_avg");
		lua_pushnumber(L, (lua_Number)dst.flt_ss_);      lua_setfield(L, 1, "flt_ss");
#endif
#if VI_TM_STAT_USE_MINMAX
		lua_pushnumber(L, (lua_Number)dst.min_);         lua_setfield(L, 1, "min");
		lua_pushnumber(L, (lua_Number)dst.max_);         lua_setfield(L, 1, "max");
#endif
		return 0;
	}

	// vi_tmStatsReset(stats_table) -> ()
	int l_vi_tmStatsReset(lua_State *L)
	{
		luaL_checktype(L, 1, LUA_TTABLE);
		vi_tmStats_t s{};
		vi_tmStatsReset(&s);
		// overwrite Lua table with reset values
		lua_pushinteger(L, (lua_Integer)s.calls_); lua_setfield(L, 1, "calls");
#if VI_TM_STAT_USE_RAW
		lua_pushinteger(L, (lua_Integer)s.cnt_);   lua_setfield(L, 1, "cnt");
		lua_pushinteger(L, (lua_Integer)s.sum_);   lua_setfield(L, 1, "sum");
#endif
#if VI_TM_STAT_USE_RMSE
		lua_pushinteger(L, (lua_Integer)s.flt_calls_); lua_setfield(L, 1, "flt_calls");
		lua_pushnumber(L, (lua_Number)s.flt_cnt_);     lua_setfield(L, 1, "flt_cnt");
		lua_pushnumber(L, (lua_Number)s.flt_avg_);     lua_setfield(L, 1, "flt_avg");
		lua_pushnumber(L, (lua_Number)s.flt_ss_);      lua_setfield(L, 1, "flt_ss");
#endif
#if VI_TM_STAT_USE_MINMAX
		lua_pushnumber(L, (lua_Number)s.min_);         lua_setfield(L, 1, "min");
		lua_pushnumber(L, (lua_Number)s.max_);         lua_setfield(L, 1, "max");
#endif
		return 0;
	}

	// vi_tmStatsIsValid(stats_table) -> result(int)
	int l_vi_tmStatsIsValid(lua_State *L)
	{
		luaL_checktype(L, 1, LUA_TTABLE);
		vi_tmStats_t s{};
		// minimal: calls
		lua_getfield(L, 1, "calls"); if (lua_isinteger(L, -1)) s.calls_ = (VI_TM_SIZE)lua_tointeger(L, -1); lua_pop(L, 1);
		VI_TM_RESULT r = vi_tmStatsIsValid(&s);
		lua_pushinteger(L, r);
		return 1;
	}

	// vi_tmStaticInfo(info_flag) -> number|string|lightuserdata|nil
	int l_vi_tmStaticInfo(lua_State *L)
	{
		VI_TM_FLAGS info = (VI_TM_FLAGS)luaL_checkinteger(L, 1);
		const void *p = vi_tmStaticInfo(info);
		if (!p) { lua_pushnil(L); return 1; }
		// Heuristic: try to interpret some known types if user asks
		// Expose as lightuserdata; advanced parsing can be done from Lua-side by additional helpers.
		lua_pushlightuserdata(L, const_cast<void *>(p));
		return 1;
	}

	// vi_tmReportCb(str[, ignored]) -> result(int)
	// Exposes default C callback as a callable Lua function.
	int l_vi_tmReportCb(lua_State *L)
	{
		const char *s = luaL_checkstring(L, 1);
		void *ignored = nullptr;
		if (!lua_isnoneornil(L, 2))
		{
			ignored = lua_touserdata(L, 2); // optional context pointer
		}
		VI_TM_RESULT r = vi_tmReportCb(s, ignored);
		lua_pushinteger(L, r);
		return 1;
	}

	// vi_tmRegistryReport(reg[, flags[, lua_fn]]) -> result(int)
	// If lua_fn provided, each line is forwarded to that Lua function as (str).
	int l_vi_tmRegistryReport(lua_State *L)
	{
		VI_TM_HREG j = lua_check_reg(L, 1);
		VI_TM_FLAGS flags = (VI_TM_FLAGS)luaL_optinteger(L, 2, vi_tmReportDefault);
		LuaReportCtx ctx{ L, LUA_REFNIL };

		vi_tmReportCb_t cb = vi_tmReportCb;
		void *cb_ctx = nullptr;

		if (!lua_isnoneornil(L, 3))
		{
			luaL_checktype(L, 3, LUA_TFUNCTION);
			lua_pushvalue(L, 3);
			ctx.fn_ref = luaL_ref(L, LUA_REGISTRYINDEX);
			cb = lua_report_cb;
			cb_ctx = &ctx;
		}

		VI_TM_RESULT r = vi_tmRegistryReport(j, flags, cb, cb_ctx);

		if (ctx.fn_ref != LUA_REFNIL)
		{
			luaL_unref(L, LUA_REGISTRYINDEX, ctx.fn_ref);
		}
		lua_pushinteger(L, r);
		return 1;
	}
} // namespace

// ------------------- Module registration -------------------

static const luaL_Reg vi_timing_funcs[] = {
	{"GetTicks",              l_vi_tmGetTicks},
	{"GlobalInit",            l_vi_tmGlobalInit},
	{"MeasurementAdd",        l_vi_tmMeasurementAdd},
	{"MeasurementGet",        l_vi_tmMeasurementGet},
	{"MeasurementMerge",      l_vi_tmMeasurementMerge},
	{"MeasurementReset",      l_vi_tmMeasurementReset},
	{"RegistryClose",         l_vi_tmRegistryClose},
	{"RegistryCreate",        l_vi_tmRegistryCreate},
	{"RegistryEnumerateMeas", l_vi_tmRegistryEnumerateMeas},
	{"RegistryGetMeas",       l_vi_tmRegistryGetMeas},
	{"RegistryReport",        l_vi_tmRegistryReport},
	{"RegistryReset",         l_vi_tmRegistryReset},
	{"ReportCb",              l_vi_tmReportCb},
	{"StaticInfo",            l_vi_tmStaticInfo},
	{"StatsAdd",              l_vi_tmStatsAdd},
	{"StatsIsValid",          l_vi_tmStatsIsValid},
	{"StatsMerge",            l_vi_tmStatsMerge},
	{"StatsReset",            l_vi_tmStatsReset},
	{nullptr, nullptr}
};

extern "C" int luaopen_vi_timing(lua_State* L) {
	luaL_newlib(L, vi_timing_funcs);

	// Push constants that might be useful
	lua_pushinteger(L, vi_tmReportDefault);
	lua_setfield(L, -2, "ReportDefault");

	lua_pushlightuserdata(L, (void*)VI_TM_HGLOBAL);
	lua_setfield(L, -2, "HGLOBAL");

	return 1;
}
