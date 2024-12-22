/*
  CSCI 420 Computer Graphics, Computer Science, USC
  Assignment 1: Height Fields with Shaders.
  C/C++ starter code

  Student username: heyumeng
*/

#include "openGLHeader.h"
#include "glutHeader.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "pipelineProgram.h"
#include "vbo.h"
#include "vao.h"
#include "ebo.h"

#include <iostream>
#include <cstring>
#include <memory>
#include <chrono>

#if defined(WIN32) || defined(_WIN32)
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#if defined(WIN32) || defined(_WIN32)
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char shaderBasePath[1024] = "../openGLHelper";
#endif

using namespace std;

int mousePos[2]; // x,y screen coordinates of the current mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// Transformations of the terrain.
float terrainRotate[3] = { 0.0f, 0.0f, 0.0f }; 
// terrainRotate[0] gives the rotation around x-axis (in degrees)
// terrainRotate[1] gives the rotation around y-axis (in degrees)
// terrainRotate[2] gives the rotation around z-axis (in degrees)
float terrainTranslate[3] = { 0.0f, 0.0f, 0.0f };
float terrainScale[3] = { 1.0f, 1.0f, 1.0f };

// Width and height of the OpenGL window, in pixels.
int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 Homework 1";

// Number of vertices in the single triangle (starter code).
int numVertices;
float zScale, zShift;

// CSCI 420 helper classes.
OpenGLMatrix matrix;
PipelineProgram pipelineProgram;

// pipeline
GLuint programHandle;
VAO VAOVertice;
VBO VBOPosition, VBOColor;
EBO EBOLineIndex, EBOTriangleIndex;

// for image
std::unique_ptr<ImageIO> heightmapImage = std::make_unique<ImageIO>();
int width, height, bytesPerPixel;
unsigned char * pixels;

// for vertices
std::vector<float> pointPosition, pointColor;
std::vector<unsigned int> lineIndex, triangleIndex;

// display option
typedef enum { POINTS, LINES, TRIANGLES, SMOOTH, WIREFRAME } DISPLAY_OPTION;
DISPLAY_OPTION displayOption = POINTS;
typedef enum { GRAYSCALE, COLOR } COLOR_STATE;
COLOR_STATE colorState = GRAYSCALE;

// mode 1
float scale = 64.0f;
float exponent = 1.0f;
std::vector<float> PcenterPosition;
std::vector<float> PleftPosition;
std::vector<float> PrightPosition;
std::vector<float> PdownPosition;
std::vector<float> PupPosition;
VBO PcenterVBO, PleftVBO, PrightVBO, PdownVBO, PupVBO;

// rotate
bool autoRotate = false;

// color image
std::unique_ptr<ImageIO> colorImage = std::make_unique<ImageIO>();
std::vector<float> pointColorImage;
VBO VBOColorImage;
int colorMode = 0; // 0 for original color, 1 for color image

// take 300 screenshots
bool isCapturing = false;           // capturing status
int screenshotFrame = 0;            // count screenshots
const int totalFrames = 300;        // take 300 screenshots in total
const int fps = 15;                 // 15 screenshots per second
std::chrono::steady_clock::time_point lastFrameTime; // timer

// replacement for CTRL
bool isTranslatingKeyPressed = false;

// camera
int lookAtZ = (width < 200)? 0 : (width < 500)? 128: (width < 700)? 448 : 640;


// Write a screenshot to the specified filename.
void saveScreenshot(const char * filename)
{
  std::unique_ptr<unsigned char[]> screenshotData = std::make_unique<unsigned char[]>(windowWidth * windowHeight * 3);
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData.get());

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData.get());

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;
}

