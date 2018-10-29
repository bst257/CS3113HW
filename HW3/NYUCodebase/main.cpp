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
#include <vector>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;


class SheetSprite {
	public:
		SheetSprite();
		SheetSprite(GLuint textureID_in, float width_in, float height_in, float size_in);

		void Draw(ShaderProgram &program);

		float size;
		GLuint textureID;
		std::vector<unsigned int> indices;
		unsigned int currAnimFrame;
		float u;
		float v;
		float width;
		float height;

		void Animate();
};

class Entity {
	public:

		void Draw(ShaderProgram &program);
		void Update(float elapsed);

		glm::vec3 position;
		glm::vec3 velocity;
		glm::vec3 size;

		float rotation;
		
		SheetSprite sprite;

		bool alive;
		float animFPS;
		float elapsedSinceLastAnim;
};

SheetSprite::SheetSprite() {
	size = 1.0f;
	textureID = 0;
	u = 1.0f;
	v = 1.0f;
	width = 1.0f;
	height = 1.0f;
}

SheetSprite::SheetSprite(GLuint textureID_in, float width_in, float height_in, float size_in) {
	size = size_in;
	textureID = textureID_in;
	u = 0.0f;
	v = 0.0f;
	currAnimFrame = 0;
	width = width_in;
	height = height_in;
}

void SheetSprite::Draw(ShaderProgram &program) {
	glBindTexture(GL_TEXTURE_2D, textureID);

	GLfloat texCoords[] = {
		u, v + height,
		u + width, v,
		u, v,
		u + width, v,
		u, v + height,
		u + width, v + height
	};

	float aspect = width / height;
	float vertices[] = {
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, 0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, -0.5f * size,
	};

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program.positionAttribute);

	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
}

void SheetSprite::Animate() {
	u = indices[currAnimFrame];
	v = indices[currAnimFrame + 1];
	
	currAnimFrame += 2;
	if (currAnimFrame >= indices.size()) { currAnimFrame = 0; }
}

void Entity::Update(float elapsed) {
	elapsedSinceLastAnim += elapsed;
	if (elapsedSinceLastAnim >= 1.0f / animFPS) {
		sprite.Animate();
	}
	if (alive) {
		position += velocity;
	}
	else if (!alive) {
		position = glm::vec3(-100.0f, 0.0f, 0.0f);
	}
}

void Entity::Draw(ShaderProgram &program) {
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, position);

	program.SetModelMatrix(modelMatrix);
	sprite.Draw(program);
}

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

float lastFrameTicks = 0.0f;

GLuint fontTexture = LoadTextureNearest(RESOURCE_FOLDER"font_spritesheet.png");
GLuint frogTexture = LoadTextureNearest(RESOURCE_FOLDER"frog.png");
GLuint boyTexture = LoadTextureLinear(RESOURCE_FOLDER"boy_spritesheet.png");
GLuint bulletTexture = LoadTextureNearest(RESOURCE_FOLDER"bee.png");

#define MAX_BOYS 50
Entity boys[MAX_BOYS];

#define MAX_BULLETS 25
Entity bullets[MAX_BULLETS];
Entity frog;

glm::mat4 projectionMatrix = glm::mat4(1.0f);
glm::mat4 viewMatrix = glm::mat4(1.0f);

ShaderProgram program0, program1;

enum GameMode { MODE_PRESS_START, MODE_GAME};
GameMode mode;

void DrawText(ShaderProgram &program, GLuint fontTexture, std::string text, float size, float spacing) {
	float character_size = 1.0 / 16.0f;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;
	for (int i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];
		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;
		vertexData.insert(vertexData.end(), {
		((size + spacing) * i) + (-0.5f * size), 0.5f * size,
		((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		((size + spacing) * i) + (0.5f * size), 0.5f * size,
		((size + spacing) * i) + (0.5f * size), -0.5f * size,
		((size + spacing) * i) + (0.5f * size), 0.5f * size,
		((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			});
		texCoordData.insert(texCoordData.end(), {
		texture_x, texture_y,
		texture_x, texture_y + character_size,
		texture_x + character_size, texture_y,
		texture_x + character_size, texture_y + character_size,
		texture_x + character_size, texture_y,
		texture_x, texture_y + character_size,
			});
	}
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	float *vertices = vertexData.data();
	float *texCoords = texCoordData.data();

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program.positionAttribute);
	
	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);
	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);

}

void ProcessInput() {
	switch (mode) {
	case MODE_PRESS_START:
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.85f, 0.0f, 0.0f));
		program0.SetModelMatrix(modelMatrix);
		DrawText(program0, fontTexture, "tBBF3: Modern Warfare", 0.15f, -0.075f);
	}
}

void Update(float elapsed) {
	
}

void Render() {
	
}

void drawUntexturedPolygons() {
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

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("tBBF3: Modern Warfare", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 360, 720, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif

	std::srand((int)time(NULL));

	glViewport(0, 0, 360, 720);

	projectionMatrix = glm::ortho(-1.0f, 1.0f, -2.0f, 2.0f, -1.0f, 1.0f);

	program0.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	program1.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");

	program0.SetModelMatrix(modelMatrix);
	program0.SetProjectionMatrix(projectionMatrix);
	program0.SetViewMatrix(viewMatrix);

	glClearColor(0.05f, 0.46f, 0.73f, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	mode = MODE_PRESS_START;

    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
			}
			else if (event.type == SDL_MOUSEMOTION) {
				//playerPosition = (((float)(360 - event.motion.y) / 360.0f) * 2.0f) - 1.0f;
			}
        }
        glClear(GL_COLOR_BUFFER_BIT);

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		/*
		determinePolygonPositions(elapsed);
		drawUntexturedPolygons();
		drawTexturedPolygons();
		*/

		

        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
