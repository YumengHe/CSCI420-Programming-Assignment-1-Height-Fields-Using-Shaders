#version 150

in vec3 position;
in vec3 positionUnscaled;
in vec4 color;
in vec4 colorImage;
out vec4 col;

// mode
uniform int mode;

// For mode 1
in vec3 PleftPosition, PrightPosition, PdownPosition, PupPosition;
uniform float scale;
uniform float exponent;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

// for color
uniform int colorMode; // 0 for original color, 1 for color image

// for wireframe
uniform int useOverrideColor;


void main()
{
  if(mode == 1)
  { 
    // Use unscaled y-values
    float y_center = positionUnscaled.y;
    float y_left = PleftPosition.y;
    float y_right = PrightPosition.y;
    float y_down = PdownPosition.y;
    float y_up = PupPosition.y;

    // Computer the smoothed y-coordiante
    float y_smoothed = (y_center + y_left + y_right + y_down + y_up) / 5.0;

    // Ensure y is possitive
    y_smoothed = max(y_smoothed, 0.00001);

    // Change the vertex color and height
    float outputColor = pow(y_smoothed, exponent);
    y_smoothed = scale * pow(y_smoothed, exponent);
    col = vec4(outputColor, outputColor, outputColor, 1.0);

    // Compute the transformed and projected vertex position
    vec4 transformedPosition = vec4(positionUnscaled.x, y_smoothed, positionUnscaled.z, 1.0);
    gl_Position = projectionMatrix * modelViewMatrix * transformedPosition;
  }
  else if(mode == 0)
  {
  	gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
  	col = color;
  }

  // apply color from color image
  if (colorMode == 1)
  {
    col = colorImage * color;
  }
  else
  {
    col = color;
  }

  // change the color for wireframe
  if (useOverrideColor == 1)
  {
    col = color * vec4(0.7, 0.3, 0.7, 0.6);
  }

}

