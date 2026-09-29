#include <GL4ES/gl.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations of TinyGL functions
extern void glArrayElement(GLint i);
extern void glEnd(void);
extern void glopMultMatrix(void *p);

// 1. glDrawElements implementation for TinyGL
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {
    if (count <= 0 || indices == NULL)
        return;
    glBegin(mode);
    if (type == GL_UNSIGNED_SHORT) {
        const GLushort *idx = (const GLushort *)indices;
        for (GLsizei i = 0; i < count; i++) {
            glArrayElement(idx[i]);
        }
    } else if (type == GL_UNSIGNED_BYTE) {
        const GLubyte *idx = (const GLubyte *)indices;
        for (GLsizei i = 0; i < count; i++) {
            glArrayElement(idx[i]);
        }
    } else {
        const GLuint *idx = (const GLuint *)indices;
        for (GLsizei i = 0; i < count; i++) {
            glArrayElement(idx[i]);
        }
    }
    glEnd();
}

// 2. glOrtho implementation for TinyGL
void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val) {
    GLfloat m[16];
    memset(m, 0, sizeof(m));
    m[0]  = (GLfloat)(2.0 / (right - left));
    m[5]  = (GLfloat)(2.0 / (top - bottom));
    m[10] = (GLfloat)(-2.0 / (far_val - near_val));
    m[12] = (GLfloat)(-(right + left) / (right - left));
    m[13] = (GLfloat)(-(top + bottom) / (top - bottom));
    m[14] = (GLfloat)(-(far_val + near_val) / (far_val - near_val));
    m[15] = 1.0f;
    glMultMatrixf(m);
}

void glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val) {
    glOrtho(left, right, bottom, top, near_val, far_val);
}

void glFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val) {
    glFrustum(left, right, bottom, top, near_val, far_val);
}

// 3. Stubs for multi-texturing (TinyGL uses single texture unit)
void glActiveTexture(GLenum texture) {
    (void)texture;
}

void glClientActiveTexture(GLenum texture) {
    (void)texture;
}

// 4. Stubs for texture parameter & environment
void glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
    glTexParameteri(target, pname, (GLint)param);
}

void glTexEnvf(GLenum target, GLenum pname, GLfloat param) {
    glTexEnvi(target, pname, (GLint)param);
}

void glPixelStorei(GLenum pname, GLint param) {
    (void)pname;
    (void)param;
}

void glAlphaFunc(GLenum func, GLclampf ref) {
    (void)func;
    (void)ref;
}

void glLineWidth(GLfloat width) {
    (void)width;
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    (void)red; (void)green; (void)blue; (void)alpha;
}

void glDepthFunc(GLenum func) {
    (void)func;
}

GLboolean glIsEnabled(GLenum cap) {
    (void)cap;
    return GL_FALSE;
}

void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat,
                            GLsizei width, GLsizei height, GLint border,
                            GLsizei imageSize, const GLvoid *data) {
    (void)target; (void)level; (void)internalformat;
    (void)width; (void)height; (void)border; (void)imageSize; (void)data;
}

// 5. OES matrix palette stubs (for skinned mesh fallback)
void glCurrentPaletteMatrixOES(GLuint matrixpaletteindex) {
    (void)matrixpaletteindex;
}

void glLoadPaletteFromModelViewMatrixOES(void) {
}

void glMatrixIndexPointerOES(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    (void)size; (void)type; (void)stride; (void)pointer;
}

void glWeightPointerOES(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    (void)size; (void)type; (void)stride; (void)pointer;
}

#ifdef __cplusplus
}
#endif
