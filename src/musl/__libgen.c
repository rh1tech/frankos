#include <string.h>
#include "internal/__libgen.h"
#include "sys_table.h"

char* __libc() __dirname(char *s)
{
	size_t i;
	if (!s || !*s) return ".";
	i = strlen(s)-1;
	for (; s[i]=='/'; i--) if (!i) return "/";
	for (; s[i]!='/'; i--) if (!i) return ".";
	for (; s[i]=='/'; i--) if (!i) return "/";
	s[i+1] = 0;
	return s;
}

char* __libc() __basename(char *s)
{
	size_t i;
	if (!s || !*s) return ".";
	i = strlen(s)-1;
	for (; i&&s[i]=='/'; i--) s[i] = 0;
	for (; i&&s[i-1]!='/'; i--);
	return s+i;
}

weak_alias(__basename, __xpg_basename);

#include "cmd.h"

const char* __libc() __getprogname(void) {
	cmd_ctx_t* ctx = get_cmd_ctx();
	if (!ctx || ctx->argc < 1 || !ctx->argv[0]) return "null";
	return __basename(ctx->argv[0]);
}