void idleFunc()
{
  // capture 300 screenshot in 20 seconds, 15 screenshot per second
  // and name each screenshot from 000.jpg to 299.jpg
  // save all screenshot under "screenshot" folder
  if (isCapturing)
  {
    // Calculate the time elapsed since the last frame
    auto currentTime = std::chrono::steady_clock::now();
    std::chrono::duration<float> elapsedTime = currentTime - lastFrameTime;

    // Calculate the frame duration (in seconds)
    float frameDuration = 1.0f / fps;

    if (elapsedTime.count() >= frameDuration)
    {
      // Time to capture the next frame

      // Construct the filename
      char filename[256];
      sprintf(filename, "screenshot/%03d.jpg", screenshotFrame);

      // Take and save the screenshot
      saveScreenshot(filename);

      std::cout << "Saved screenshot: " << filename << std::endl;

      // Update the frame counter and last frame time
      screenshotFrame++;
      lastFrameTime = currentTime;

      // Check if we've reached the total number of frames
      if (screenshotFrame >= totalFrames)
      {
        isCapturing = false;
        std::cout << "Finished capturing screenshots." << std::endl;
      }
    }
  }

  // auto rotate the terrain when pressed "r"
  if (autoRotate)
    {
      // Increment the rotation angle around the y-axis
      terrainRotate[1] += 0.1f; // rotation speed

      // Keep the rotation angle within [0, 360) degrees
      if (terrainRotate[1] >= 360.0f)
        terrainRotate[1] -= 360.0f;
    }

  // Notify GLUT that it should call displayFunc.
  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);

  // When the window has been resized, we need to re-set our projection matrix.
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.LoadIdentity();
  // You need to be careful about setting the zNear and zFar. 
  // Anything closer than zNear, or further than zFar, will be culled.
  const float zNear = 0.1f;
  const float zFar = 10000.0f;
  const float humanFieldOfView = 60.0f;
  matrix.Perspective(humanFieldOfView, 1.0f * w / h, zNear, zFar);
}

// rescale the input color image to fit the terrain image
std::vector<unsigned char> rescaleColorImage(int newWidth, int newHeight)
{
    int originalWidth = colorImage->getWidth();
    int originalHeight = colorImage->getHeight();
    int channels = colorImage->getBytesPerPixel(); // Should be 3 for RGB or 4 for RGBA

    // Store the rescaled image data
    std::vector<unsigned char> rescaledData(newWidth * newHeight * channels);

    // Calculate scaling factors
    float xScale = static_cast<float>(originalWidth - 1) / (newWidth - 1);
    float yScale = static_cast<float>(originalHeight - 1) / (newHeight - 1);

    for (int newY = 0; newY < newHeight; ++newY)
    {
        for (int newX = 0; newX < newWidth; ++newX)
        {
            // Corresponding position in the original image
            float originalX = newX * xScale;
            float originalY = newY * yScale;

            // Get the integer and fractional parts
            int x0 = static_cast<int>(originalX);
            int y0 = static_cast<int>(originalY);
            int x1 = std::min(x0 + 1, originalWidth - 1);
            int y1 = std::min(y0 + 1, originalHeight - 1);

            float xLerp = originalX - x0;
            float yLerp = originalY - y0;

            for (int c = 0; c < channels; ++c)
            {
                // Get the pixel values at the four surrounding pixels
                float c00 = colorImage->getPixel(y0, x0, c);
                float c10 = colorImage->getPixel(y0, x1, c);
                float c01 = colorImage->getPixel(y1, x0, c);
                float c11 = colorImage->getPixel(y1, x1, c);

                // Perform bilinear interpolation
                float c0 = c00 * (1 - xLerp) + c10 * xLerp;
                float c1 = c01 * (1 - xLerp) + c11 * xLerp;
                float cValue = c0 * (1 - yLerp) + c1 * yLerp;

                // Store the interpolated value in the rescaled data
                rescaledData[(newY * newWidth + newX) * channels + c] = static_cast<unsigned char>(cValue);
            }
        }
    }

    return rescaledData;
}

