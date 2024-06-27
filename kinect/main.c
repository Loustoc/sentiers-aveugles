// MODIFIED AND ADAPTED FROM :

/*
 * This file is part of the OpenKinect Project. http://www.openkinect.org
 *
 * Copyright (c) 2010 individual OpenKinect contributors. See the CONTRIB file
 * for details.
 *
 * Andrew Miller <amiller@dappervision.com>
 *
 * This code is licensed to you under the terms of the Apache License, version
 * 2.0, or, at your option, the terms of the GNU General Public License,
 * version 2.0. See the APACHE20 and GPL2 files for the text of the licenses,
 * or the following URLs:
 * http://www.apache.org/licenses/LICENSE-2.0
 * http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * If you redistribute this file in source form, modified or unmodified, you
 * may:
 *   1) Leave this header intact and distribute it under the same terms,
 *      accompanying it with the APACHE20 and GPL20 files, or
 *   2) Delete the Apache 2.0 clause and accompany it with the GPL2 file, or
 *   3) Delete the GPL v2 clause and accompany it with the APACHE20 file
 * In all cases you must keep the copyright notice intact and include a copy
 * of the CONTRIB file.
 *
 * Binary distributions must follow the binary distribution requirements of
 * either License.
 */

#include "libfreenect.h"
#include "libfreenect_sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include "tinyosc.h"
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>  // For threading support

// OSC settings

// WIP : Dynamic subdivs change via OSC msgs
#define OSC_LISTEN_PORT 9420

#define OSC_PORT 9419
#define BUFFER_SIZE 2048

#define SUBDIV 3

#define X_THRESHOLD 630

bool hasStarted = false;

// to convert raw depth data to meters : 1.0 / (RAWDATA * -0.0030711016 + 3.3309495161);
// only valid from 0 to ~ 1000 (3.8m)

float Y_THRESHOLD = 1.0 / (1000 * -0.0030711016 + 3.3309495161);

// Global variables
int oscRecvSocket, oscSendSocket;
struct sockaddr_in oscAddr;
pthread_t oscThread;

static volatile bool keepRunning = true;

static void sigintHandler(int x) {
  keepRunning = false;
}

#if defined(__APPLE__)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

GLuint gl_rgb_tex;
int mx = -1, my = -1;        // Previous mouse coordinates
int rotangles[2] = {0};      // Panning angles
float zoom = 1;              // zoom factor
int color = 1;               // Use the RGB texture or just draw it as color
int window;
char buffer[2048]; // declare a buffer to write OSC messages

// Reference array
int calibration_array[307200][3];
bool calibration_done = false;

// Function to handle incoming OSC messages
void *oscListener(void *arg) {
    char buffer[BUFFER_SIZE];
    int recvlen;
    socklen_t addrlen = sizeof(oscAddr);

    while (keepRunning) {
        recvlen = recvfrom(oscRecvSocket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&oscAddr, &addrlen);
        if (recvlen > 0) {
            tosc_message osc;
            if (tosc_parseMessage(&osc, buffer, recvlen) >= 0) {
                printf("Received OSC message: %s\n", osc.format);
                tosc_printMessage(&osc);
            }
        }
    }
    return NULL;
}

