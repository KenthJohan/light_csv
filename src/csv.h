/* (c) 2019 Jan Doczy
 * This code is licensed under MIT license (see LICENSE.txt for details) */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "csv_parsing.h"

/* Windows specific */
#ifdef _WIN32
//https://stackoverflow.com/questions/5440551/why-win32-winnt-0x400-in-qt-creator-on-windows-7-machine
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#include <Windows.h>
#include <memoryapi.h>
typedef unsigned long long file_off_t;
#else
#include <sys/types.h>
typedef off_t file_off_t;
#endif

/* max allowed buffer */
#define BUFFER_WIDTH_APROX (40 * 1024 * 1024)



/* private csv handle:
 * @mem: pointer to memory
 * @pos: position in buffer
 * @size: size of memory chunk
 * @context: context used when processing cols
 * @blockSize: size of mapped block
 * @fileSize: size of opened file
 * @mapSize: ...
 * @auxbuf: auxiliary buffer
 * @auxbufSize: size of aux buffer
 * @auxbufPos: position of aux buffer reader
 * @quotes: number of pending quotes parsed
 * @fh: file handle - descriptor
 * @delim: delimeter - ','
 * @quote: quote '"'
 * @escape: escape char
 */
struct CsvHandle_
{
    void* mem;
    size_t pos;
    size_t size;
    char* context;
    size_t blockSize;
    file_off_t fileSize;
    file_off_t mapSize;
    size_t auxbufSize;
    size_t auxbufPos;
    size_t quotes;
    void* auxbuf;
    
#if defined ( __unix__ )
    int fh;
#elif defined ( _WIN32 )
    HANDLE fh;
    HANDLE fm;
#else
    #error Wrong platform definition
#endif

    char delim;
    char quote;
    char escape;
};



/* trivial macro used to get page-aligned buffer size */
#define GET_PAGE_ALIGNED( orig, page ) \
    (((orig) + ((page) - 1)) & ~((page) - 1))

/* thin platform dependent layer so we can use file mapping
 * with winapi and oses following posix specs.
 */
#ifdef __unix__
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

CsvHandle CsvOpen2(const char* filename,
                   char delim,
                   char quote,
                   char escape)
{
    /* alloc zero-initialized mem */
    long pageSize;
    struct stat fs;

    CsvHandle handle = calloc(1, sizeof(struct CsvHandle_));
    if (!handle)
        goto fail;

    /* set chars */
    handle->delim = delim;
    handle->quote = quote;
    handle->escape = escape;

    /* page size */
    pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize < 0)
        goto fail;

    /* align to system page size */
    handle->blockSize = GET_PAGE_ALIGNED(BUFFER_WIDTH_APROX, pageSize);
    
    /* open new fd */
    handle->fh = open(filename, O_RDONLY);
    if (handle->fh < 0)
        goto fail;

    /* get real file size */
    if (fstat(handle->fh, &fs))
    {
       close(handle->fh);
       goto fail;
    }
    
    handle->fileSize = fs.st_size;
    return handle;
    
  fail:
    free(handle);
    return NULL;
}

static void* MapMem(CsvHandle handle)
{
    handle->mem = mmap(0, handle->blockSize,
                       PROT_READ | PROT_WRITE,
                       MAP_PRIVATE,
                       handle->fh, handle->mapSize);
    return handle->mem;
}

static void UnmapMem(CsvHandle handle)
{
    if (handle->mem)
        munmap(handle->mem, handle->blockSize);
}

void CsvClose(CsvHandle handle)
{
    if (!handle)
        return;

    UnmapMem(handle);

    close(handle->fh);
    free(handle->auxbuf);
    free(handle);
}

#else

/* extra Windows specific implementations
 */
void CsvOpen2(struct CsvHandle_ * handle, const char* filename)
{
    LARGE_INTEGER fsize;
	SYSTEM_INFO info;

    GetSystemInfo(&info);
    handle->blockSize = GET_PAGE_ALIGNED(BUFFER_WIDTH_APROX, info.dwPageSize);
	handle->fh = CreateFileA(filename,
                            GENERIC_READ, 
                            FILE_SHARE_READ, 
                            NULL, 
                            OPEN_EXISTING, 
                            FILE_ATTRIBUTE_NORMAL, 
                            NULL);

	if (handle->fh == INVALID_HANDLE_VALUE)
	{
		goto fail;
	}

	if (GetFileSizeEx(handle->fh, &fsize) == FALSE)
	{
		goto fail;
	}

    handle->fileSize = fsize.QuadPart;
	if (!handle->fileSize)
	{
		goto fail;
	}

    handle->fm = CreateFileMapping(handle->fh, NULL, PAGE_WRITECOPY, 0, 0, NULL);
    if (handle->fm == NULL)
	{
		goto fail;
	}

	return;

fail:
    if (handle->fh != INVALID_HANDLE_VALUE)
	{
        CloseHandle(handle->fh);
	}
	{
		wchar_t buf[256];
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		buf, (sizeof(buf) / sizeof(wchar_t)), NULL);
		wprintf(L"%S\n", buf);
	}
    free(handle);
	return;
}

