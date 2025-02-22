#define STB_IMAGE_IMPLEMENTATION

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "stb_image.h"
#include <math.h> 
#include <time.h>

#define M_PI 3.14159265358979323846
#define DEG_TO_RAD(deg) ((deg) * (M_PI / 180.0))

/*
COMPILE COMMAND: 
gcc -o test test.c -lGL -lGLU -lglut -lGLEW -lm
*/

// GLOBAL VARIABLES
int WIDTH = 800;
int HEIGHT = 600;
float FOV = 45.0f;

// Texture ID
GLuint* TextureIDs = NULL;
int TextureCount = 0;


// CUBE POINTER
struct object *cubeptr;

// CUBE COLOR
struct color *colorptr;


struct vertex {
    float x, y, z;
	float r, g, b, a;
	float u, v;
};


struct color {
    float r, g, b, a;
};


struct Transform {
    float px, py, pz;
    float sx, sy, sz;
    float rx, ry, rz;
};


struct Triangle {
    struct vertex v1, v2, v3;
};


struct object {
	int trianglenum;
	struct Triangle* triangles;
};


struct object CreateObject(int trianglenum, struct Triangle* triangles) {
    struct object newObject;
    newObject.trianglenum = trianglenum;

    // Allocate memory for the triangles array
    newObject.triangles = (struct Triangle*)malloc(trianglenum * sizeof(struct Triangle));
    if (newObject.triangles == NULL) {
        printf("Memory allocation failed for triangles\n");
        exit(1);
    }

    // Copy data from the input array into the new object
    for (int i = 0; i < trianglenum; i++) {
        newObject.triangles[i] = triangles[i]; // Copy triangle data
    }

    return newObject;
}

GLuint LoadTexture(const char* filename) {
    // Load the image data using stb_image
    int width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* img_data = stbi_load(filename, &width, &height, &channels, 0);
    if (img_data == NULL) {
        printf("Error in loading texture image: %s\n", filename);
        return 0; // Return 0 if there's an error
    }

    // Generate the OpenGL texture ID
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Set texture parameters (e.g., filtering, wrapping)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Load the texture data into OpenGL
    GLenum format = GL_RGB;
    if (channels == 4) {
        format = GL_RGBA; // If the image has an alpha channel
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, img_data);
    glGenerateMipmap(GL_TEXTURE_2D); // Optionally generate mipmaps

    // Free the image data as it's now loaded into OpenGL
    stbi_image_free(img_data);

    // Return the texture ID
    return textureID;
}


void LoadMultipleTextures(int numTextures, const char** filenames) {
    if (numTextures <= 0) return;

    GLuint* newTextureIDs = realloc(TextureIDs, numTextures * sizeof(GLuint));
    if (newTextureIDs == NULL) {
        printf("Failed to allocate memory for textures.\n");
        exit(1);
    }
    TextureIDs = newTextureIDs;

    for (int i = 0; i < numTextures; ++i) {
        TextureIDs[i] = LoadTexture(filenames[i]);
        if (TextureIDs[i] == 0) {
            printf("Error: Failed to load texture %s\n", filenames[i]);
        } else {
            printf("Texture %s loaded successfully. ID: %u\n", filenames[i], TextureIDs[i]);
        }
    }

    TextureCount += numTextures;
}


void DrawColoredTriangle(struct vertex vertices[3]){
    struct vertex v1 = vertices[0];
    struct vertex v2 = vertices[1];
	struct vertex v3 = vertices[2];

    glBegin(GL_TRIANGLES);	
		glColor4f(v1.r, v1.g, v1.b, v1.a);
		glVertex3f(v1.x, v1.y, v1.z);

		glColor4f(v2.r, v2.g, v2.b, v2.a);
		glVertex3f(v2.x, v2.y, v2.z);

		glColor4f(v3.r, v3.g, v3.b, v3.a);
		glVertex3f(v3.x, v3.y, v3.z);
    glEnd();

	glutSwapBuffers();

    return;
}


