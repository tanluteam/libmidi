#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "url.h"

typedef struct _URL_mem
{
    char common[sizeof(struct _URL)];
    char *memory;
    long memsiz;
    long mempos;
    int autofree;
} URL_mem;

static long url_mem_read(URL url, void *buff, long n);
static char *url_mem_gets(URL url, char *buff, int n);
static int url_mem_fgetc(URL url);
static long url_mem_seek(URL url, long offset, int whence);
static long url_mem_tell(URL url);
static void url_mem_close(URL url);

URL url_mem_open(char *memory, long memsiz, int autofree)
{
    URL_mem *url;

    url = (URL_mem *)alloc_url(sizeof(URL_mem));
    if (url == NULL)
    {
        url_errno = errno;
        if (autofree)
        {
            free(memory);
            errno = url_errno;
        }
        return NULL;
    }

    /* common members */
    URLm(url, type) = URL_mem_t;
    URLm(url, url_read) = url_mem_read;
    URLm(url, url_gets) = url_mem_gets;
    URLm(url, url_fgetc) = url_mem_fgetc;
    URLm(url, url_seek) = url_mem_seek;
    URLm(url, url_tell) = url_mem_tell;
    URLm(url, url_close) = url_mem_close;

    /* private members */
    url->memory = memory;
    url->memsiz = memsiz;
    url->mempos = 0;
    url->autofree = autofree;

    return (URL)url;
}

static long url_mem_read(URL url, void *buff, long n)
{
    URL_mem *urlp = (URL_mem *)url;
    long s;
    char *p = (char *)buff;

    s = urlp->memsiz - urlp->mempos;
    if (s > n)
        s = n;
    if (s <= 0)
        return 0;
    memcpy(p, urlp->memory + urlp->mempos, s);
    urlp->mempos += s;
    return s;
}

static char *url_mem_gets(URL url, char *buff, int n)
{
    URL_mem *urlp = (URL_mem *)url;
    long s;
    char *nlp, *p;

    if (urlp->memsiz == urlp->mempos)
        return NULL;
    if (n <= 0)
        return buff;
    if (n == 1)
    {
        *buff = '\0';
        return buff;
    }
    n--; /* for '\0' */
    s = urlp->memsiz - urlp->mempos;
    if (s > n)
        s = n;
    p = urlp->memory + urlp->mempos;
    nlp = (char *)memchr(p, url_newline_code, s);
    if (nlp != NULL)
        s = nlp - p + 1;
    memcpy(buff, p, s);
    buff[s] = '\0';
    urlp->mempos += s;
    return buff;
}

static int url_mem_fgetc(URL url)
{
    URL_mem *urlp = (URL_mem *)url;

    if (urlp->memsiz == urlp->mempos)
        return EOF;
    return (int)(unsigned char)urlp->memory[urlp->mempos++];
}

static long url_mem_seek(URL url, long offset, int whence)
{
    URL_mem *urlp = (URL_mem *)url;
    long ret;

    ret = urlp->mempos;
    switch (whence)
    {
    case SEEK_SET:
        urlp->mempos = offset;
        break;
    case SEEK_CUR:
        urlp->mempos += offset;
        break;
    case SEEK_END:
        urlp->mempos = urlp->memsiz + offset;
        break;
    }
    if (urlp->mempos > urlp->memsiz)
        urlp->mempos = urlp->memsiz;
    else if (urlp->mempos < 0)
        urlp->mempos = 0;

    return ret;
}

static long url_mem_tell(URL url)
{
    return ((URL_mem *)url)->mempos;
}

static void url_mem_close(URL url)
{
    int save_errno = errno;
    URL_mem *urlp = (URL_mem *)url;
    if (urlp->autofree)
        free(urlp->memory);
    free(url);
    errno = save_errno;
}
