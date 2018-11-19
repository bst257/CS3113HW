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
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>

using namespace std;

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
		vector<float> indices;
		unsigned int currAnimFrame;
		float u;
		float v;
		float width;
		float height;

		void Animate();
};

enum EntityType {ENTITY_PLAYER, ENTITY_COIN};

class Entity {
	public:

		void Draw(ShaderProgram &program);
		void Animate(float elapsed);
		void UpdateX(float elapsed);
		void UpdateY(float elapsed);

		bool IsColliding(Entity &entity);

		glm::vec3 position;
		glm::vec3 velocity;
		glm::vec3 size;
		glm::vec3 acceleration;

		bool isStatic;

		float rotation;
		
		SheetSprite sprite;

		float animFPS;
		float elapsedSinceLastAnim;

		EntityType entityType;

		bool collidedTop;
		bool collidedBottom;
		bool collidedLeft;
		bool collidedRight;
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

	float vertices[] = {
		-0.5f * size, -0.5f * size,
		0.5f * size, 0.5f * size,
		-0.5f * size, 0.5f * size,
		0.5f * size, 0.5f * size,
		-0.5f * size, -0.5f * size,
		0.5f * size, -0.5f * size,
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

bool Entity::IsColliding(Entity &entity) {
	float e1HalfWidth = (sprite.size * size[0]) / 2.0f;
	float e1HalfHeight = (sprite.size * size[1]) / 2.0f;
	float e2HalfWidth = (entity.sprite.size * entity.size[0]) / 2.0f;
	float e2HalfHeight = (entity.sprite.size * entity.size[1]) / 2.0f;
	bool xOverlap = ((float)abs(entity.position[0] - position[0]) <= e1HalfWidth + e2HalfWidth);
	bool yOverlap = ((float)abs(entity.position[1] - position[1]) <= e1HalfHeight + e2HalfHeight);
	return (xOverlap && yOverlap);
}

float lerp(float v0, float v1, float t) {
	return (float)((1.0 - t)*v0 + t * v1);
}

void Entity::Animate(float elapsed) {
	elapsedSinceLastAnim += elapsed;
	if (elapsedSinceLastAnim >= 1.0f / animFPS) {
		sprite.Animate();
		elapsedSinceLastAnim = 0.0f;
	}
}

void Entity::UpdateX(float elapsed) {
	if (!isStatic) {
		velocity[0] += acceleration[0] * elapsed;
		velocity[0] = lerp(velocity[0], 0.0f, 2.0f * elapsed);
		position[0] += velocity[0] * elapsed;
	}
}

void Entity::UpdateY(float elapsed) {
	if (!isStatic) {
		velocity[1] += acceleration[1] * elapsed;
		//velocity[1] = lerp(velocity[1], 0.0f, 0.5f * elapsed);
		position[1] += velocity[1] * elapsed;
	}
}

void Entity::Draw(ShaderProgram &program) {
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, position);
	modelMatrix = glm::scale(modelMatrix, size);

	program.SetModelMatrix(modelMatrix);
	sprite.Draw(program);
}

GLuint LoadTextureNearest(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);

	if (image == NULL) {
		cout << "Unable to load image. Make sure the path is correct\n";
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
		cout << "Unable to load image. Make sure the path is correct\n";
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

GLuint fontTexture, coinTexture, tilesTexture, playerTexture;

#define FIXED_TIMESTEP 0.01666667f
#define TILE_SIZE 0.2f
#define LEVEL1_WIDTH 20
#define LEVEL1_HEIGHT 27
#define SPRITE_COUNT_X 14
#define SPRITE_COUNT_Y 14
#define NUM_SOLIDS 12

vector<float> level1_vertexData;
vector<float> level1_texCoordData;

vector<Entity> coins;
Entity Player;

int mapWidth, mapHeight;
unsigned int** levelData;
unsigned int solid_indices[NUM_SOLIDS] = {
	148, 17, 170, 171, 157, 143, 131, 117, 103, 134, 120, 106
};

glm::mat4 projectionMatrix = glm::mat4(1.0f);
glm::mat4 viewMatrix = glm::mat4(1.0f);

ShaderProgram program;

//Tilemap/Level Generation
bool readHeader(ifstream &stream) {
	string line;
	mapWidth = -1;
	mapHeight = -1;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "width") {
			mapWidth = atoi(value.c_str());
		}
		else if (key == "height") {
			mapHeight = atoi(value.c_str());
		}
	}
	if (mapWidth == -1 || mapHeight == -1) {
		return false;
	}
	else { // allocate our map data
		levelData = new unsigned int*[mapHeight];
		for (int i = 0; i < mapHeight; ++i) {
			levelData[i] = new unsigned int[mapWidth];
		}
		return true;
	}
}

