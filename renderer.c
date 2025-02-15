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
#include <unistd.h>

#define M_PI 3.14159265358979323846
#define DEG_TO_RAD(deg) ((deg) * (M_PI / 180.0))
#define TARGETFPS 60
#define FRAMETIME (1000 / TARGETFPS)

/*
COMPILE COMMAND: 
gcc -o renderer renderer.c -lGL -lGLU -lglut -lGLEW -lm -lrt
*/

// GLOBAL VARIABLES
int WIDTH = 800;
int HEIGHT = 600;
float FOV = 45.0f;

int OBJECTAMOUNT = 2;
int LIGHTAMOUNT = 1;

// Texture ID
GLuint* TextureIDs = NULL;
int TextureCount = 0;


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
    bool invertnormal;
};


struct object {
	int trianglenum;
	struct Triangle* triangles;
};


struct vector3 {
    float x, y, z;
};


struct Light {
    struct vector3 position;
    struct color color;
    float intensity;
};


struct color WHITE = {
    .r=1.0f, .g=1.0f, .b=1.0f, .a=1.0f
};


// OBJECT POINTER
struct object *objectptr;

// OBJECT COLOR
struct color *colorptr;

// LIGHT LIST POINTER
struct Light *lightptr;


// VECTOR3 ZERO
struct vector3 VZERO = {0.0f, 0.0f, 0.0f};


// The camera position
struct Transform camerapos = {
    .px=0.0f, .py=0.0f, .pz=0.0f,
    .rx=0.0f, .ry=0.0f, .rz=0.0f
};


void fpsLimiter(int value) {
    glutPostRedisplay();  // Schedule a redraw
    glutTimerFunc(FRAMETIME, fpsLimiter, 0);  // Call again after FRAME_TIME ms
}


void rotatePoint3D(float* x, float* y, float* z, float angleX, float angleY, float angleZ) {
    // Convert degrees to radians
    float radX = angleX * (M_PI / 180.0f);
    float radY = angleY * (M_PI / 180.0f);
    float radZ = angleZ * (M_PI / 180.0f);

    // Rotation matrix for X-axis
    float cosX = cos(radX);
    float sinX = sin(radX);

    // Rotation matrix for Y-axis
    float cosY = cos(radY);
    float sinY = sin(radY);

    // Rotation matrix for Z-axis
    float cosZ = cos(radZ);
    float sinZ = sin(radZ);

    // Step 1: Rotate around X-axis
    float tempY = *y * cosX - *z * sinX;
    float tempZ = *y * sinX + *z * cosX;
    *y = tempY;
    *z = tempZ;

    // Step 2: Rotate around Y-axis
    float tempX = *z * sinY + *x * cosY;
    *z = *z * cosY - *x * sinY;
    *x = tempX;

    // Step 3: Rotate around Z-axis
    tempX = *x * cosZ - *y * sinZ;
    tempY = *x * sinZ + *y * cosZ;
    *x = tempX;
    *y = tempY;
}


struct vector3 WorldspaceToCameraSpace(struct vector3 position) {
    float x = position.x - camerapos.px;
    float y = position.y - camerapos.py;
    float z = position.z - camerapos.pz;

    rotatePoint3D(&x, &y, &z, camerapos.rx, camerapos.ry, camerapos.rz);
    return (struct vector3){x, y, z};
}


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


