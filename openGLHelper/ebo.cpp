#include "ebo.h"

#include <cstring>
#include <cstdio>
#include <iostream>

using namespace std;

EBO::~EBO()
{
  // Delete the EBO.
  glDeleteBuffers(1, &handle);
}

void EBO::Gen(GLsizeiptr size, const void* data, GLenum usage) 
{
  // Create the EBO handle 
  glGenBuffers(1, &handle);

  // Initialize the EBO.
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, usage);

  // Calculate the number of indices.
  numIndices = static_cast<int>(size / sizeof(unsigned int));

}

void EBO::Bind()
{
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle);
}