void mouseMotionDragFunc(int x, int y)
{
  // Mouse has moved, and one of the mouse buttons is pressed (dragging).

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

  switch (controlState)
  {
    // translate the terrain
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        terrainTranslate[0] += mousePosDelta[0] * 0.10f;
        terrainTranslate[1] -= mousePosDelta[1] * 0.10f;
      }
      if (middleMouseButton)
      {
        // control z translation via the middle mouse button
        terrainTranslate[2] += mousePosDelta[1] * 0.10f;
      }
      break;

    // rotate the terrain
    case ROTATE:
      if (leftMouseButton)
      {
        // control x,y rotation via the left mouse button
        terrainRotate[0] += mousePosDelta[1]; // rotate around x-axis
        terrainRotate[1] += mousePosDelta[0]; // rotate around y-axis
      }
      if (middleMouseButton)
      {
        // control z rotation via the middle mouse button
        terrainRotate[2] += mousePosDelta[1]; // rotate around z-axis
      }
      break;

    // scale the terrain
    case SCALE:
      if (leftMouseButton)
      {
        // control x,y scaling via the left mouse button
        terrainScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
        terrainScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z scaling via the middle mouse button
        terrainScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // Mouse has moved.
  // Store the new mouse position.
  mousePos[0] = x;
  mousePos[1] = y;
}

// detect mouse input and CTRL, SHIFT
void mouseButtonFunc(int button, int state, int x, int y)
{
  // A mouse button has has been pressed or depressed.

  // Keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables.
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // Keep track of whether CTRL and SHIFT keys are pressed.
  // switch (glutGetModifiers())
  // {
  //   case GLUT_ACTIVE_CTRL:
  //     controlState = TRANSLATE;
  //   break;

  //   case GLUT_ACTIVE_SHIFT:
  //     controlState = SCALE;
  //   break;

  //   // If CTRL and SHIFT are not pressed, we are in rotate mode.
  //   default:
  //     controlState = ROTATE;
  //   break;
  // }

  int modifiers = glutGetModifiers();

  if (isTranslatingKeyPressed)
  {
    controlState = TRANSLATE;
  }
  else if (modifiers & GLUT_ACTIVE_SHIFT)
  {
    controlState = SCALE;
  }
  else
  {
    controlState = ROTATE;
  }

  // Store the new mouse position.
  mousePos[0] = x;
  mousePos[1] = y;
}

// detect key input
void keyboardFunc(unsigned char key, int x, int y)
{ 
  GLint loc = glGetUniformLocation(pipelineProgram.GetProgramHandle(), "mode");
  switch (key)
  {
    case 27: // ESC key
      exit(0); // exit the program
    break;

    case ' ':
      cout << "You pressed the spacebar." << endl;
    break;

    case 'x':
      // Take a screenshot.
      saveScreenshot("screenshot.jpg");
    break;

    case '1':
      // draw points
      displayOption = POINTS;
      pipelineProgram.SetUniformVariablei("mode", 0);
    break;
    case '2':
      // draw lines
      displayOption = LINES;
      pipelineProgram.SetUniformVariablei("mode", 0);
    break;

    case '3':
      // draw solid triangles
      displayOption = TRIANGLES;
      pipelineProgram.SetUniformVariablei("mode", 0);
    break;

    case '4':
      // draw smoothed triangles
      displayOption = SMOOTH;
      pipelineProgram.SetUniformVariablei("mode", 1);
      pipelineProgram.SetUniformVariablei("scale", scale);
      pipelineProgram.SetUniformVariablei("exponent", exponent);
    break;

    case '5':
      // draw wireframe on solid triangles
      displayOption = WIREFRAME;
      pipelineProgram.SetUniformVariablei("mode", 0);
    break;

    case '=':
      // increase scale
      scale *= 2.0f;
      std::cout << "Scale increased to: " << scale << std::endl;
      pipelineProgram.SetUniformVariablef("scale", scale);
    break;

    case '-':
      // decrease scale
      scale /= 2.0f;
      std::cout << "Scale decreased to: " << scale << std::endl;
      pipelineProgram.SetUniformVariablef("scale", scale);
    break;

    case '9':
      // increase exponent
      exponent *= 2.0f;
      std::cout << "Exponent increased to: " << exponent << std::endl;
      pipelineProgram.SetUniformVariablef("exponent", exponent);
    break;

    case '0':
      // decrease exponent
      exponent /= 2.0f;
      std::cout << "Exponent decreased to: " << exponent << std::endl;
      pipelineProgram.SetUniformVariablef("exponent", exponent);
    break;

    case 'r':
      // auto rotate
      autoRotate = !autoRotate;
      if (autoRotate)
          std::cout << "Automatic rotation enabled." << std::endl;
      else
          std::cout << "Automatic rotation disabled." << std::endl;
    break;

    case 'c':
      // color mode
      colorMode = 1 - colorMode;
      pipelineProgram.SetUniformVariablei("colorMode", colorMode);
      if (colorMode == 1)
        std::cout << "Using color image for terrain coloring." << std::endl;
      else
        std::cout << "Using original color for terrain coloring." << std::endl;
    break;

    case 's':
      // capture 300 screenshot
      if (!isCapturing)
      {
        isCapturing = true;
        screenshotFrame = 0;
        lastFrameTime = std::chrono::steady_clock::now();
        std::cout << "Started capturing screenshots." << std::endl;
      }
      else
      {
        isCapturing = false;
        std::cout << "Stopped capturing screenshots." << std::endl;
      }
    break;

    case 't':
      // replacement for CTRL
      isTranslatingKeyPressed = true; // 't' key is pressed
    break;

    case 'o':
      // zoom in camera
      lookAtZ -= 10.0f;
      if (lookAtZ < 0.0f)
          lookAtZ = 0.0f; // set the minimum distance
      glutPostRedisplay();
    break;

    case 'p':
      // zoom out camera
      lookAtZ += 10.0f;
      if (lookAtZ > 2000.0f)
          lookAtZ = 2000.0f; // set the maximum distance
      glutPostRedisplay();
    break;
  }
}

// detect release key 't', this is a replacement for CTRL
void keyboardUpFunc(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 't':
      isTranslatingKeyPressed = false; // 't' key is released
    break;
  }
}