static void* MapMem(struct CsvHandle_ * handle)
{
    size_t size = handle->blockSize;
    if (handle->mapSize + size > handle->fileSize)
        size = 0;  /* last chunk, extend to file mapping max */

    handle->mem = MapViewOfFileEx(handle->fm, 
                                  FILE_MAP_COPY,
                                  (DWORD)(handle->mapSize >> 32),
                                  (DWORD)(handle->mapSize & 0xFFFFFFFF),
                                  size,
                                  NULL);
    return handle->mem;
}

static void UnmapMem(struct CsvHandle_ * handle)
{
    if (handle->mem)
	{
		UnmapViewOfFileEx(handle->mem, 0);
	}
}

void CsvClose(struct CsvHandle_ * handle)
{
    if (!handle)
        return;

    UnmapMem(handle);

    CloseHandle(handle->fm);
    CloseHandle(handle->fh);
    free(handle->auxbuf);
    free(handle);
}

#endif



void CsvOpen(struct CsvHandle_ * handle, const char* filename)
{
	CsvOpen2(handle, filename);
}

static int CsvEnsureMapped(struct CsvHandle_ * handle)
{
    file_off_t newSize;
    
    /* do not need to map */
    if (handle->pos < handle->size)
        return 0;

    UnmapMem(handle);  

    handle->mem = NULL;
    if (handle->mapSize >= handle->fileSize)
        return -EINVAL;

    newSize = handle->mapSize + handle->blockSize;
    if (MapMem(handle))
    {
        handle->pos = 0;
        handle->mapSize = newSize;

        /* read only up to filesize:
         * 1. mapped block size is < then filesize: (use blocksize)
         * 2. mapped block size is > then filesize: (use remaining filesize) */
        handle->size = handle->blockSize;
        if (handle->mapSize > handle->fileSize)
            handle->size = (size_t)(handle->fileSize % handle->blockSize);
        
        return 0;
    }
    
    return -ENOMEM;
}

static char* CsvChunkToAuxBuf(struct CsvHandle_ * handle, char* p, size_t size)
{
    size_t newSize = handle->auxbufPos + size + 1;
    if (handle->auxbufSize < newSize)
    {
        void* mem = realloc(handle->auxbuf, newSize);
        if (!mem)
            return NULL;

        handle->auxbuf = mem;
        handle->auxbufSize = newSize;
    }

    memcpy((char*)handle->auxbuf + handle->auxbufPos, p, size);
    handle->auxbufPos += size;
    
    *(char*)((char*)handle->auxbuf + handle->auxbufPos) = '\0';
    return handle->auxbuf;
}




char* CsvReadNextRow(struct CsvHandle_ * handle)
{
    size_t size;
    char* p = NULL;
    char* found = NULL;

    do
    {
        int err = CsvEnsureMapped(handle);
        handle->context = NULL;
        
        if (err == -EINVAL)
        {
            /* if this is n-th iteration
             * return auxbuf (remaining bytes of the file) */
            if (p == NULL)
                break;

            return handle->auxbuf;
        }
        else if (err == -ENOMEM)
        {
            break;
        }
        
        size = handle->size - handle->pos;
        if (!size)
            break;

        /* search this chunk for NL */
        p = (char*)handle->mem + handle->pos;
		found = csv_search_linefeed(p, size, handle->quote, &(handle->quotes));

        if (found)
        {
            /* prepare position for next iteration */
            size = (size_t)(found - p) + 1;
            handle->pos += size;
            handle->quotes = 0;
            
            if (handle->auxbufPos)
            {
                if (!CsvChunkToAuxBuf(handle, p, size))
                    break;
                
                p = handle->auxbuf;
                size = handle->auxbufPos;
            }

            /* reset auxbuf position */
            handle->auxbufPos = 0;

            /* terminate line */
			csv_terminate_line(p + size - 1, size);
            return p;
        }
        else
        {
            /* reset on next iteration */
            handle->pos = handle->size;
        }

        /* correctly process boundries, storing
         * remaning bytes in aux buffer */
        if (!CsvChunkToAuxBuf(handle, p, size))
            break;

    } while (!found);

    return NULL;
}







const char* CsvReadNextCol(char* row, struct CsvHandle_ * handle)
{
	return csv_read_next_column(row, handle->delim, handle->quote, handle->escape, &handle->context);
}