void initOSC() {
    // UDP socket to receiving OSC
    if ((oscRecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Error creating OSC receiving socket");
        exit(1);
    }

    // address struct for receiving init
    memset((char *)&oscAddr, 0, sizeof(oscAddr));
    oscAddr.sin_family = AF_INET;
    oscAddr.sin_port = htons(OSC_LISTEN_PORT);
    oscAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind receiving socket to OSC port
    if (bind(oscRecvSocket, (struct sockaddr *)&oscAddr, sizeof(oscAddr)) < 0) {
        perror("Error binding OSC receiving socket");
        close(oscRecvSocket);
        exit(1);
    }

    // OSC listener thread
    if (pthread_create(&oscThread, NULL, oscListener, NULL) != 0) {
        perror("Error creating OSC listener thread");
        close(oscRecvSocket);
        exit(1);
    }

    // Detach the thread
    pthread_detach(oscThread);

    // UDP socket to send OSC
    if ((oscSendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Error creating OSC sending socket");
        exit(1);
    }

    // target address struct
    memset((char *)&oscAddr, 0, sizeof(oscAddr));
    oscAddr.sin_family = AF_INET;
    oscAddr.sin_port = htons(OSC_PORT);
    oscAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Assuming you are sending to localhost
}

void gluPerspective(GLdouble fovY, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
    const double pi = acos(-1);
    const GLdouble fH = tan(fovY / 360 * pi) * zNear;
    const GLdouble fW = fH * aspect;
    glFrustum(-fW, fW, -fH, fH, zNear, zFar);
}

void LoadVertexMatrix()
{
    float fx = 594.21f;
    float fy = 591.04f;
    float a = -0.0030711f;
    float b = 3.3309495f;
    float cx = 339.5f;
    float cy = 242.7f;
    GLfloat mat[16] = {
        1 / fx, 0, 0, 0,
        0, -1 / fy, 0, 0,
        0, 0, 0, a,
        -cx / fx, cy / fy, -1, b
    };
    glMultMatrixf(mat);
}

void LoadRGBMatrix()
{
    float mat[16] = {
        5.34866271e+02, 3.89654806e+00, 0.00000000e+00, 1.74704200e-02,
        -4.70724694e+00, -5.28843603e+02, 0.00000000e+00, -1.22753400e-02,
        -3.19670762e+02, -2.60999685e+02, 0.00000000e+00, -9.99772000e-01,
        -6.98445586e+00, 3.31139785e+00, 0.00000000e+00, 1.09167360e-02
    };
    glMultMatrixf(mat);
}

void mouseMoved(int x, int y)
{
    if (mx >= 0 && my >= 0) {
        rotangles[0] += y - my;
        rotangles[1] += x - mx;
    }
    mx = x;
    my = y;
}

void mousePress(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        mx = x;
        my = y;
    }
    if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        mx = -1;
        my = -1;
    }
}

void no_kinect_quit(void)
{
    printf("Error: Kinect not connected?\n");
    exit(1);
}

void calibration()
{
    short *depth = 0;
    char *rgb = 0;
    uint32_t ts;
    int farthest = 0;
    // Get depth data from Kinect
    if (freenect_sync_get_depth((void**)&depth, &ts, 0, FREENECT_DEPTH_11BIT) < 0)
        no_kinect_quit();
    
    // Get RGB data from Kinect
    if (freenect_sync_get_video((void**)&rgb, &ts, 0, FREENECT_VIDEO_RGB) < 0)
        no_kinect_quit();

    for (int i = 0; i < 480; ++i) {
        for (int j = 0; j < 640; ++j) {
            int index = i * 640 + j;
            if (depth[index] >= 1001) {
                 calibration_array[index][2] = 1000;
            } else {
                calibration_array[index][2] = depth[index];
            }
            calibration_array[i * 640 + j][0] = j; // x
            calibration_array[i * 640 + j][1] = i; // y
        }
    }
    calibration_done = true;
    printf("------ END CALIBRATION ------\n");
}