// actual rendering
void displayFunc()
{
  // This function performs the actual rendering.

  // First, clear the screen.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Set up the camera position, focus point, and the up vector.
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();

  // set up camera
  matrix.LookAt(128, 128, lookAtZ, 0, 0, 0, 0, 1, 0);

  // Translation
  matrix.Translate(terrainTranslate[0], terrainTranslate[1], terrainTranslate[2]);
  
  // Rotation
  matrix.Rotate(terrainRotate[0], 1.0f, 0.0f, 0.0f); // Rotate around x-axis
  matrix.Rotate(terrainRotate[1], 0.0f, 1.0f, 0.0f); // Rotate around y-axis
  matrix.Rotate(terrainRotate[2], 0.0f, 0.0f, 1.0f); // Rotate around z-axis

  // Scale
  matrix.Scale(terrainScale[0], terrainScale[1], terrainScale[2]);

  // Read the current modelview and projection matrices from our helper class.
  // The matrices are only read here; nothing is actually communicated to OpenGL yet.
  float modelViewMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(modelViewMatrix);

  float projectionMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(projectionMatrix);

  // Upload the modelview and projection matrices to the GPU. Note that these are "uniform" variables.
  // Important: these matrices must be uploaded to *all* pipeline programs used.
  // In hw1, there is only one pipeline program, but in hw2 there will be several of them.
  // In such a case, you must separately upload to *each* pipeline program.
  // Important: do not make a typo in the variable name below; otherwise, the program will malfunction.
  pipelineProgram.SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
  pipelineProgram.SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);

  // bind VAOVertice
  VAOVertice.Bind();

  // bind pipelineProgram
  pipelineProgram.Bind();

  if (displayOption == POINTS)
  {
    // glPointSize(2.0f); // Set point size.
    glDrawArrays(GL_POINTS, 0, numVertices);
  }
  else if (displayOption == LINES)
  { 
    EBOLineIndex.Bind();
    glDrawElements(GL_LINES, EBOLineIndex.GetNumIndices(), GL_UNSIGNED_INT, 0);
  }
  else if (displayOption == TRIANGLES || displayOption == SMOOTH)
  {
    EBOTriangleIndex.Bind();
    glDrawElements(GL_TRIANGLES, EBOTriangleIndex.GetNumIndices(), GL_UNSIGNED_INT, 0);
  }
  else if (displayOption == WIREFRAME)
  {
    // ------ Render Solid Triangles ------

    // enable polygon offset to prevent z-fighting
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f); // Push the depth values back

    // bind EBO
    EBOTriangleIndex.Bind();

    // set polygon mode to fill
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // draw the filled triangles
    glDrawElements(GL_TRIANGLES, EBOTriangleIndex.GetNumIndices(), GL_UNSIGNED_INT, 0);

    // disable polygon offset for fill
    glDisable(GL_POLYGON_OFFSET_FILL);

    // ------ Render Wireframe on Top ------

    // enable polygon offset for lines
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0f, -1.0f); // pull the depth values forward

    // set polygon mode to line
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // set line width
    glLineWidth(1.0f);

    // color wirefram
    pipelineProgram.SetUniformVariablei("useOverrideColor", 1);

    // bind EBO
    EBOLineIndex.Bind();

    // draw the lines
    glDrawElements(GL_LINES, EBOLineIndex.GetNumIndices(), GL_UNSIGNED_INT, 0);
    
    // disable polygon offset
    glDisable(GL_POLYGON_OFFSET_LINE);

    // reset polygon mode to fill
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // reset the override color usage
    pipelineProgram.SetUniformVariablei("useOverrideColor", 0);
  }

  // Unbind VAO
  glBindVertexArray(0);

  // Swap the double-buffers.
  glutSwapBuffers();
}

