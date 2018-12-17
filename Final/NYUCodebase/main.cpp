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
#include <SDL_mixer.h>
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

enum EntityType {ENTITY_PLAYER, ENTITY_KEY, ENTITY_DOOR, ENTITY_POI, ENTITY_ENEMY};

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

		bool facingRight;
		glm::vec3 squish;

		//for doors
		bool isLocked;

		//for enemies
		bool isAngry;
		glm::vec3 resetPos;
		
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

	//squish
	squish[0] = lerp(1.0f, 0.7f, abs(velocity[1] / 2.0f));
	squish[1] = lerp(1.0f, 1.5f, abs(velocity[1] / 2.0f));
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
	modelMatrix = glm::scale(modelMatrix, glm::vec3((facingRight ? size[0] : -size[0]), size[1], size[2]));
	if (entityType == ENTITY_PLAYER) { modelMatrix = glm::scale(modelMatrix, squish); }

	program.SetModelMatrix(modelMatrix);
	sprite.Draw(program);
}

class Particle {
public:
	Particle(float lifetime_in, glm::vec3 position_in);

	glm::vec3 position;
	glm::vec3 velocity;
	float lifetime;
};

Particle::Particle(float lifetime_in, glm::vec3 position_in) {
	position = position_in;
	velocity = glm::vec3(0.0f);
	lifetime = lifetime_in;
}

class ParticleEmitter {
public:
	ParticleEmitter(unsigned int particleCount, float maxLifetime_in,
		glm::vec3 position_in, glm::vec3 gravity_in);

	void Update(float elapsed);
	void Render(ShaderProgram &program);
	
	glm::vec3 position;
	glm::vec3 gravity;
	glm::vec3 velocity;
	glm::vec3 velocityDeviation;
	float maxLifetime;

	glm::vec4 startColor;
	glm::vec4 endColor;
	
	std::vector<Particle> particles;
};

void ParticleEmitter::Update(float elapsed) {
	float randPercent;
	for (int i = 0; i < particles.size(); i++) {
		particles[i].velocity[0] += gravity[0] * elapsed;
		particles[i].velocity[1] += gravity[1] * elapsed;
		particles[i].position[0] += particles[i].velocity[0] * elapsed;
		particles[i].position[1] += particles[i].velocity[1] * elapsed;

		particles[i].lifetime += elapsed;
		if (particles[i].lifetime > maxLifetime) {
			//reset particle
			particles[i].velocity = velocity;
			particles[i].position = position;
			particles[i].lifetime -= maxLifetime;

			randPercent = (float)((rand() % 201) - 100) / 100.0f;
			particles[i].velocity[0] += randPercent * velocityDeviation[0];
			particles[i].velocity[1] += randPercent * velocityDeviation[1];

		}

	}
}

ParticleEmitter::ParticleEmitter(unsigned int particleCount, float maxLifetime_in, glm::vec3 position_in, glm::vec3 gravity_in) {
	maxLifetime = maxLifetime_in;
	position = position_in;
	gravity = gravity_in;
	velocity = glm::vec3(0.0f);
	velocityDeviation = glm::vec3(0.1f);
	float lifetime, randPercent;
	for (int i = 0; i < particleCount; i++) {
		lifetime = ((float)((rand() % 100) + 1) / 100.0f) * maxLifetime;
		particles.push_back(*new Particle(lifetime, position));

		randPercent = (float)((rand() % 201) - 100) / 100.0f;
		particles[i].velocity[0] += randPercent * velocityDeviation[0];
		particles[i].velocity[1] += randPercent * velocityDeviation[1];
	}
	//startColor = glm::vec4(0.81f, 0.263f, 0.11f, 1.0f);
	//endColor = glm::vec4(0.22f, 0.157f, 0.1333f, 0.2f);
	startColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
	endColor = glm::vec4(0.0f, 0.0f, 1.0f, 0.2f);
}

