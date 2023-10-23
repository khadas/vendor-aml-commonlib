#include "util.h"

/* return 0 for match, nonzero for no match */
int startsWith(const char *s, const char *prefix)
{
	return strncmp(s, prefix, strlen(prefix));
}

/* return 0 for match, nonzero for no match */
int endWith(const char *s, const char *t)
{
    size_t slen = strlen(s);
    size_t tlen = strlen(t);
    if (tlen > slen) return 1;
    return strcmp(s + slen - tlen, t);
}
bool contains(const char *str, const char *sub)
{
	if (strstr(str, sub) != NULL) {
		return true;
	}
	return false;
}