#ifndef _EBO_H_
#define _EBO_H_

#include "openGLHeader.h"

/*
	"Element Buffer Object" (EBO) helper class.
	It creates and manages EBOs.
	Designed based on EBO.
*/

class EBO
{
public:

	EBO() : numIndices(0) {}
	virtual ~EBO();

	// Initialize the EBO.
    // "data": pointer to index data.
    // "size": size of the index data in bytes.
    // "usage": must be either GL_STATIC_DRAW or GL_DYNAMIC_DRAW.
	void Gen(GLsizeiptr size, const void* data, GLenum usage = GL_STATIC_DRAW);

	// Binds (activates) this EBO.
	void Bind();

	// Get handle to this EBO.
	GLuint GetHandle() { return handle; }
	// Get the number of vertices in this EBO.
	int GetNumIndices() { return numIndices; }

protected:
	GLuint handle; // the handle to the EBO
	int numIndices;
};

#endif

