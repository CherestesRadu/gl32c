#include <windows.h>
#include <gl/gl.h>

/*
	Example:
	
	#define GL32C_IMPLEMENTATION
	#include "gl32c.h"
	
	int main(void)
	{
		...
		
		gl32c_t ct = {0};
		DWORD result = gl32c_create_context(valid_window_handle, &ct, 3, 3)
		if(result != 0) // Error has occured, check GetLastError() codes
		{
			printf("Error\n");
		}
		else // Success
		{
			const GLbyte *version = glGetString(GL_VERSION);
			printf(version);
		}
	}
	
*/

#ifndef WGL_DRAW_TO_WINDOW_ARB
	#define WGL_DRAW_TO_WINDOW_ARB                    0x2001
#endif

#ifndef WGL_ACCELERATION_ARB
	#define WGL_ACCELERATION_ARB                      0x2003
#endif

#ifndef WGL_SUPPORT_OPENGL_ARB
	#define WGL_SUPPORT_OPENGL_ARB                    0x2010
#endif

#ifndef WGL_DOUBLE_BUFFER_ARB
	#define WGL_DOUBLE_BUFFER_ARB                     0x2011
#endif

#ifndef WGL_PIXEL_TYPE_ARB
	#define WGL_PIXEL_TYPE_ARB                        0x2013
#endif

#ifndef WGL_COLOR_BITS_ARB
	#define WGL_COLOR_BITS_ARB                        0x2014
#endif

#ifndef WGL_DEPTH_BITS_ARB
	#define WGL_DEPTH_BITS_ARB                        0x2022
#endif

#ifndef WGL_STENCIL_BITS_ARB
	#define WGL_STENCIL_BITS_ARB                      0x2023
#endif

#ifndef WGL_FULL_ACCELERATION_ARB
	#define WGL_FULL_ACCELERATION_ARB                 0x2027
#endif

#ifndef WGL_TYPE_RGBA_ARB
	#define WGL_TYPE_RGBA_ARB                         0x202B
#endif

#ifndef WGLContext_MAJOR_VERSION_ARB
	#define WGLContext_MAJOR_VERSION_ARB             0x2091
#endif

#ifndef WGLContext_MINOR_VERSION_ARB
	#define WGLContext_MINOR_VERSION_ARB             0x2092
#endif

#ifndef WGLContext_PROFILE_MASK_ARB
	#define WGLContext_PROFILE_MASK_ARB              0x9126
#endif

#ifndef WGLContext_CORE_PROFILE_BIT_ARB
	#define WGLContext_CORE_PROFILE_BIT_ARB          0x00000001
#endif

typedef HGLRC WINAPI wglCreateContextAttribsARB_type(HDC hdc, HGLRC hShareContext,
	const int *attribList);

typedef BOOL WINAPI wglChoosePixelFormatARB_type(HDC hdc, const int *piAttribIList,
	const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);

typedef struct gl32c // struct containing a compatible device context and opengl context
{
	HDC device_context;
	HGLRC opengl_context;
} gl32c_t;

void  gl32c_zero_init(void *memory, unsigned long size);
DWORD gl32c_create_context(HWND valid_window_handle, gl32c_t *ct, int major, int minor);

#ifdef GL32C_IMPLEMENTATION
wglCreateContextAttribsARB_type* wglCreateContextAttribsARB = 0;
wglChoosePixelFormatARB_type* 	 wglChoosePixelFormatARB = 0;

void 
gl32c_zero_init(void *memory, unsigned long size)
{
	unsigned char *data = (unsigned char*)memory;
	for(int i = 0; i < size; ++i)
	{
		data[i] = 0;
	}
}