void ParticleEmitter::Render(ShaderProgram &program) {
	program.SetModelMatrix(glm::mat4(1.0f));
	program.SetColor(1.0f, 0.0f, 0.0f, 1.0f);

	glPointSize(100.0f);

	vector<float> vertices;
	for (int i = 0; i < particles.size(); i++) {
		vertices.push_back(particles[i].position[0]);
		vertices.push_back(particles[i].position[1]);
	}

	vector<float> particleColors;
	for (int i = 0; i < particles.size(); i++) {
		float relativeLifetime = (particles[i].lifetime / maxLifetime);
		particleColors.push_back(lerp(startColor[0], endColor[0], relativeLifetime));
		particleColors.push_back(lerp(startColor[1], endColor[1], relativeLifetime));
		particleColors.push_back(lerp(startColor[2], endColor[2], relativeLifetime));
		particleColors.push_back(lerp(startColor[3], endColor[3], relativeLifetime));
	}

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
	glEnableVertexAttribArray(program.positionAttribute);

	//GLuint colorAttribute = glGetAttribLocation(program.programID, "color");
	//glVertexAttribPointer(colorAttribute, 4, GL_FLOAT, false, 0, particleColors.data());
	//glEnableVertexAttribArray(colorAttribute);

	glDrawArrays(GL_POINTS, 0, particles.size());
	glDisableVertexAttribArray(program.positionAttribute);
	//glDisableVertexAttribArray(colorAttribute);
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

GLuint fontTexture, keyTexture, tilesTexture, playerTexture, emptyTexture, beeTexture;

enum gameMode {MODE_START, MODE_OUTDOORS, MODE_STORE, MODE_EXIT, MODE_GAMEOVER, MODE_VICTORY};
gameMode mode;

Mix_Chunk *pickup, *jump, *ribbit;
Mix_Music *bgm_outdoors, *bgm_store, *bgm_exit;

#define FIXED_TIMESTEP 0.01666667f
#define TILE_SIZE 0.2f
#define SPRITE_COUNT_X 14
#define SPRITE_COUNT_Y 14
#define NUM_SOLIDS 7

vector<float> level_vertexData;
vector<float> level_texCoordData;
vector<float> overlay_vertexData;
vector<float> overlay_texCoordData;
vector<float> temporary_vertexData;
vector<float> temporary_texCoordData;

Entity Player, Key, Door, PointOfInterest, Enemy;

vector<ParticleEmitter> ParticleEmitters;

bool showOverlay = true;
bool showTemporary = true;
bool showPyrotechnics = false;

bool showFlavorText = false;
string flavorText;
bool ribbited = false;

int mapWidth, mapHeight;
unsigned int** levelData, **overlayData, **temporaryData;
unsigned int solid_indices[NUM_SOLIDS] = {
	114, 100, 86, 72, 120, 177, 64
}; 

glm::mat4 projectionMatrix = glm::mat4(1.0f);
glm::mat4 viewMatrix = glm::mat4(1.0f);

float maxCameraX, maxCameraY, minCameraX, minCameraY;

ShaderProgram program, pointProgram;

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
		overlayData = new unsigned int*[mapHeight];
		temporaryData = new unsigned int*[mapHeight];
		for (int i = 0; i < mapHeight; ++i) {
			levelData[i] = new unsigned int[mapWidth];
			overlayData[i] = new unsigned int[mapWidth];
			temporaryData[i] = new unsigned int[mapWidth];
		}
		return true;
	}
}