// read the image to find vertices and colors
void readImage()
{
  // Read the image to find the width, height, bytes per pixel and pixels
  width = heightmapImage->getWidth();
  cout << "Width:" << width << endl;
  height = heightmapImage->getHeight();
  cout << "Height:" << height << endl;
  bytesPerPixel = heightmapImage->getBytesPerPixel(); // bytesPerPixel == 1 for grey image and == 3 for color image
  cout << "Bytes per pixel:" << bytesPerPixel << endl;
  pixels = heightmapImage->getPixels();

  numVertices = width * height;

  // scale and shift for z
  zScale = 64.0f / 256.0f;
  zShift = 0.0f;

  // Clear the global vectors before populating
  pointPosition.clear();
  pointColor.clear();
  lineIndex.clear();
  triangleIndex.clear();

  // generate "pointLocation"
  for (unsigned int x = 0; x < height; x++)
  {
    for (unsigned int y = 0; y < width; y++)
    { 
      unsigned char z = heightmapImage->getPixel(x, y, 0);
      pointPosition.push_back(-height/2.0 + x); // x
      pointPosition.push_back((int)z * zScale - zShift); // z
      pointPosition.push_back(-width/2.0 + y); // y
    }
  }
  cout << "Built pointLocation with size: " << pointPosition.size() << ", should be " << 3 * width * height << endl; 

  // generate "pointColor"
  for (unsigned int x = 0; x < height; x++)
  {
    for (unsigned int y = 0; y < width; y++)
    { 
      unsigned char z = heightmapImage->getPixel(x, y, 0);
      pointColor.push_back(z / 255.0); // red
      pointColor.push_back(z / 255.0); // green
      pointColor.push_back(z / 255.0); // blue
      pointColor.push_back(1.0); // alpha
    }
  }
  cout << "Build pointColor with size: " << pointColor.size() << endl;

  // generate lineIndex
  for (unsigned int i = 0; i < height; i++)
  {
    for (unsigned int j = 0; j < width; j++)
    {
      // Horizontal lines.
      if (j < width - 1)
      {
        lineIndex.push_back(i * width + j);
        lineIndex.push_back(i * width + (j + 1));
      }
      // Vertical lines.
      if (i < height - 1)
      {
        lineIndex.push_back(i * width + j);
        lineIndex.push_back((i + 1) * width + j);
      }
    }
  }
  cout << "Build lineIndex with size " << lineIndex.size() << ", should be " << 4 * width * height - 2 * width - 2 * height << endl; 

  // generate triangleIndex
  for (unsigned int i = 0; i < height - 1; i++)
  {
    for (unsigned int j = 0; j < width - 1; j++)
    {
      // First triangle of the quad.
      triangleIndex.push_back(i * width + j);
      triangleIndex.push_back((i + 1) * width + j);
      triangleIndex.push_back(i * width + (j + 1));

      // Second triangle of the quad.
      triangleIndex.push_back((i + 1) * width + j);
      triangleIndex.push_back((i + 1) * width + (j + 1));
      triangleIndex.push_back(i * width + (j + 1));
    }
  }
  cout << "Build triangleIndex with size: " << triangleIndex.size() << endl; 

  // generate pointColorImage
  for (unsigned int x = 0; x < height; x++)
  {
    for (unsigned int y = 0; y < width; y++)
    {
      // Get the RGB values from the rescaled color image
      unsigned char r = colorImage->getPixel(x, y, 0);
      unsigned char g = colorImage->getPixel(x, y, 1);
      unsigned char b = colorImage->getPixel(x, y, 2);

      // Normalize the color values to [0, 1]
      pointColorImage.push_back(r / 255.0f); // red
      pointColorImage.push_back(g / 255.0f); // green
      pointColorImage.push_back(b / 255.0f); // blue
      pointColorImage.push_back(1.0f);       // alpha
    }
  }

}