bool readLayerData(ifstream &stream) {
	string line;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "data") {
			for (int y = 0; y < mapHeight; y++) {
				getline(stream, line);
				istringstream lineStream(line);
				string tile;
				for (int x = 0; x < mapWidth; x++) {
					getline(lineStream, tile, ',');
					unsigned int val = (unsigned int)atoi(tile.c_str());
					if (val > 0) {
						levelData[y][x] = val - 1;
					}
					else {
						levelData[y][x] = 0;
					}
				}
			}
		}
	}
	return true;
}

void placeEntity(string type, float placeX, float placeY) {
	if (type == "player") {
		Player.entityType = ENTITY_PLAYER;
		Player.position = glm::vec3(placeX, placeY, 0.0f);
		Player.isStatic = false;
		Player.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
		Player.animFPS = 1.0f;
		Player.elapsedSinceLastAnim = 0.0f;
		Player.sprite = SheetSprite(playerTexture, 0.5f, 0.5f, 1.0f);
		Player.size = glm::vec3(0.165714f, 0.111429f, 1.0f);
		Player.sprite.indices.insert(Player.sprite.indices.end(), {
			0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f,
			0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f,
			0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f,
			0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.5f, 0.5f, 0.0f
		});
		Player.animFPS = 4.0f;
	}
	else if (type == "coin") {
		Entity newCoin;
		newCoin.entityType = ENTITY_COIN;
		newCoin.position = glm::vec3(placeX + 0.5f * TILE_SIZE, placeY, 0.0f);
		newCoin.isStatic = false;
		newCoin.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
		newCoin.animFPS = 1.0f;
		newCoin.elapsedSinceLastAnim = 0.0f;
		newCoin.sprite = SheetSprite(coinTexture, 1.0f, 1.0f, 1.0f);
		newCoin.size = glm::vec3(0.1f, 0.1444f, 1.0f);
		newCoin.sprite.indices.insert(newCoin.sprite.indices.end(), { 0.0f, 0.0f });
		coins.push_back(newCoin);
	}
}

bool readEntityData(ifstream &stream) {
	string line;
	string type;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "type") {
			type = value;
		}
		else if (key == "location") {
			istringstream lineStream(value);
			string xPosition, yPosition;
			getline(lineStream, xPosition, ',');
			getline(lineStream, yPosition, ',');

			float placeX = atoi(xPosition.c_str()) * TILE_SIZE;
			float placeY = atoi(yPosition.c_str()) * -TILE_SIZE;
			
			placeEntity(type, placeX, placeY);
		}
	}
	return true;
}