void rotatePoint(float* x, float* y, float angle) {
    float rad = angle * (M_PI / 180.0f); // Convert degrees to radians
    float cosA = cos(rad);
    float sinA = sin(rad);

    // Apply rotation matrix
    float newX = cosA * (*x) - sinA * (*y);
    float newY = sinA * (*x) + cosA * (*y);

    // Update vertex coordinates
    *x = newX;
    *y = newY;
}


float Random(){
    float min = 0.0f;
    float max = 1.0f;

    // Generate a random floating-point number between min and max
    float randomFloat = min + (float)rand() / RAND_MAX * (max - min);
    return randomFloat;
}


void DrawTriangle(struct Triangle triangle, GLuint TextureID, struct color Color) {//, struct Transform transform){
    struct vertex v1 = triangle.v1;
    struct vertex v2 = triangle.v2;
	struct vertex v3 = triangle.v3;

    float rotation =22.5f;

    //glPushMatrix();

    //glTranslatef(transform.px, transform.py, transform.pz);
    //glScalef(transform.sx, transform.sy, transform.sz);

	// Bind the texture
	glBindTexture(GL_TEXTURE_2D, TextureID);

    glColor4f(Color.r, Color.g, Color.b, Color.a);

    glBegin(GL_TRIANGLES);
        glTexCoord2f(v1.u, v1.v);
		glVertex3f(v1.x, v1.y, v1.z);

        glTexCoord2f(v2.u, v2.v);
		glVertex3f(v2.x, v2.y, v2.z);

        glTexCoord2f(v3.u, v3.v);
		glVertex3f(v3.x, v3.y, v3.z);
    glEnd();

    //glPopMatrix();
}


void DrawMesh(struct object Object, struct Transform transform, GLuint TextureID) {
    int Trianglenum = Object.trianglenum;
    struct Triangle* Triangles = Object.triangles;

    // Push a matrix and apply the transformation
    glPushMatrix();
    glTranslatef(transform.px, transform.py, transform.pz);
    glScalef(transform.sx, transform.sy, transform.sz);
    glRotatef(transform.rx, 1, 0, 0);
    glRotatef(transform.ry, 0, 1, 0);
    glRotatef(transform.rz, 0, 0, 1);

    for (int i = 0; i < Trianglenum; i++) {
        DrawTriangle(Triangles[i], TextureID, colorptr[i]);
    }

    // Pop the matrix to avoid the current transform affecting other meshes
    glPopMatrix();
}


void display(void) {
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (TextureCount == 0 || TextureIDs == NULL || TextureIDs[0] == 0) {
        printf("Error: No valid texture loaded.\n");
        return;
    }

    // Update the cube's rotation over time
    static float angle = 0.0f;
    angle += 1.0f; // Increment the angle for rotation


    // Draw a Triangle with the right colors and positions using the vertex struct and DrawTriangle function
    struct vertex vertices[3] = {
        { .x = 1.0f, .y = 1.0f, .z = 0.0f, .r = 1.0f, .g = 0.0f, .b = 0.0f, .u = 1.0f, .v = 1.0f},
        { .x = -1.0f, .y = -1.0f, .z = 0.0f, .r = 0.0f, .g = 1.0f, .b = 0.0f, .u = 0.0f, .v = 0.0f},
        { .x = 1.0f, .y = -1.0f, .z = 0.0f, .r = 0.0f, .g = 0.0f, .b = 1.0f, .u = 1.0f, .v = 0.0f},
    };


    struct Triangle triangle = {
        { .x = 1.0f, .y = 1.0f, .z = 0.0f, .r = 1.0f, .g = 0.0f, .b = 0.0f, .u = 1.0f, .v = 1.0f},
        { .x = -1.0f, .y = -1.0f, .z = 0.0f, .r = 0.0f, .g = 1.0f, .b = 0.0f, .u = 0.0f, .v = 0.0f},
        { .x = 1.0f, .y = -1.0f, .z = 0.0f, .r = 0.0f, .g = 0.0f, .b = 1.0f, .u = 1.0f, .v = 0.0f},
    };

    struct Transform Transformation1 = {
        .px = 0.0f, .py = 0.0f, .pz = 0.0f,
        .sx = 1.0f, .sy = 1.0f, .sz = 1.0f,
        .rx = angle, .ry = angle, .rz = 0.0f // Rotate around X and Y axes
    };

    // Draw the cube with the updated transformation
    DrawMesh(*cubeptr, Transformation1, TextureIDs[0]);

    // Swap buffers to display the rendered frame
    glutSwapBuffers();
}