// Creates an OpenGL Core Profile with specified version
// Returns ERROR_SUCCESS(0x0) on succes and a Win32 GetLastError() code if it fails.
DWORD 
gl32c_create_context(HWND valid_window_handle, gl32c_t *ct, int major, int minor)
{
	// Create proxy context to get wglCreateContextAttribsARB
	// and wglChoosePixelFormatARB
	{
		HINSTANCE instance = GetModuleHandle(0);
		WNDCLASS wc;
		gl32c_zero_init(&wc, sizeof(wc));
		wc.style = CS_OWNDC;
		wc.hInstance = instance;
		wc.hCursor = LoadCursor(0, IDC_ARROW);
		wc.lpszClassName = "GL32CProxyOpenGLClass";
		wc.lpfnWndProc = DefWindowProc;
	
		if (!RegisterClass(&wc))
		{
			return GetLastError();
		}
		
	
		HWND window = CreateWindowEx(0, "GL32CProxyOpenGLClass", 0, WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			0, 0, wc.hInstance, 0);
	
		if (!window)
		{
			UnregisterClass("GL32CProxyOpenGLClass", instance);
			return GetLastError();
		}
	
		HDC dc = GetDC(window);
		int format = 0;
		PIXELFORMATDESCRIPTOR pfd = {};
		pfd.nSize = sizeof(pfd);
		pfd.nVersion = 1;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.cColorBits = 32;
		pfd.cAlphaBits = 8;
		pfd.iLayerType = PFD_MAIN_PLANE;
		pfd.cDepthBits = 24;
		pfd.cStencilBits = 8;
		format = ChoosePixelFormat(dc, &pfd);
		if (!format)
		{
			DestroyWindow(window);
			UnregisterClass("GL32CProxyOpenGLClass", instance);
			return GetLastError();
		}
		
		BOOL success = SetPixelFormat(dc, format, &pfd);
		if (!success)
		{
			DestroyWindow(window);
			UnregisterClass("GL32CProxyOpenGLClass", instance);
			return GetLastError();
		}
		
		HGLRC gc = wglCreateContext(dc);
		if (!gc)
		{
			DestroyWindow(window);
			UnregisterClass("GL32CProxyOpenGLClass", instance);
			return GetLastError();
		}
	
		if (!wglMakeCurrent(dc, gc))
		{
			wglDeleteContext(gc);
			ReleaseDC(window, dc);
			DestroyWindow(window);
			UnregisterClass("GL32CProxyOpenGLClass", instance);
			return GetLastError();
		}
	
		wglCreateContextAttribsARB = (wglCreateContextAttribsARB_type*)wglGetProcAddress("wglCreateContextAttribsARB");
		wglChoosePixelFormatARB = (wglChoosePixelFormatARB_type*)wglGetProcAddress("wglChoosePixelFormatARB");
	
		if (!wglCreateContextAttribsARB)
		{
			wglMakeCurrent(dc, 0);
			wglDeleteContext(gc);
			ReleaseDC(window, dc);
			DestroyWindow(window);
			UnregisterClass("GL32CProxyOpenGLClass", wc.hInstance);
			return GetLastError();
		}
	
		if (!wglChoosePixelFormatARB)
		{
			wglMakeCurrent(dc, 0);
			wglDeleteContext(gc);
			ReleaseDC(window, dc);
			DestroyWindow(window);
			UnregisterClass("GL32CProxyOpenGLClass", wc.hInstance);
			return GetLastError();
		}
	
		wglMakeCurrent(dc, 0);
		wglDeleteContext(gc);
		ReleaseDC(window, dc);
		DestroyWindow(window);
		UnregisterClass("GL32CProxyOpenGLClass", wc.hInstance);
	}
	
	int attribs[] = {
		WGL_DRAW_TO_WINDOW_ARB,     GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB,     GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB,      GL_TRUE,
		WGL_ACCELERATION_ARB,       WGL_FULL_ACCELERATION_ARB,
		WGL_PIXEL_TYPE_ARB,         WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB,         32,
		WGL_DEPTH_BITS_ARB,         24,
		WGL_STENCIL_BITS_ARB,       8,
		0
	};

	ct->device_context = GetDC(valid_window_handle);

	int pixel_format = 0;
	UINT num_formats = 0;
	if (!wglChoosePixelFormatARB(ct->device_context, attribs, 0, 1, &pixel_format, &num_formats))
	{
		ReleaseDC(valid_window_handle, ct->device_context);
		return GetLastError();
	}

	PIXELFORMATDESCRIPTOR pfd;
	gl32c_zero_init(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
	if (!DescribePixelFormat(ct->device_context, pixel_format, sizeof(pfd), &pfd))
	{
		ReleaseDC(valid_window_handle, ct->device_context);
		return GetLastError();
	}

	if (!SetPixelFormat(ct->device_context, pixel_format, &pfd))
	{
		ReleaseDC(valid_window_handle, ct->device_context);
		return GetLastError();
	}

	int GL33Attribs[] = 
	{
		WGLContext_MAJOR_VERSION_ARB, major,
		WGLContext_MINOR_VERSION_ARB, minor,
		WGLContext_PROFILE_MASK_ARB,  WGLContext_CORE_PROFILE_BIT_ARB,
		0,
	};

	ct->opengl_context = wglCreateContextAttribsARB(ct->device_context, 0, GL33Attribs);
	if (!ct->opengl_context)
	{
		ReleaseDC(valid_window_handle, ct->device_context);
		return GetLastError();
	}

	if (!wglMakeCurrent(ct->device_context, ct->opengl_context))
	{
		wglDeleteContext(ct->opengl_context);
		ReleaseDC(valid_window_handle, ct->device_context);
		return GetLastError();
	}
	
	return GetLastError();
}

#endif