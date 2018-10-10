#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <ctime>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

GLuint LoadTextureNearest(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);

	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}

	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	stbi_image_free(image);
	return retTexture;
}

GLuint LoadTextureLinear(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);

	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}

	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(image);
	return retTexture;
}

float frogScale = 1.0f;
float frogXPosition = 0.0f;
float frogYPosition = 0.0f;
float frogXDirection = -1.0f;
float frogYDirection = 1.0f;
float playerPosition, CPUPosition;
float lastFrameTicks = 0.0f;
bool smartCPU = true;
float arbitCPUDirection = 1.0f;
float prevFrameDirection = -1.0f;
glm::mat4 projectionMatrix = glm::mat4(1.0f);
glm::mat4 modelMatrix = glm::mat4(1.0f);
glm::mat4 viewMatrix = glm::mat4(1.0f);

ShaderProgram program0, program1;

float getFrogAngle() {
	float angleMod = (float)(rand() % 30 + 1) / 100.0f;
	return angleMod * 3.1415926f;
}

float frogAngle = getFrogAngle();

void checkFrogCollision() {
	float playerXPosition = -1.777f + 0.125f;
	float playerYPosition = playerPosition;
	float CPUXPosition = 1.777f - 0.125f;
	float CPUYPosition = CPUPosition;
	float lilypadHeight = 0.5f;
	float lilypadWidth = 0.25f;
	float frogHeight = 0.5f * frogScale;
	float frogWidth = 0.74f * frogScale;

	float xDistance_player = std::abs(playerXPosition - frogXPosition) - ((lilypadHeight + frogHeight) / 2.0f);
	float yDistance_player = std::abs(playerYPosition - frogYPosition) - ((lilypadWidth + frogWidth) / 2.0f);

	float xDistance_CPU = std::abs(CPUXPosition - frogXPosition) - ((lilypadHeight + frogHeight) / 2.0f);
	float yDistance_CPU = std::abs(CPUYPosition - frogYPosition) - ((lilypadWidth + frogWidth) / 2.0f);

	if (yDistance_player < 0 && xDistance_player < 0 && frogXPosition > -1.6f) {
		frogXDirection = 1.0f;
	}

	if (yDistance_CPU < 0 && xDistance_CPU < 0 && frogXPosition < 1.6f) {
		frogXDirection = -1.0f;
	}
}

void determinePolygonPositions(float elapsed) {
	frogYPosition += frogYDirection * elapsed * std::sin(frogAngle) * 2.0f;
	frogXPosition += frogXDirection * elapsed * std::cos(frogAngle) * 2.0f;
	
	if (frogYPosition > 1.0f - 0.1f - (0.25f * frogScale) || frogYPosition < -1.0f + 0.1f + (0.25f * frogScale)) {
		frogYPosition = (frogYPosition > 1.0f - 0.1f - (0.25f * frogScale) ? 1.0f - 0.1f - (0.25f * frogScale) : -1.0f + 0.1f + (0.25f * frogScale));
		frogYDirection *= -1.0f;
	}

	if (playerPosition > 0.65f || playerPosition < -0.65f) {
		playerPosition = (playerPosition > 0.65f ? 0.65f : -0.65f);
	}

	if (smartCPU) {
		CPUPosition = frogYPosition;
	}
	else {
		CPUPosition += arbitCPUDirection * elapsed * 0.5f;
	}

	if (CPUPosition > 0.65f || CPUPosition < -0.65f) {
		CPUPosition = (CPUPosition > 0.65f ? 0.65f : -0.65f);
		arbitCPUDirection *= -1.0f;
	}

	checkFrogCollision();

	//reset on win/lose
	if (frogXPosition > 1.77f || frogXPosition < -1.77f) {
		frogXPosition = 0.0f;
		frogYPosition = 0.0f;
		frogXDirection = -1.0f;
		frogYDirection = 1.0f;
		frogAngle = getFrogAngle();
		prevFrameDirection = -1.0f;
		smartCPU = true;
	}

	frogScale = (-1.0f * (std::abs(frogXPosition) / 1.77f)) + 1.2f;
}

