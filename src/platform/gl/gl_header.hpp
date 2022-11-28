#pragma once

#ifdef PLATFORM_ANDROID
#define USE_GLES3
#endif

#if defined(PLATFORM_SWITCH)
#include <glad/glad.h>
#elif defined(PLATFORM_PS4)
#define __PIGLET__
#include <orbis/Pigletv2VSH.h>
#elif defined(USE_GLES3)
#include <GLES3/gl3.h>
#elif defined(USE_GLES2)
#include <GLES2/gl2.h>
#elif defined(USE_GL1)
#include <GLFW/glfw3.h>
#else
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#endif

#ifndef glGenFramebuffersEXT
#define glGenFramebuffersEXT glGenFramebuffers
#define glDeleteFramebuffersEXT glDeleteFramebuffers
#define glCheckFramebufferStatusEXT glCheckFramebufferStatus
#define glFramebufferTexture2DEXT glFramebufferTexture2D
#define glBindFramebufferEXT glBindFramebuffer
//#define glDrawBuffers
#endif