void SetupGame() {
	//Setup the Level/Objects
	ifstream infile(RESOURCE_FOLDER"tilemapData.txt");
	string line;
	while (getline(infile, line)) {
		if (line == "[header]") {
			(!readHeader(infile));
		}
		else if (line == "[layer]") {
			readLayerData(infile);
		}
		else if (line == "[coinLayer]") {
			readEntityData(infile);
		}
	}

	for (int y = 0; y < LEVEL1_HEIGHT; y++) {
		for (int x = 0; x < LEVEL1_WIDTH; x++) {
			if (levelData[y][x] != 0) {
				float u = (float)(((int)levelData[y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
				float v = (float)(((int)levelData[y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;

				//manually putting these in b/c of annoying padding in the spritesheet
				float spriteWidth = 0.069444444f;
				float spriteHeight = 0.069444444f;

				level1_vertexData.insert(level1_vertexData.end(), {
					TILE_SIZE * x, -TILE_SIZE * y,
					TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					TILE_SIZE * x, -TILE_SIZE * y,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
					});
				level1_texCoordData.insert(level1_texCoordData.end(), {
					u, v,
					u, v + (spriteHeight),
					u + spriteWidth, v + (spriteHeight),
					u, v,
					u + spriteWidth, v + (spriteHeight),
					u + spriteWidth, v
					});
			}
		}
	}

	//Setup the player
	placeEntity("player", 1.0f, -4.4f);
}

void DrawText(ShaderProgram &program, GLuint fontTexture, string text, float size, float spacing) {
	float character_size = 1.0 / 16.0f;
	vector<float> vertexData;
	vector<float> texCoordData;
	for (int i = 0; i < (int)text.size(); i++) {
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

void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
	*gridX = (int)(worldX / TILE_SIZE);
	*gridY = (int)(worldY / -TILE_SIZE);
}

void HandleTilemapCollisionY(Entity &entity) {
	float worldX, worldY;
	int gridX, gridY;
	unsigned int tileIndex;

	entity.collidedTop = false;
	entity.collidedBottom = false;

	//check top
	worldX = entity.position[0];
	worldY = entity.position[1] + 0.5f * entity.size[1];
	worldToTileCoordinates(worldX, worldY, &gridX, &gridY);
	tileIndex = levelData[gridY][gridX] + 1;
	for (int i = 0; i < NUM_SOLIDS; i++) {
		//check if tile at top is solid
		if (tileIndex == solid_indices[i]) {
			//stop moving vertically
			entity.velocity[1] = 0.0f;

			//set collision flag
			entity.collidedTop = true;

			//push out of tile
			entity.position[1] -= (worldY - ((-TILE_SIZE * gridY) - TILE_SIZE)) + 0.001f;
			break;
		}
	}

	//check bottom
	worldX = entity.position[0];
	worldY = entity.position[1] - 0.5f * entity.size[1];
	worldToTileCoordinates(worldX, worldY, &gridX, &gridY);
	tileIndex = levelData[gridY][gridX] + 1;
	for (int i = 0; i < NUM_SOLIDS; i++) {
		//check if tile at bottom is solid
		if (tileIndex == solid_indices[i]) {
			//stop moving vertically
			entity.velocity[1] = 0.0f;

			//set collision flag
			entity.collidedBottom = true;

			//push out of tile
			entity.position[1] += ((-TILE_SIZE * gridY) - worldY) + 0.001f;
			break;
		}
	}
}

void HandleTilemapCollisionX(Entity &entity) {
	float worldX, worldY;
	int gridX, gridY;
	unsigned int tileIndex;

	entity.collidedLeft = false;
	entity.collidedRight = false;

	//check left
	worldX = entity.position[0] - 0.5f * entity.size[0];
	worldY = entity.position[1];
	worldToTileCoordinates(worldX, worldY, &gridX, &gridY);
	tileIndex = levelData[gridY][gridX] + 1;
	for (int i = 0; i < NUM_SOLIDS; i++) {
		//check if tile at left is solid
		if (tileIndex == solid_indices[i]) {
			//stop moving horizontally
			entity.velocity[0] = 0.0f;

			//set collision flag
			entity.collidedLeft = true;

			//push out of tile
			entity.position[0] += (((TILE_SIZE * gridX) + TILE_SIZE) - worldX) + 0.001f;
			break;
		}
	}

	//check right
	worldX = entity.position[0] + 0.5f * entity.size[0];
	worldY = entity.position[1];
	worldToTileCoordinates(worldX, worldY, &gridX, &gridY);
	tileIndex = levelData[gridY][gridX] + 1;
	for (int i = 0; i < NUM_SOLIDS; i++) {
		//check if tile at right is solid
		if (tileIndex == solid_indices[i]) {
			//stop moving horizontally
			entity.velocity[0] = 0.0f;

			//set collision flag
			entity.collidedRight = true;

			//push out of tile
			entity.position[0] -= (worldX - (TILE_SIZE * gridX)) + 0.001f;
			break;
		}
	}
}

void Update(float elapsed) {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);

	//Update Player

	//every frame
	Player.acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
	Player.Animate(elapsed);

	//move left and right
	if (!Player.collidedLeft && keys[SDL_SCANCODE_LEFT]) {
		Player.acceleration[0] = -1.5f;
	}
	else if (!Player.collidedRight && keys[SDL_SCANCODE_RIGHT]) {
		Player.acceleration[0] = 1.5f;
	}

	//jump
	if (Player.collidedBottom && keys[SDL_SCANCODE_SPACE]) {
		Player.velocity[1] = 2.0f;
	}

	//apply gravity if off ground
	if (!Player.collidedBottom) {
		Player.acceleration[1] += -2.0f;
	}

	//do work on y-axis
	Player.UpdateY(elapsed);
	HandleTilemapCollisionY(Player);

	//do work on x-axis
	Player.UpdateX(elapsed);
	HandleTilemapCollisionX(Player);

	//Update Coins
	for (int i = 0; i < (int)coins.size(); i++) {
		if (coins[i].IsColliding(Player)) {
			coins.erase(coins.begin() + i);
		}
	}

	for (int i = 0; i < (int)coins.size(); i++) {
		//every frame
		coins[i].acceleration = glm::vec3(0.0f, 0.0f, 0.0f);

		//gravity
		if (!coins[i].collidedBottom) {
			coins[i].acceleration[1] += -0.7f;
		}
		//do work on y-axis
		coins[i].UpdateY(elapsed);
		HandleTilemapCollisionY(coins[i]);

		//x-axis
		coins[i].UpdateX(elapsed);
		HandleTilemapCollisionX(coins[i]);
	}
}

void Render(ShaderProgram &program) {
	//draw level
	glBindTexture(GL_TEXTURE_2D, tilesTexture);
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	program.SetModelMatrix(modelMatrix);

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, level1_vertexData.data());
	glEnableVertexAttribArray(program.positionAttribute);

	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, level1_texCoordData.data());
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, level1_vertexData.size());
	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);

	//draw entities

	//player
	Player.Draw(program);

	//coins
	for (int i = 0; i < (int)coins.size(); i++) {
		coins[i].Draw(program);
	}
}

glm::vec3 getCameraPos(){
	glm::vec3 currentPos = Player.position;

	currentPos[0] = (currentPos[0] < 1.777f ? -1.777f : (currentPos[0] > 2.223f ? -2.223f : -currentPos[0]));

	currentPos[1] = (currentPos[1] < -4.4f ? 4.4f : (currentPos[1] > -1.0f ? 1.0f : -currentPos[1]));

	return currentPos;
}

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("tBBF4: Hardcore Parkour", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif

	glViewport(0, 0, 1280, 720);

	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);

	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	
	program.SetProjectionMatrix(projectionMatrix);

	glClearColor(0.05f, 0.46f, 0.73f, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	fontTexture = LoadTextureNearest(RESOURCE_FOLDER"font_spritesheet.png");
	playerTexture = LoadTextureNearest(RESOURCE_FOLDER"frog.png");
	coinTexture = LoadTextureNearest(RESOURCE_FOLDER"coinGold.png");
	tilesTexture = LoadTextureNearest(RESOURCE_FOLDER"tiles_spritesheet.png");
	
	float acc = 0.0f;

	SetupGame();

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

		elapsed += acc;
		if (elapsed < FIXED_TIMESTEP) {
			acc = elapsed;
			continue;
		}
		while (elapsed >= FIXED_TIMESTEP) {
			Update(FIXED_TIMESTEP);
			elapsed -= FIXED_TIMESTEP;
		}
		acc = elapsed;

		viewMatrix = glm::mat4(1.0f);
		viewMatrix = glm::translate(viewMatrix, getCameraPos());
		program.SetViewMatrix(viewMatrix);
		Render(program);

        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