void populateLevelArray(ifstream &stream, unsigned int** &levelArray) {
	string line;
	getline(stream, line);
	for (int y = 0; y < mapHeight; y++) {
		getline(stream, line);
		istringstream lineStream(line);
		string tile;
		for (int x = 0; x < mapWidth; x++) {
			getline(lineStream, tile, ',');
			unsigned int val = (unsigned int)atoi(tile.c_str());
			if (val > 0) {
				levelArray[y][x] = val - 1;
			}
			else {
				levelArray[y][x] = 0;
			}
		}
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
		if (key == "type") {
			if (value == "base") {
				populateLevelArray(stream, levelData);
			}
			else if (value == "overlay") {
				populateLevelArray(stream, overlayData);
			}
			else if (value == "temporary") {
				populateLevelArray(stream, temporaryData);
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
		Player.facingRight = true;
	}
	else if (type == "key") {
		Key.entityType = ENTITY_KEY;
		Key.position = glm::vec3(placeX + 0.5f * TILE_SIZE, placeY, 0.0f);
		Key.isStatic = false;
		Key.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
		Key.animFPS = 1.0f;
		Key.elapsedSinceLastAnim = 0.0f;
		Key.sprite = SheetSprite(keyTexture, 1.0f, 1.0f, 1.0f);
		Key.size = glm::vec3(0.1714f, 0.16f, 1.0f);
		Key.sprite.indices.insert(Key.sprite.indices.end(), { 0.0f, 0.0f });
		Key.facingRight = true;
	}
	else if (type == "door") {
		Door.entityType = ENTITY_DOOR;
		Door.position = glm::vec3(placeX + 0.5f * TILE_SIZE, placeY, 0.0f);
		Door.isStatic = true;
		Door.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
		Door.animFPS = 1.0f;
		Door.elapsedSinceLastAnim = 0.0f;
		Door.sprite = SheetSprite(emptyTexture, 1.0f, 1.0f, 1.0f);
		Door.size = glm::vec3(0.2f, 0.2f, 1.0f);
		Door.sprite.indices.insert(Door.sprite.indices.end(), { 0.0f, 0.0f });
		Door.isLocked = true;
		Door.facingRight = true;
	}
	else if (type == "POI") {
		PointOfInterest.entityType = ENTITY_POI;
		PointOfInterest.position = glm::vec3(placeX + 0.5f * TILE_SIZE, placeY, 0.0f);
		PointOfInterest.isStatic = true;
		PointOfInterest.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
		PointOfInterest.animFPS = 1.0f;
		PointOfInterest.elapsedSinceLastAnim = 0.0f;
		PointOfInterest.sprite = SheetSprite(emptyTexture, 1.0f, 1.0f, 1.0f);
		PointOfInterest.size = glm::vec3(0.3, 0.3f, 1.0f);
		PointOfInterest.sprite.indices.insert(PointOfInterest.sprite.indices.end(), { 0.0f, 0.0f });
		PointOfInterest.facingRight = true;
	}
	else if (type == "enemy") {
		Enemy.entityType = ENTITY_ENEMY;
		Enemy.position = glm::vec3(placeX + 0.5f * TILE_SIZE, placeY, 0.0f);
		Enemy.isStatic = false;
		Enemy.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
		Enemy.animFPS = 1.0f;
		Enemy.elapsedSinceLastAnim = 0.0f;
		Enemy.sprite = SheetSprite(beeTexture, 1.0f, 1.0f, 1.0f);
		Enemy.size = glm::vec3(0.16, 0.137f, 1.0f);
		Enemy.sprite.indices.insert(Enemy.sprite.indices.end(), { 0.0f, 0.0f });
		Enemy.facingRight = true;
		Enemy.isAngry = false;
		Enemy.resetPos = glm::vec3(placeX + 0.5f * TILE_SIZE, placeY, 0.0f);
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

void populateLevelVector(vector<float> &levelVertexVector, vector<float> &levelTexCoordVector, unsigned int** &levelArray) {
	for (int y = 0; y < mapHeight; y++) {
		for (int x = 0; x < mapWidth; x++) {
			if (levelArray[y][x] != 0) {
				float u = (float)(((int)levelArray[y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
				float v = (float)(((int)levelArray[y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;

				//manually putting these in b/c of annoying padding in the spritesheet
				float spriteWidth = 0.069444444f;
				float spriteHeight = 0.069444444f;

				levelVertexVector.insert(levelVertexVector.end(), {
					TILE_SIZE * x, -TILE_SIZE * y,
					TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					TILE_SIZE * x, -TILE_SIZE * y,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
					});
				levelTexCoordVector.insert(levelTexCoordVector.end(), {
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
}

void SetupLevel(string filename, Mix_Music* &music) {
	//Setup the Level/Objects
	ifstream infile(RESOURCE_FOLDER+filename);
	string line;
	while (getline(infile, line)) {
		if (line == "[header]") {
			(!readHeader(infile));
		}
		else if (line == "[layer]") {
			readLayerData(infile);
		}
		else if (line == "[objectLayer]") {
			readEntityData(infile);
		}
	}

	populateLevelVector(level_vertexData, level_texCoordData, levelData);
	populateLevelVector(overlay_vertexData, overlay_texCoordData, overlayData);
	populateLevelVector(temporary_vertexData, temporary_texCoordData, temporaryData);

	//determine Camera Extremes
	minCameraX = 1.777f + TILE_SIZE;
	minCameraY = 1.0f + TILE_SIZE;
	maxCameraX = (mapWidth * TILE_SIZE) - 1.777f - TILE_SIZE;
	maxCameraY = (mapHeight * TILE_SIZE) - 1.0f - TILE_SIZE;

	//start the music
	Mix_PlayMusic(music, -1);

	//restore bools
	showOverlay = true;
	showTemporary = true;
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

glm::vec3 getCameraPos() {
	glm::vec3 currentPos = Player.position;

	currentPos[0] = (currentPos[0] < minCameraX ? -minCameraX : (currentPos[0] > maxCameraX ? -maxCameraX : -currentPos[0]));

	currentPos[1] = (currentPos[1] < -maxCameraY ? maxCameraY : (currentPos[1] > -minCameraY ? minCameraY : -currentPos[1]));

	return currentPos;
}

void ExitLevel() {
	for (int i = 0; i < mapHeight; i++) {
		delete[] levelData[i];
		delete[] overlayData[i];
		delete[] temporaryData[i];
	}
	delete[] levelData;
	delete[] overlayData;
	delete[] temporaryData;

	level_vertexData.clear();
	level_texCoordData.clear();
	overlay_vertexData.clear();
	overlay_texCoordData.clear();
	temporary_vertexData.clear();
	temporary_texCoordData.clear();

	if (!ParticleEmitters.empty()) {
		for (int i = 0; i < ParticleEmitters.size(); i++) {
			ParticleEmitters[i].particles.clear();
		}
		ParticleEmitters.clear();
	}

	Mix_HaltMusic();
}

void Update(float elapsed) {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);

	switch (mode)
	{
	case MODE_START:
		if (keys[SDL_SCANCODE_SPACE]) {
			SetupLevel("FinalMap_Outdoors.txt", bgm_outdoors);
			mode = MODE_OUTDOORS;
			//ParticleEmitters.push_back(*new ParticleEmitter(25, 3.0f, Player.position, glm::vec3(0.0f, 0.25f, 0.0f)));
			//showPyrotechnics = true;
		}
		break;
	case MODE_GAMEOVER:
		if (keys[SDL_SCANCODE_SPACE]) {
			SetupLevel("FinalMap_Exit.txt", bgm_exit);
			Door.isLocked = false;
			mode = MODE_EXIT;
		}
		break;
	case MODE_VICTORY:
		if (keys[SDL_SCANCODE_SPACE]) {
			SetupLevel("FinalMap_Outdoors.txt", bgm_outdoors);
			mode = MODE_OUTDOORS;
		}
		break;
	default:
		//Update Player

	//every frame
		Player.acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
		Player.Animate(elapsed);

		//move left and right
		if (!Player.collidedLeft && keys[SDL_SCANCODE_LEFT]) {
			Player.acceleration[0] = -1.5f;
			Player.facingRight = false;
		}
		else if (!Player.collidedRight && keys[SDL_SCANCODE_RIGHT]) {
			Player.acceleration[0] = 1.5f;
			Player.facingRight = true;
		}

		//jump
		if (Player.collidedBottom && keys[SDL_SCANCODE_SPACE]) {
			Player.velocity[1] = 2.0f;
			Mix_PlayChannel(-1, jump, 0);
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

		//Update Key 
		if (mode == MODE_OUTDOORS) {
			if (Key.IsColliding(Player)) {
				Mix_PlayChannel(-1, pickup, 0);
				Key.position[0] = -100.0f;
				Door.isLocked = false;
				showTemporary = false;
				Key.isStatic = true;
			}
			if (!Key.isStatic) {
				//every frame
				Key.acceleration = glm::vec3(0.0f, 0.0f, 0.0f);

				//gravity
				if (!Key.collidedBottom) {
					Key.acceleration[1] = -0.7f;
				}
				//do work on y-axis
				Key.UpdateY(elapsed);
				HandleTilemapCollisionY(Key);

				//x-axis
				Key.UpdateX(elapsed);
				HandleTilemapCollisionX(Key);
			}
		}

		//Update Point of Interest (show text if near it)
		if (PointOfInterest.IsColliding(Player)) {
			showFlavorText = true;
			switch (mode) {
			case MODE_OUTDOORS:
				flavorText = "What a terrible sign!";
				break;
			case MODE_STORE:
				flavorText = (showOverlay ? "I can't see a thing! (Press UP to light torch)" : "Woah, it's lit in here!");
				if (keys[SDL_SCANCODE_UP]) {
					showOverlay = false;
					showTemporary = false;
					showPyrotechnics = true;
					Door.isLocked = false;
				}
				break;
			case MODE_EXIT:
				flavorText = "If I get to close to that bee's turf, he'll come after me.";
				break;
			default:
				break;
			}
		}

		//Update Door
		if (Door.IsColliding(Player)) {
			showFlavorText = true;
			flavorText = (Door.isLocked ? "It's locked." : "Press UP to proceed.");
			if (!Door.isLocked && keys[SDL_SCANCODE_UP]) {
				switch (mode) {
				case MODE_OUTDOORS:
					ExitLevel();
					glClearColor(0.6f, 0.41961f, 0.29f, 1.0f);
					mode = MODE_STORE;
					/*for (int i = 0; i < 4; i++) {
						ParticleEmitters.push_back(*new ParticleEmitter(15, 3.0f, glm::vec3((float)i, -1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
					} */
					SetupLevel("FinalMap_Store.txt", bgm_store);
					break;
				case MODE_STORE:
					ExitLevel();
					glClearColor(0.05f, 0.46f, 0.8f, 1.0f);
					mode = MODE_EXIT;
					SetupLevel("FinalMap_Exit.txt", bgm_exit);
					Door.isLocked = false;
					break;
				case MODE_EXIT:
					ExitLevel();
					mode = MODE_VICTORY;
					break;
				}
			}
		}

		//both
		if (!Door.IsColliding(Player) && !PointOfInterest.IsColliding(Player)) {
			showFlavorText = false;
			ribbited = false;
		}

		//Update enemy 
		if (mode == MODE_EXIT) {
			Enemy.isAngry = (abs(Enemy.resetPos[0] - Player.position[0]) < 1.0f);
			if (Enemy.isAngry) {
				Enemy.velocity[0] = (Enemy.position[0] - Player.position[0] < 0.0f ? 0.25f : -0.25f);
				Enemy.velocity[1] = (Enemy.position[1] - Player.position[1] < 0.0f ? 0.25f : -0.25f);
			}
			else if (abs(Enemy.resetPos[0] - Enemy.position[0]) > 0.2f || abs(Enemy.resetPos[1] - Enemy.position[1]) > 0.2f) {
				Enemy.velocity[0] = (Enemy.position[0] - Enemy.resetPos[0] < 0.0f ? 1.0f : -1.0f);
				Enemy.velocity[1] = (Enemy.position[1] - Enemy.resetPos[1] < 0.0f ? 1.0f : -1.0f);
			}
			else {
				Enemy.velocity[0] = 0.0f;
				Enemy.velocity[1] = 0.0f;
			}
			//move
			Enemy.position[0] += Enemy.velocity[0] * elapsed;
			Enemy.position[1] += Enemy.velocity[1] * elapsed;

			//apply random jitters
			float randPercentX = (float)((rand() % 201) - 100) / 100.0f;
			float randPercentY = (float)((rand() % 201) - 100) / 100.0f;
			Enemy.position[0] += 0.01 * randPercentX;
			Enemy.position[1] += 0.01 * randPercentY;

			Enemy.facingRight = Enemy.velocity[0] >= 0.0f;

			if (Enemy.IsColliding(Player)) {
				ExitLevel();
				mode = MODE_GAMEOVER;
				break;
			}
		}

		if (showPyrotechnics = true) {
			for (int i = 0; i < ParticleEmitters.size(); i++) {
				ParticleEmitters[i].Update(elapsed);
			}
		}
		break;
	}

	
}

void renderLevelVector(float* levelVertexVector, float* levelTexCoordVector, int size, ShaderProgram &program) {
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, levelVertexVector);
	glEnableVertexAttribArray(program.positionAttribute);

	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, levelTexCoordVector);
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, size);
	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
}

void Render(ShaderProgram &program) {
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	program.SetModelMatrix(modelMatrix);
	switch (mode) {
	case MODE_START:
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.6f, 0.0f, 0.0f));
		program.SetModelMatrix(modelMatrix);
		DrawText(program, fontTexture, "The Big Beautiful Frog in their FINAL Adventure", 0.1f, -0.05f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -0.15f, 0.0f));
		program.SetModelMatrix(modelMatrix);
		DrawText(program, fontTexture, "Press Space to Begin", 0.1f, -0.05f);
		break;
	case MODE_GAMEOVER:
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.45f, 0.0f, 0.0f));
		program.SetModelMatrix(modelMatrix);
		DrawText(program, fontTexture, "GAME OVER", 0.1f, 0.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.5f, -0.15f, 0.0f));
		program.SetModelMatrix(modelMatrix);
		DrawText(program, fontTexture, "Press Space to Retry or ESC to Exit", 0.1f, -0.05f);
		break;
	case MODE_VICTORY:
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.5f, 0.0f, 0.0f));
		program.SetModelMatrix(modelMatrix);
		DrawText(program, fontTexture, "Congratulations!", 0.1f, -0.05f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.5f, -0.15f, 0.0f));
		program.SetModelMatrix(modelMatrix);
		DrawText(program, fontTexture, "Press Space to Play Again or ESC to Exit", 0.1f, -0.05f);
		break;
	default:
		//draw level
		glBindTexture(GL_TEXTURE_2D, tilesTexture);

		renderLevelVector(level_vertexData.data(), level_texCoordData.data(), level_vertexData.size(), program);
		if (showOverlay) { renderLevelVector(overlay_vertexData.data(), overlay_texCoordData.data(), overlay_vertexData.size(), program); }
		if (showTemporary) { renderLevelVector(temporary_vertexData.data(), temporary_texCoordData.data(), temporary_vertexData.size(), program); }

		//draw visible entities

		//player
		Player.Draw(program);

		//Draw Key if we havent moved it offscreen
		if (mode == MODE_OUTDOORS && Key.position[0] > 0) {
			Key.Draw(program);
		}

		//Draw bee if were in the last stage
		if (mode == MODE_EXIT) {
			Enemy.Draw(program);
		}

		//Draw Level's Flavor text
		if (showFlavorText) {
			modelMatrix = glm::mat4(1.0f);
			glm::vec3 textPos = getCameraPos();
			textPos[0] = -textPos[0] - 1.6f;
			textPos[1] = -textPos[1] - 0.9f;
			modelMatrix = glm::translate(modelMatrix, textPos);
			program.SetModelMatrix(modelMatrix);
			DrawText(program, fontTexture, flavorText, 0.1f, -0.05f);
			if (!ribbited) { Mix_PlayChannel(-1, ribbit, 0); }
			ribbited = true;
		}

		//Fiyah
		if (showPyrotechnics) {
			glUseProgram(pointProgram.programID);
			for (int i = 0; i < ParticleEmitters.size(); i++) {
				ParticleEmitters[i].Render(pointProgram);
			}
			glUseProgram(program.programID);
		}
		break;
	}
}

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("tBBF6: The Final Adventure", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif

	glViewport(0, 0, 1280, 720);

	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);

	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	pointProgram.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
	
	program.SetProjectionMatrix(projectionMatrix);
	pointProgram.SetProjectionMatrix(projectionMatrix);
	pointProgram.SetModelMatrix(glm::mat4(1.0f));

	glClearColor(0.05f, 0.46f, 0.8f, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);

	fontTexture = LoadTextureNearest(RESOURCE_FOLDER"font_spritesheet.png");
	beeTexture = LoadTextureNearest(RESOURCE_FOLDER"bee.png");
	playerTexture = LoadTextureNearest(RESOURCE_FOLDER"frog.png");
	keyTexture = LoadTextureNearest(RESOURCE_FOLDER"keyYellow.png");
	tilesTexture = LoadTextureNearest(RESOURCE_FOLDER"tiles_spritesheet_plus2.png");

	pickup = Mix_LoadWAV(RESOURCE_FOLDER"coinPickup.wav");
	jump = Mix_LoadWAV(RESOURCE_FOLDER"jump.wav");
	ribbit = Mix_LoadWAV(RESOURCE_FOLDER"ribbit.wav");
	bgm_outdoors = Mix_LoadMUS(RESOURCE_FOLDER"bgm.mp3");
	bgm_store = Mix_LoadMUS(RESOURCE_FOLDER"bgm_store.mp3");
	bgm_exit = Mix_LoadMUS(RESOURCE_FOLDER"bgm_exit.mp3");
	
	Mix_VolumeMusic(15);

	float acc = 0.0f;

	mode = MODE_START;

	srand(time(NULL));

    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
			}
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
					done = true;
				}
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
		if (mode == MODE_OUTDOORS || mode == MODE_STORE || mode == MODE_EXIT) {
			viewMatrix = glm::translate(viewMatrix, getCameraPos());
		}
		program.SetViewMatrix(viewMatrix);
		pointProgram.SetViewMatrix(viewMatrix);
		Render(program);

        SDL_GL_SwapWindow(displayWindow);
    }

	Mix_FreeChunk(jump);
	Mix_FreeChunk(pickup);
	Mix_FreeChunk(ribbit);
	Mix_FreeMusic(bgm_outdoors);
	Mix_FreeMusic(bgm_store);
	Mix_FreeMusic(bgm_exit);
    
    SDL_Quit();
    return 0;
}