void idle() {
    glutPostRedisplay(); // Request a redraw
}


void init() {
    // Initialize GLEW after creating the window and OpenGL context
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        printf("GLEW initialization failed!\n");
        exit(1); // Exit if GLEW fails to initialize
    }

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);

    // Set up the projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Set up a perspective view
    float AspectRatio = (float)WIDTH / (float)HEIGHT;
    gluPerspective(FOV, AspectRatio, 0.1f, 100.0f);

    const char* Textures[1] = {"cobblesmall.png"};

    // Load the cube model
    struct Triangle triangles[12] = {
        // Face 1: Front (+Z)
        {{-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f}, { 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f}, { 0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f}, { 0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, {-0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f}},

        // Face 2: Left (-X)
        {{-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f}, {-0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f}, {-0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f}, {-0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, {-0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f}},

        // Face 3: Right (+X)
        {{ 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f}, { 0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f}, { 0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
        {{ 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f}, { 0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, { 0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f}},

        // Face 4: Back (-Z)
        {{-0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f}, {-0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f}, { 0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f}, { 0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, { 0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f}},

        // Face 5: Top (+Y)
        {{-0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f}, { 0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, { 0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f}, { 0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f}, {-0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f}},

        // Face 6: Bottom (-Y)
        {{-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f}, {-0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f}, { 0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f}, { 0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f}, { 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f}},
    };

    struct object TempCube = CreateObject(12, triangles);

    cubeptr->triangles = TempCube.triangles;
    cubeptr->trianglenum = TempCube.trianglenum;

    for (int i = 0; i < 12; i++) {
        colorptr[i].r = Random();
        colorptr[i].g = Random();
        colorptr[i].b = Random();
        colorptr[i].a = 1.0f; // Alpha is always 1.0
    }

    TextureCount = 1;
    LoadMultipleTextures(1, Textures);

    // Move the camera back
    glTranslatef(0.0f, 0.0f, -5.0f);
}


void Cleanup() {
	// Clean up all textures
	if (TextureIDs != NULL) {
		glDeleteTextures(TextureCount, TextureIDs);
		free(TextureIDs);
	}
    free(cubeptr);
    free(colorptr);
}


// Main function
int main(int argc, char** argv) {
    struct object *cube;

    cube = (struct object*)malloc(sizeof(*cube));
    cubeptr = (struct object*)cube;


    struct color *Color;//Color = {Random(), Random(), Random(), 1.0f};

    Color = (struct color*)malloc(sizeof(*Color));
    colorptr = (struct color*)malloc(12 * sizeof(struct color));

    srand(time(NULL));

    // Initialize GLUT
    glutInit(&argc, argv);

    // Set up the renderer with Double buffering, RGB colors, and Depth testing
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    // Set up the window size
    glutInitWindowSize(800, 600);

    // Create a window with a name
    glutCreateWindow("Test Window");

    // Call the INIT function
    init();

    // Register the draw function
    glutDisplayFunc(display);

    // Register the idle function
    glutIdleFunc(idle);

    // Register the cleanup function
    atexit(Cleanup);

    // Run the main loop
    glutMainLoop();

    return 0;
}