struct vector3 crossProduct(struct vector3 a, struct vector3 b) {
    struct vector3 result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

// Function to compute the dot product of two vectors
float dotProduct(struct vector3 a, struct vector3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Function to normalize a vector
struct vector3 normalize(struct vector3 v) {
    float length = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    v.x /= length;
    v.y /= length;
    v.z /= length;
    return v;
}

// Function to compute the normal vector of a triangle
struct vector3 ComputeNormal(struct vector3 A, struct vector3 B, struct vector3 C, struct vector3 viewDir) {
    // Compute the two edge vectors
    struct vector3 AB = {B.x - A.x, B.y - A.y, B.z - A.z};
    struct vector3 AC = {C.x - A.x, C.y - A.y, C.z - A.z};
    

    struct vector3 Center = {
        (A.x + B.x + C.x) / 3,
        (A.y + B.y + C.y) / 3,
        (A.z + B.z + C.z) / 3
    };

    // Compute the cross product of AB and AC
    struct vector3 normal = crossProduct(AB, AC);
    
    // Normalize the normal vector
    normal = normalize(normal);

    // Determine if the normal is facing the correct direction
    // If the dot product between the normal and the view direction is negative,
    // the normal is facing away from the camera, so flip it
    if (dotProduct(normal, viewDir) < 0.0f) {
        normal.x = -normal.x;
        normal.y = -normal.y;
        normal.z = -normal.z;
    }
    
    return normal;
}


struct color LambertianDiffuse(struct vector3 normal, struct vector3 midpoint, struct Light* lights, int lightCount) {
    struct color totalDiffuse = {0.0f, 0.0f, 0.0f, 1.0f}; // Initialize to black

    // Loop through all the lights
    for (int i = 0; i < lightCount; i++) {
        // Vector from the midpoint to the light
        struct vector3 lightDir = {lights[i].position.x - midpoint.x, 
                                   lights[i].position.y - midpoint.y, 
                                   lights[i].position.z - midpoint.z};
        
        // Calculate the distance between the midpoint and the light source
        float distance = sqrt(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);

        // Normalize light direction
        lightDir = normalize(lightDir); 

        // Calculate the Lambertian diffuse intensity (clamped to non-negative)
        float diffuseIntensity = fmax(0.0f, dotProduct(normal, lightDir)) * lights->intensity;

        // Apply distance attenuation (inverse square law)
        if (diffuseIntensity > 0.0f && distance > 0.0f) {
            // Attenuate light intensity with distance
            float attenuation = 1.0f / (distance * distance);

            // Apply the attenuation and add the contribution of the current light source
            totalDiffuse.r += diffuseIntensity * lights[i].color.r * attenuation;
            totalDiffuse.g += diffuseIntensity * lights[i].color.g * attenuation;
            totalDiffuse.b += diffuseIntensity * lights[i].color.b * attenuation;
        }
    }

    // Ensure the color components are clamped between 0 and 1
    if (totalDiffuse.r > 1.0f) totalDiffuse.r = 1.0f;
    if (totalDiffuse.g > 1.0f) totalDiffuse.g = 1.0f;
    if (totalDiffuse.b > 1.0f) totalDiffuse.b = 1.0f;

    // Ensure the color components are above a certain threshold
    if (totalDiffuse.r < 0.05f) totalDiffuse.r = 0.05f;
    if (totalDiffuse.g < 0.05f) totalDiffuse.g = 0.05f;
    if (totalDiffuse.b < 0.05f) totalDiffuse.b = 0.05f;

    return totalDiffuse;
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


struct vector3 angleToZero(struct vector3 position) {
    float x = cos(position.x);
    float z = cos(position.z);
    float y = sin(position.y);

    return (struct vector3){x, y, z};
}


void DrawMesh(struct object Object, struct Transform transform, GLuint TextureID, struct Light *lights, int lightcount, bool flatshaded) {
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
        if (!flatshaded) {
            // Extract the vertex positions of the current triangle to compute the shading
            struct vector3 A = {
                Triangles[i].v1.x, 
                Triangles[i].v1.y, 
                Triangles[i].v1.z
            };
            struct vector3 B = {
                Triangles[i].v2.x, 
                Triangles[i].v2.y, 
                Triangles[i].v2.z
            };
            struct vector3 C = {
                Triangles[i].v3.x, 
                Triangles[i].v3.y, 
                Triangles[i].v3.z
            };


            // Account for rotation

            rotatePoint3D(&A.x, &A.y, &A.z, transform.rx, transform.ry, transform.rz);
            rotatePoint3D(&B.x, &B.y, &B.z, transform.rx, transform.ry, transform.rz);
            rotatePoint3D(&C.x, &C.y, &C.z, transform.rx, transform.ry, transform.rz);

            // Calculate the center point
            struct vector3 center = {
                (A.x + B.x + C.x) / 3,
                (A.y + B.y + C.y) / 3,
                (A.z + B.z + C.z) / 3
            };

            // Compute the shading
            struct color Shade = LambertianDiffuse(ComputeNormal(A, B, C, angleToZero(center)), center, lights, lightcount);

            DrawTriangle(Triangles[i], TextureID, Shade);//, colorptr[i]);
        }
        else {
            DrawTriangle(Triangles[i], TextureID, WHITE);
        }
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
        { .x = 1.0f, .y = -1.0f, .z = 0.0f, .r = 0.0f, .g = 0.0f, .b = 1.0f, .u = 1.0f, .v = 0.0f}  
    };

    struct Transform Transformation1 = {
        .px = 0.0f, .py = 0.0f, .pz = 0.0f,
        .sx = 1.0f, .sy = 1.0f, .sz = 1.0f,
        .rx = angle, .ry = angle, .rz = 0.0f // Rotate around X and Y axes
    };


    //struct Transform Transformation1 = {
    //    .px = 0.0f, .py = 0.0f, .pz = 0.0f,
    //    .sx = 1.0f, .sy = 1.0f, .sz = 1.0f,
    //    .rx = 0.0f, .ry = 0.0f, .rz = 0.0f // Rotate around X and Y axes
    //};

    struct Transform Transformation2 = {
        .px = 0.0f, .py = 0.0f, .pz = 0.0f,
        .sx = 1.0f, .sy = 1.0f, .sz = 1.0f,
        .rx = angle + 45.0f, .ry = angle + 45.0f, .rz = 0.0f // Rotate around X and Y axes
    };

    // Draw the cube with the updated transformation
    DrawMesh(objectptr[0], Transformation1, TextureIDs[0], lightptr, LIGHTAMOUNT, false);
    DrawMesh(objectptr[1], Transformation2, TextureIDs[0], lightptr, LIGHTAMOUNT, false);

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

    objectptr[0].triangles = TempCube.triangles;
    objectptr[0].trianglenum = TempCube.trianglenum;

    objectptr[1].triangles = TempCube.triangles;
    objectptr[1].trianglenum = TempCube.trianglenum;

    struct Light light1 = {
        {0.0f, 0.0f, 3.0f},
        {1.0f, 1.0f, 1.0f, 0.0f},
        5.0f
    };

    lightptr[0].color = light1.color;
    lightptr[0].position = light1.position;
    lightptr[0].intensity = light1.intensity;

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
    free(objectptr);
    free(colorptr);
}


// Main function
int main(int argc, char** argv) {
    // Assign memory to all needed pointers
    struct object *cube;

    cube = (struct object*)malloc(sizeof(*cube));
    objectptr = (struct object*)malloc(sizeof(struct object) * OBJECTAMOUNT);  // N is the number of objects you plan to store.


    struct color *Color;//Color = {Random(), Random(), Random(), 1.0f};

    Color = (struct color*)malloc(sizeof(*Color));
    colorptr = (struct color*)malloc(12 * sizeof(struct color));


    struct Light *lights;
    lights = (struct Light*)malloc(sizeof(*lights));
    lightptr = (struct Light*)malloc(sizeof(struct Light) * LIGHTAMOUNT);

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

    // Limit the framerate
    glutTimerFunc(FRAMETIME, fpsLimiter, 0);

    // Register the idle function
    glutIdleFunc(idle);

    // Register the cleanup function
    atexit(Cleanup);

    // Run the main loop
    glutMainLoop();

    return 0;
}