// read the image to find vertices in mode 1
void readImageMode1() {
  for (unsigned int x = 0; x < height; x++)
  {
    for (unsigned int y = 0; y < width; y++)
    {
      unsigned char z_center = heightmapImage->getPixel(x, y, 0);
      float y_center = z_center / 255.0f;

      // Center position
      float center_x = -height / 2.0f + x;
      float center_z = -width / 2.0f + y;
      PcenterPosition.push_back(center_x); // x
      PcenterPosition.push_back(y_center); // y
      PcenterPosition.push_back(center_z); // z

      // Left neighbor
      float y_left = y_center;
      if (x > 0)
      {
        unsigned char z_left = heightmapImage->getPixel(x - 1, y, 0);
        y_left = z_left / 255.0f;
      }
      PleftPosition.push_back(-height / 2.0f + (x > 0 ? x - 1 : x)); // x coordinate
      PleftPosition.push_back(y_left); // y coordinate
      PleftPosition.push_back(center_z); // z coordinate

      // Right neighbor
      float y_right = y_center;
      if (x < height - 1)
      {
        unsigned char z_right = heightmapImage->getPixel(x + 1, y, 0);
        y_right = z_right / 255.0f;
      }
      PrightPosition.push_back(-height / 2.0f + (x < height - 1 ? x + 1 : x)); // x coordinate
      PrightPosition.push_back(y_right); // y coordinate
      PrightPosition.push_back(center_z); // z coordinate

      // Down neighbor
      float y_down = y_center;
      if (y > 0)
      {
        unsigned char z_down = heightmapImage->getPixel(x, y - 1, 0);
        y_down = z_down / 255.0f;
      }
      PdownPosition.push_back(center_x); // x coordinate
      PdownPosition.push_back(y_down); // y coordinate
      PdownPosition.push_back(-width / 2.0f + (y > 0 ? y - 1 : y)); // z coordinate

      // Up neighbor
      float y_up = y_center;
      if (y < width - 1)
      {
        unsigned char z_up = heightmapImage->getPixel(x, y + 1, 0);
        y_up = z_up / 255.0f;
      }
      PupPosition.push_back(center_x); // x coordinate
      PupPosition.push_back(y_up); // y coordinate
      PupPosition.push_back(-width / 2.0f + (y < width - 1 ? y + 1 : y)); // z coordinate
    }
  }

}

