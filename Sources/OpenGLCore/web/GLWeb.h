#pragma once

#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>

#include <cstdlib>

#ifndef APIENTRY
#define APIENTRY
#endif

struct GladWebVersion
{
    int major;
    int minor;
};

inline GladWebVersion GLVersion = { 3, 0 };

using GLADloadproc = void * (*)(char const *);

inline int gladLoadGL()
{
    return 1;
}

#ifndef GL_TEXTURE_1D
#define GL_TEXTURE_1D GL_TEXTURE_2D
#endif

#ifndef GL_LINE_SMOOTH
#define GL_LINE_SMOOTH 0x0B20
#endif

#ifndef GL_LINE_SMOOTH_HINT
#define GL_LINE_SMOOTH_HINT 0x0C52
#endif

#ifndef GL_POLYGON_SMOOTH
#define GL_POLYGON_SMOOTH 0x0B41
#endif

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

#ifndef GL_POINT_SPRITE
#define GL_POINT_SPRITE 0x8861
#endif

#ifndef GL_VERTEX_PROGRAM_POINT_SIZE
#define GL_VERTEX_PROGRAM_POINT_SIZE 0x8642
#endif

#ifndef GL_LINE
#define GL_LINE 0x1B01
#endif

#ifndef GL_FILL
#define GL_FILL 0x1B02
#endif

#ifndef GL_WRITE_ONLY
#define GL_WRITE_ONLY 0x88B9
#endif

namespace glweb
{
    inline float PointSize = 1.0f;

    inline bool IsUnsupportedCapability(GLenum capability)
    {
        return capability == GL_LINE_SMOOTH
            || capability == GL_POLYGON_SMOOTH
            || capability == GL_MULTISAMPLE
            || capability == GL_POINT_SPRITE
            || capability == GL_VERTEX_PROGRAM_POINT_SIZE
            || capability == GL_TEXTURE_2D;
    }

    inline void Enable(GLenum capability)
    {
        if (!IsUnsupportedCapability(capability))
        {
            (::glEnable)(capability);
        }
    }

    inline void Disable(GLenum capability)
    {
        if (!IsUnsupportedCapability(capability))
        {
            (::glDisable)(capability);
        }
    }

    inline void Hint(GLenum target, GLenum mode)
    {
        if (target != GL_LINE_SMOOTH_HINT)
        {
            (::glHint)(target, mode);
        }
    }

    struct MappedBufferSlot
    {
        GLenum target;
        void * data;
        GLint size;
    };

    inline MappedBufferSlot MappedBufferSlots[4] = {};

    inline void * MapBuffer(GLenum target)
    {
        GLint size = 0;
        glGetBufferParameteriv(target, GL_BUFFER_SIZE, &size);

        for (auto & slot : MappedBufferSlots)
        {
            if (slot.data == nullptr)
            {
                slot.target = target;
                slot.size = size;
                slot.data = std::calloc(1, size > 0 ? static_cast<size_t>(size) : 1u);
                return slot.data;
            }
        }

        return nullptr;
    }

    inline GLboolean UnmapBuffer(GLenum target)
    {
        for (auto & slot : MappedBufferSlots)
        {
            if (slot.data != nullptr && slot.target == target)
            {
                glBufferSubData(target, 0, slot.size, slot.data);
                std::free(slot.data);
                slot.data = nullptr;
                return GL_TRUE;
            }
        }

        return GL_FALSE;
    }
}

#define glEnable(capability) glweb::Enable(capability)
#define glDisable(capability) glweb::Disable(capability)
#define glHint(target, mode) glweb::Hint(target, mode)
#define glMapBuffer(target, access) glweb::MapBuffer(target)
#define glUnmapBuffer(target) glweb::UnmapBuffer(target)

inline void glPolygonMode(GLenum, GLenum)
{
}

inline void glPointSize(GLfloat size)
{
    glweb::PointSize = size;
}

inline void glTexImage1D(
    GLenum,
    GLint level,
    GLint internalformat,
    GLsizei width,
    GLint border,
    GLenum format,
    GLenum type,
    const void * pixels)
{
    glTexImage2D(GL_TEXTURE_2D, level, internalformat, width, 1, border, format, type, pixels);
}

inline void glTexSubImage1D(
    GLenum,
    GLint level,
    GLint xoffset,
    GLsizei width,
    GLenum format,
    GLenum type,
    const void * pixels)
{
    glTexSubImage2D(GL_TEXTURE_2D, level, xoffset, 0, width, 1, format, type, pixels);
}
