#ifndef _COMMON_H_
#define _COMMON_H_

#pragma once

#define ERROR_LABEL error

#define WIN32_FROM_HRESULT(hr)  \
    (SUCCEEDED(hr) ? ERROR_SUCCESS : \
        (HRESULT_FACILITY(hr) == FACILITY_WIN32 ? HRESULT_CODE(hr) : (hr)))

#define BAIL() \
    { \
        goto ERROR_LABEL; \
    }

#define BAIL_ON_ERROR(r) \
    if( ERROR_SUCCESS != (r)) \
    { \
        goto ERROR_LABEL; \
    }

#define BAIL_ON_SUCCESS(r) \
    if( ERROR_SUCCESS == (r)) \
    { \
        goto ERROR_LABEL; \
    }

#define BAIL_ON_APP_ERROR(r, errcode) \
    { \
        (r) = errcode; \
        goto ERROR_LABEL; \
    }

#define BAIL_ON_LAST_ERROR(r) \
    { \
        (r) = GetLastError(); \
        goto ERROR_LABEL; \
    }

#define BAIL_ON_HRESULT_ERROR(r, hr) \
    { \
        (r) = WIN32_FROM_HRESULT(hr); \
        goto ERROR_LABEL; \
    }

#define BAIL_ON_WIN32_ERROR(r, hr) \
    if( ERROR_SUCCESS != (r)) \
    { \
        (hr) = HRESULT_FROM_WIN32(r); \
        goto ERROR_LABEL; \
    }

#define BAIL_ON_LAST_WIN32_ERROR(hr) \
    { \
        (hr) = HRESULT_FROM_WIN32(GetLastError()); \
        goto ERROR_LABEL; \
    }

#define BAIL_ON_FAILURE(hr) \
    if( S_OK != (hr)) \
    { \
        goto ERROR_LABEL; \
    }

#endif  // _COMMON_H_