// initialize VAO, VBO and EBO
void initVAOVBO()
{ 
  // VAO --------------------
  // init VAOVertice
  VAOVertice.Gen();
  VAOVertice.Bind();

  // VBO --------------------
  // init VBOPosition
  VBOPosition.Gen(pointPosition.size(), 3, pointPosition.data(), GL_STATIC_DRAW);
  VBOPosition.Bind();

  // enable "position" attribute in the vertex shader
  GLuint loc = glGetAttribLocation(pipelineProgram.GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc); 
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*) 0); // set the layout of the "position" attribute data

  // init VBOColor
  VBOColor.Gen(pointColor.size(), 4, pointColor.data(), GL_STATIC_DRAW);
  VBOColor.Bind();

  // enable "color" attribute in the vertex shader
  loc = glGetAttribLocation(pipelineProgram.GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc); 
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void*) 0); // set the layout of the "color" attribute data

  // -- mode 1 --
  // PcenterVBO
  PcenterVBO.Gen(PcenterPosition.size() * sizeof(float), 3, PcenterPosition.data(), GL_STATIC_DRAW);
  PcenterVBO.Bind();
  loc = glGetAttribLocation(pipelineProgram.GetProgramHandle(), "positionUnscaled");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

  // PleftVBO
  PleftVBO.Gen(PleftPosition.size(), 3, PleftPosition.data(), GL_STATIC_DRAW);
  PleftVBO.Bind();
  loc = glGetAttribLocation(pipelineProgram.GetProgramHandle(), "PleftPosition");
  glEnableVertexAttribArray(loc); 
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

  // PrightVBO
  PrightVBO.Gen(PrightPosition.size(), 3, PrightPosition.data(), GL_STATIC_DRAW);
  PrightVBO.Bind();
  loc = glGetAttribLocation(pipelineProgram.GetProgramHandle(), "PrightPosition");
  glEnableVertexAttribArray(loc); 
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

  // PdownVBO
  PdownVBO.Gen(PdownPosition.size(), 3, PdownPosition.data(), GL_STATIC_DRAW);
  PdownVBO.Bind();
  loc = glGetAttribLocation(pipelineProgram.GetProgramHandle(), "PdownPosition");
  glEnableVertexAttribArray(loc); 
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

  // PupVBO
  PupVBO.Gen(PupPosition.size(), 3, PupPosition.data(), GL_STATIC_DRAW);
  PupVBO.Bind();
  loc = glGetAttribLocation(pipelineProgram.GetProgramHandle(), "PupPosition");
  glEnableVertexAttribArray(loc); 
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

  // VBO for the color image
  VBOColorImage.Gen(pointColorImage.size() * sizeof(float), 4, pointColorImage.data(), GL_STATIC_DRAW);
  VBOColorImage.Bind();
  GLuint locColorImage = glGetAttribLocation(pipelineProgram.GetProgramHandle(), "colorImage");
  glEnableVertexAttribArray(locColorImage);
  glVertexAttribPointer(locColorImage, 4, GL_FLOAT, GL_FALSE, 0, (const void*)0);

  // EBO --------------------
  // init EBOLineIndex
  EBOLineIndex.Gen(lineIndex.size() * sizeof(unsigned int), (const int*)lineIndex.data(), GL_STATIC_DRAW);
  EBOLineIndex.Bind();

  // init EBOTriangleIndex
  EBOTriangleIndex.Gen(triangleIndex.size() * sizeof(unsigned int), (const int*)triangleIndex.data(), GL_STATIC_DRAW);

  // Unbind VBO
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  // Unbind VAO
  glBindVertexArray(0);

  cout << "Successfully built the VAO, VBO and EBO" << endl;
}

