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

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

GLuint LoadTexture(const char *filePath) {
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

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("The Big Bouncing Frog and their Friends", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif

	glViewport(0, 0, 640, 360);

	ShaderProgram program0;
	program0.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	ShaderProgram program1;
	program1.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");

	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	glm::mat4 viewMatrix = glm::mat4(1.0f);
	float scaleRatio = 1.0f;
	float scaleDirection = 1.0f;

	glClearColor(0.51f, 0.26f, 0.8f, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	float lastFrameTicks = 0.0f;

    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
        glClear(GL_COLOR_BUFFER_BIT);

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		scaleRatio += 0.5f * elapsed * scaleDirection;
		if (scaleRatio > 1.25f || scaleRatio < 1.0f) {
			scaleRatio = (scaleRatio > 1.25f ? 1.25f : 1.0f);
			scaleDirection *= -1.0f;
		}

		glUseProgram(program0.programID);
		program0.SetProjectionMatrix(projectionMatrix);
		program0.SetViewMatrix(viewMatrix);

		//draw a big beautiful bouncing baby bfrog
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.5f, 0.0f, 0.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(scaleRatio, scaleRatio, 1.0f));
		
		program0.SetModelMatrix(modelMatrix);

		GLuint frogTexture = LoadTexture(RESOURCE_FOLDER"frog.png");

		float vertices0[] = { -0.37f, -0.25f, 0.37f, -0.25f, 0.37f, 0.25f, -0.37f, -0.25f, 0.37f, 0.25f, -0.37f, 0.25f };
		glVertexAttribPointer(program0.positionAttribute, 2, GL_FLOAT, false, 0, vertices0);
		glEnableVertexAttribArray(program0.positionAttribute);

		float texCoords0[] = { 0.0f, 1.0f,  1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 1.0f,  1.0f,  0.0f, 0.0f, 0.0f };
		glVertexAttribPointer(program0.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords0);
		glEnableVertexAttribArray(program0.texCoordAttribute);
		
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program0.positionAttribute);
		glDisableVertexAttribArray(program0.texCoordAttribute);

		float texCoords[] = { 0.0f, 1.0f,  1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 1.0f,  1.0f,  0.0f, 0.0f, 0.0f };
		glVertexAttribPointer(program0.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program0.texCoordAttribute);
		
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program0.positionAttribute);
		glDisableVertexAttribArray(program0.texCoordAttribute);

		//drawing bee
		GLuint beeTexture = LoadTexture(RESOURCE_FOLDER"bee.png");

		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.5f, 0.4f, 0.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(-0.5f, 0.5f, 1.0f));
		program0.SetModelMatrix(modelMatrix);

		float vertices3[] = { -0.3f, -0.25f, 0.3f, -0.25f, 0.3f, 0.25f, -0.3f, -0.25f, 0.3f, 0.25f, -0.3f, 0.25f };
		glVertexAttribPointer(program0.positionAttribute, 2, GL_FLOAT, false, 0, vertices3);
		glEnableVertexAttribArray(program0.positionAttribute);

		float texCoords1[] = { 0.0f, 1.0f,  1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 1.0f,  1.0f,  0.0f, 0.0f, 0.0f };
		glVertexAttribPointer(program0.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords1);
		glEnableVertexAttribArray(program0.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program0.positionAttribute);
		glDisableVertexAttribArray(program0.texCoordAttribute);

		//drawing ghost
		GLuint ghostTexture = LoadTexture(RESOURCE_FOLDER"ghost.png");

		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(1.25f, 0.4f, 0.0f));
		program0.SetModelMatrix(modelMatrix);

		float vertices4[] = { -0.255f, -0.365f, 0.255f, -0.365f, 0.255f, 0.365f, -0.255f, -0.365f, 0.255f, 0.365f, -0.255f, 0.365f };
		glVertexAttribPointer(program0.positionAttribute, 2, GL_FLOAT, false, 0, vertices4);
		glEnableVertexAttribArray(program0.positionAttribute);

		float texCoords2[] = { 0.0f, 1.0f,  1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 1.0f,  1.0f,  0.0f, 0.0f, 0.0f };
		glVertexAttribPointer(program0.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords2);
		glEnableVertexAttribArray(program0.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program0.positionAttribute);
		glDisableVertexAttribArray(program0.texCoordAttribute);

		glUseProgram(program1.programID); //switching to untextured polygons
		program1.SetProjectionMatrix(projectionMatrix);
		program1.SetViewMatrix(viewMatrix);
		program1.SetColor(0.41f, 0.25f, 0.12f, 1.0f); //tree brown

		//draw trunk
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.75f, -0.25f, 0.0f));
		program1.SetModelMatrix(modelMatrix);

		float vertices5[] = { 0.1f, 0.0f, 0.0f, 0.9f, -0.1f, 0.0f };
		glVertexAttribPointer(program1.positionAttribute, 2, GL_FLOAT, false, 0, vertices5);
		glEnableVertexAttribArray(program1.positionAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisableVertexAttribArray(program1.positionAttribute);

		program1.SetColor(0.15f, 0.57f, 0.21f, 1.0f); //leaf green

		//first green triangle
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.75f, 0.0f, 0.0f));
		program1.SetModelMatrix(modelMatrix);
		
		float vertices1[] = { 0.25f, 0.0f, 0.0f, 0.5f, -0.25f, 0.0f };
		glVertexAttribPointer(program1.positionAttribute, 2, GL_FLOAT, false, 0, vertices1);
		glEnableVertexAttribArray(program1.positionAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisableVertexAttribArray(program1.positionAttribute);

		//Second green triangle
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.75f, 0.2f, 0.0f));
		program1.SetModelMatrix(modelMatrix);

		float vertices2[] = { 0.25f, 0.0f, 0.0f, 0.5f, -0.25f, 0.0f };
		glVertexAttribPointer(program1.positionAttribute, 2, GL_FLOAT, false, 0, vertices2);
		glEnableVertexAttribArray(program1.positionAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisableVertexAttribArray(program1.positionAttribute);

        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