void DrawGLScene()
{
    printf("------ INIT VALUES ------\n");
    short *depth = 0;
    char *rgb = 0;
    uint32_t ts;
    
    // Get depth data from Kinect
    if (freenect_sync_get_depth((void**)&depth, &ts, 0, FREENECT_DEPTH_11BIT) < 0)
        no_kinect_quit();
    
    // Get RGB data from Kinect
    if (freenect_sync_get_video((void**)&rgb, &ts, 0, FREENECT_VIDEO_RGB) < 0)
        no_kinect_quit();

    // Clear buffers and load identity matrix
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Apply transformations
    glPushMatrix();
    glScalef(zoom, zoom, 1);
    glTranslatef(0, 0, -3.5);
    glRotatef(rotangles[0], 1, 0, 0);
    glRotatef(rotangles[1], 0, 1, 0);
    glTranslatef(0, 0, 1.5);

    // Load vertex matrix
    LoadVertexMatrix();

    // Set the projection from the XYZ to the texture image
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glScalef(1 / 640.0f, 1 / 480.0f, 1);
    LoadRGBMatrix();
    LoadVertexMatrix();
    glMatrixMode(GL_MODELVIEW);

    int totalBodyPoints = 0;
    int map[SUBDIV][SUBDIV] = {{0}};
    bool unChanged[480*640] = {{false}};
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 480; ++i) {
        for (int j = 0; j < 640; ++j) {
            if (depth[i * 640 + j] < 1000) {
            float depthInMeter = 1.0 / ((float) depth[i * 640 + j] * -0.0030711016 + 3.3309495161);
            if (abs(depthInMeter - (1.0 / ((float) calibration_array[i * 640 + j][2] * -0.0030711016 + 3.3309495161))) > 0.20 && depthInMeter <= (1.0 / (1000 * -0.0030711016 + 3.3309495161)) && calibration_done) {
                totalBodyPoints++;
                float limX = (X_THRESHOLD/SUBDIV);
                float limY = Y_THRESHOLD/(float)SUBDIV;
                map[floor(j/limX) >= SUBDIV ? SUBDIV - 1 : (int) floor(j/limX)][depthInMeter/limY >= SUBDIV ? SUBDIV - 1 : (int) floor(depthInMeter/limY)]++;
                switch (floor(depthInMeter/limY) >= SUBDIV ? SUBDIV - 1 : (int) floor(depthInMeter/limY))
                {
                case 2:
                    glColor3ub(255, 0, 0);
                    break;
                case 1:
                    glColor3ub(0, 255, 0);
                    break;
                case 0:
                    glColor3ub(0, 0, 255);
                    break;
                }
             }
            }
            else {
                unChanged[i * 640 + j] = true;
                glColor3ub(rgb[(i * 640 + j) * 3], rgb[(i * 640 + j) * 3 + 1], rgb[(i * 640 + j) * 3 + 2]);
            }
            glVertex3f(j, i, depth[i * 640 + j]);
        }
    }  
    glEnd();
    glPopMatrix();
    glutSwapBuffers();

    float bodyPerc[8][8];
    if (calibration_done){
        for (int i = 0; i < SUBDIV; ++i) {
            for (int j = 0; j < SUBDIV; ++j) {
                bodyPerc[i][j] = (float) map[i][j]/(float)totalBodyPoints;
                if (totalBodyPoints > 2000){
                     printf("%f",(bodyPerc[j][i]));
                     printf(" ");
                    int len = tosc_writeMessage(buffer, sizeof(buffer), "/data", "fff", (float)i,(float)j, bodyPerc[i][j]);
                    if (sendto(oscSendSocket, buffer, len, 0, (struct sockaddr *)&oscAddr, sizeof(oscAddr)) < 0) {
                        perror("Failed to send OSC message");
                        close(oscSendSocket);
                        exit(1); // Exit or handle the failure as needed
                    }
                }
                printf(" ");
            }
            printf("\n");
        }
    }
    printf("------ END CYCLE ------\n");


    if (!hasStarted) {
        int len = tosc_writeMessage(buffer, sizeof(buffer), "/start", "f", 0);
        if (sendto(oscSendSocket, buffer, len, 0, (struct sockaddr *)&oscAddr, sizeof(oscAddr)) < 0) {
            perror("Failed to send OSC message");
            close(oscSendSocket);
            exit(1); // Exit or handle the failure as needed
        }
        hasStarted = true;
    }

}

void keyPressed(unsigned char key, int x, int y)
{
    if (key == 27) {
        freenect_sync_stop();
        glutDestroyWindow(window);
        exit(0);
    }
    if (key == 'w')
        zoom *= 1.1f;
    if (key == 's')
        zoom /= 1.1f;
    if (key == 'c')
        color = !color;
    if (key == 'e')
        calibration();
    if (key == 'p')
        Y_THRESHOLD+=50;
    if (key == 'o')
        Y_THRESHOLD-=50;
    if (key == 'v')
        calibration_done = false;
}

void ReSizeGLScene(int Width, int Height)
{
    glViewport(0, 0, Width, Height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, (double)Width / (double)Height, 0.3, 200);
    glMatrixMode(GL_MODELVIEW);
}

void InitGL(int Width, int Height)
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glGenTextures(1, &gl_rgb_tex);
    glBindTexture(GL_TEXTURE_2D, gl_rgb_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ReSizeGLScene(Width, Height);
}

int main(int argc, char **argv)
{
    initOSC();
    // Initialize GLUT and create window
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);
    glutInitWindowSize(640, 480);
    glutInitWindowPosition(0, 0);
    window = glutCreateWindow("LibFreenect");

    // Register callbacks
    glutDisplayFunc(DrawGLScene);
    glutIdleFunc(DrawGLScene);
    glutReshapeFunc(ReSizeGLScene);
    glutKeyboardFunc(keyPressed);
    glutMotionFunc(mouseMoved);
    glutMouseFunc(mousePress);

    // Initialize OpenGL settings
    InitGL(640, 480);

    // Register Ctrl+C handler
    signal(SIGINT, sigintHandler);

    // Main loop
    glutMainLoop();

    // Cleanup and exit
    close(oscSendSocket);
    close(oscRecvSocket);
    return 0;
}