void initScene(int argc, char *argv[])
{ 
  // Load the image from a jpeg disk file into main memory.
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }
  cout << "Successfully read image" << argv[1] << endl;

  // Load the color image
  if (colorImage->loadJPEG("heightmap/color.jpg") != ImageIO::OK)
  {
    std::cout << "Error reading color image heightmap/color.jpg." << std::endl;
    exit(EXIT_FAILURE);
  }
  else
  {
    std::cout << "Successfully read color image heightmap/color.jpg" << std::endl;
  }

  // Check size of color image
  if (colorImage->getWidth() != heightmapImage->getWidth() || colorImage->getHeight() != heightmapImage->getHeight())
  {
    std::cout << "Rescaling color image to match heightmap dimensions..." << std::endl;

    // Rescale the color image
    std::vector<unsigned char> rescaledData = rescaleColorImage(heightmapImage->getWidth(), heightmapImage->getHeight());

    // Update the color image with the rescaled data
    colorImage = std::make_unique<ImageIO>(heightmapImage->getWidth(), heightmapImage->getHeight(), colorImage->getBytesPerPixel(), rescaledData.data());

    std::cout << "Color image rescaled successfully." << std::endl;
  }

  // Set the background color.
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black color.

  // Enable z-buffering (i.e., hidden surface removal using the z-buffer algorithm).
  glEnable(GL_DEPTH_TEST);

  // read the image to find vertices and colors
  readImage();
  readImageMode1();
  
  // Create a pipeline program. This operation must be performed BEFORE we initialize any VAOs.
  // A pipeline program contains our shaders. Different pipeline programs may contain different shaders.
  // In this homework, we only have one set of shaders, and therefore, there is only one pipeline program.
  // In hw2, we will need to shade different objects with different shaders, and therefore, we will have
  // several pipeline programs (e.g., one for the rails, one for the ground/sky, etc.).
  // Load and set up the pipeline program, including its shaders.
  if (pipelineProgram.BuildShadersFromFiles(shaderBasePath, "vertexShader.glsl", "fragmentShader.glsl") != 0)
  {
    cout << "Failed to build the pipeline program." << endl;
    throw 1;
  } 
  cout << "Successfully built the pipeline program." << endl;
    
  // Bind the pipeline program that we just created. 
  // The purpose of binding a pipeline program is to activate the shaders that it contains, i.e.,
  // any object rendered from that point on, will use those shaders.
  // When the application starts, no pipeline program is bound, which means that rendering is not set up.
  // So, at some point (such as below), we need to bind a pipeline program.
  // From that point on, exactly one pipeline program is bound at any moment of time.
  pipelineProgram.Bind();
  
  // Set initial uniform variables
  pipelineProgram.SetUniformVariablei("mode", 0); // Start with mode 0
  pipelineProgram.SetUniformVariablef("scale", scale);
  pipelineProgram.SetUniformVariablef("exponent", exponent);
  pipelineProgram.SetUniformVariablei("colorMode", colorMode);

  // create VAO and VBO
  initVAOVBO();

  // Check for any OpenGL errors.
  std::cout << "GL error status is: " << glGetError() << std::endl;
}

int main(int argc, char *argv[])
{ 
  if (argc != 2)
  {
    cout << "The arguments are incorrect." << endl;
    cout << "usage: ./hw1 <heightmap file>" << endl;
    exit(EXIT_FAILURE);
  }

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);  
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(windowWidth - 1, windowHeight - 1);
  #endif

  // Tells GLUT to use a particular display function to redraw.
  glutDisplayFunc(displayFunc);
  // Perform animation inside idleFunc.
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);
  // callback for unpressing the key 't'
  glutKeyboardUpFunc(keyboardUpFunc);

  // init glew
  #ifdef __APPLE__
    // nothing is needed on Apple
  #else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
  #endif

  // Perform the initialization.
  initScene(argc, argv);

  // Sink forever into the GLUT loop.
  glutMainLoop();
}