void drawTexturedPolygons() {
	glUseProgram(program0.programID);
	program0.SetProjectionMatrix(projectionMatrix);
	program0.SetViewMatrix(viewMatrix);

	//drawing left lilypad (player)
	GLuint beeTexture = LoadTextureLinear(RESOURCE_FOLDER"lilypad.png");

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.77f, playerPosition, 0.0f));
	program0.SetModelMatrix(modelMatrix);

	float vertices1[] = { 0.0f, -0.25f, 0.25f, 0.25f, 0.0f, 0.25f, 0.0f, -0.25f, 0.25f, -0.25f, 0.25f, 0.25f };
	glVertexAttribPointer(program0.positionAttribute, 2, GL_FLOAT, false, 0, vertices1);
	glEnableVertexAttribArray(program0.positionAttribute);

	float texCoords1[] = { 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f };
	glVertexAttribPointer(program0.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords1);
	glEnableVertexAttribArray(program0.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program0.positionAttribute);
	glDisableVertexAttribArray(program0.texCoordAttribute);

	//drawing right lilypad (CPU)
	GLuint ghostTexture = LoadTextureLinear(RESOURCE_FOLDER"lilypad.png");

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(1.77f, CPUPosition, 0.0f));
	program0.SetModelMatrix(modelMatrix);

	float vertices2[] = { -0.25f, -0.25f, -0.25f, 0.25f, 0.0f, 0.25f, -0.25f, -0.25f, 0.0f, -0.25f, 0.0f, 0.25f };
	glVertexAttribPointer(program0.positionAttribute, 2, GL_FLOAT, false, 0, vertices2);
	glEnableVertexAttribArray(program0.positionAttribute);

	float texCoords2[] = { 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f };
	glVertexAttribPointer(program0.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords2);
	glEnableVertexAttribArray(program0.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program0.positionAttribute);
	glDisableVertexAttribArray(program0.texCoordAttribute);

	//draw the frog
	GLuint frogTexture = LoadTextureNearest(RESOURCE_FOLDER"frog.png");

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(frogXPosition, frogYPosition, 0.0f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(frogScale, frogScale, 1.0f));
	program0.SetModelMatrix(modelMatrix);

	float vertices0[] = { -0.37f, -0.25f, 0.37f, -0.25f, 0.37f, 0.25f, -0.37f, -0.25f, 0.37f, 0.25f, -0.37f, 0.25f };
	glVertexAttribPointer(program0.positionAttribute, 2, GL_FLOAT, false, 0, vertices0);
	glEnableVertexAttribArray(program0.positionAttribute);

	float texCoords0[] = { 0.0f, 1.0f,  1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 1.0f,  1.0f,  0.0f, 0.0f, 0.0f };
	glVertexAttribPointer(program0.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords0);
	glEnableVertexAttribArray(program0.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program0.positionAttribute);
	glDisableVertexAttribArray(program0.texCoordAttribute);
}

void drawUntexturedPolygons() {
	glUseProgram(program1.programID); //switching to untextured polygons
	program1.SetProjectionMatrix(projectionMatrix);
	program1.SetViewMatrix(viewMatrix);
	program1.SetColor(0.08f, 0.49f, 0.2f, 1.0f); //seaweed

	//Midpoint
	modelMatrix = glm::mat4(1.0f);
	program1.SetModelMatrix(modelMatrix);

	float vertices3[] = { -0.025f, -1.0f, 0.025f, 1.0f, -0.025f, 1.0f, -0.025f, -1.0f, 0.025f, -1.0f, 0.025f, 1.0f };
	glVertexAttribPointer(program1.positionAttribute, 2, GL_FLOAT, false, 0, vertices3);
	glEnableVertexAttribArray(program1.positionAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program1.positionAttribute);

	program1.SetColor(0.85f, 0.71f, 0.57f, 1.0f); //sandy

	//Top Wall
	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 1.0f, 0.0f));
	program1.SetModelMatrix(modelMatrix);

	float vertices4[] = { -2.0f, -0.1f, 2.0f, 0.0f, -2.0f, 0.0f, -2.0f, -0.1f, 2.0f, -0.1f, 2.0f, 0.0f };
	glVertexAttribPointer(program1.positionAttribute, 2, GL_FLOAT, false, 0, vertices4);
	glEnableVertexAttribArray(program1.positionAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program1.positionAttribute);

	//Bottom Wall
	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -1.0f, 0.0f));
	program1.SetModelMatrix(modelMatrix);

	float vertices5[] = { -2.0f, 0.0f, 2.0f, 0.1f, -2.0f, 0.1f, -2.0f, 0.0f, 2.0f, 0.0f, 2.0f, 0.1f };
	glVertexAttribPointer(program1.positionAttribute, 2, GL_FLOAT, false, 0, vertices5);
	glEnableVertexAttribArray(program1.positionAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program1.positionAttribute);
}

void highlyAdvancedArtificialIntelligence() {
	//whenever the frog jumps off the CPU lilypad, it either becomes unbeatable or just lazes about until a reset
	if (prevFrameDirection == 1.0f && frogXDirection == -1.0f && smartCPU) {
		smartCPU = ((rand() % 5) == 0);
	}

	prevFrameDirection = frogXDirection;
}

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("The Big Bouncing Frog 2: Lilypong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif

	std::srand((int)time(NULL));

	glViewport(0, 0, 640, 360);

	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);

	program0.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	program1.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");

	glClearColor(0.05f, 0.46f, 0.73f, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
			}
			else if (event.type == SDL_MOUSEMOTION) {
				playerPosition = (((float)(360 - event.motion.y) / 360.0f) * 2.0f) - 1.0f;
			}
        }
        glClear(GL_COLOR_BUFFER_BIT);

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		highlyAdvancedArtificialIntelligence();
		determinePolygonPositions(elapsed);
		drawUntexturedPolygons();
		drawTexturedPolygons();

        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
