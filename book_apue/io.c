#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <locale.h>
#include <stddef.h>

#define BUFF_SIZE 4096

int main(void)
{
    setlocale(LC_ALL, "en_US.UTF-8");

    FILE *fs;
    wchar_t buf[BUFF_SIZE];

    fs = fopen("a.txt", "r");
    /* do something if open failed */

    /* at first */
    assert(fwide(fs, 0) == 0);

    wchar_t ch;
    size_t nread = 0;

    while ((ch = fgetwc(fs)) != WEOF)
        buf[nread++] = ch;

    buf[nread] = 0;

    /* after reading */
    assert(fwide(fs, 0) > 0);

    wprintf(L"content: %ls\nlength: %lu\n", buf, nread);

    /* after changing by hand */
    assert(fwide(fs, -1) > 0);
}