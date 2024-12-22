#version 150

in vec4 col;
out vec4 fragColor;

void main()
{
  // compute the final pixel color
  fragColor = col;
}